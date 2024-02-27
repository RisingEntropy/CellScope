#include "OpenSlideFileReader.h"
#include "qimage.h"
#include "qnamespace.h"
#include <stdint.h>
#include <QDebug>
OpenSlideFileReader::OpenSlideFileReader(QString fileName){
    this->file = openslide_open(fileName.toStdString().c_str());
    if(file==NULL){
        this->valid = false;
        return;
    }else{
        this->valid = true;
    }
}
OpenSlideFileReader::~OpenSlideFileReader(){
    if(this->valid){
        openslide_close(this->file);
    }

}
bool OpenSlideFileReader::validateFile(QString fileName){
    return openslide_detect_vendor(fileName.toStdString().c_str())!=NULL;
}

bool OpenSlideFileReader::validFile(){
    return this->valid;
}

int32_t OpenSlideFileReader::getLevelCount(){
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getLevelCount called on an invalid object!";
        return -1;
    }
    return openslide_get_level_count(this->file);
}
int64_t OpenSlideFileReader::getLevelWidth(int32_t level){
    int64_t width,height;
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getLevelWidth called on an invalid object!";
        return 0;
    }
    openslide_get_level_dimensions(this->file, level, &width, &height);
    return width;
}
int64_t OpenSlideFileReader::getLevelHeight(int32_t level){
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getLevelHeight called on an invalid object!";
        return 0;
    }
    int64_t width,height;
    openslide_get_level_dimensions(this->file, level, &width, &height);
    return height;
}
double OpenSlideFileReader::getLevelDownsample(int32_t level){
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getLevelDownsample called on an invalid object!";
        return 0.0;
    }
    return openslide_get_level_downsample(this->file, level);
}
double OpenSlideFileReader::getScaleBetweenLevels(int32_t level1, int32_t level2){//level1 to level2
    if(level1<0||level2<0||level1>=this->getLevelCount()||level2>=this->getLevelCount()){
        qCritical()<<"OpenSlideFileReader: getScaleBetweenLevels called with invalid level number!";
        return 1;
    }
    double level1Scale = this->getLevelDownsample(level1);
    double level2Scale = this->getLevelDownsample(level2);
    return level2Scale/level1Scale;
}
int32_t OpenSlideFileReader::getBestLevelForDownsample(double downsample)
{
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getBestLevelForDownsample called on an invalid object!";
        return 0;
    }
    return openslide_get_best_level_for_downsample(this->file, downsample);
}
uint32_t* OpenSlideFileReader::getRegionRaw(int32_t level, int64_t x, int64_t y, int64_t w, int64_t h){
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: releaseAllocatedSlides called on an invalid object!";
        return nullptr;
    }
    uint32_t* buffer = new uint32_t[w*h];
    if(buffer==NULL){
        qCritical()<<"Error: Failed to allocate buffer for getRegionRaw, is there enough memroy?";
        return nullptr;
    }
    openslide_read_region(this->file, buffer, x, y, level, w, h);
    return buffer;
}
cv::Mat OpenSlideFileReader::getRegionMat(int32_t level, int64_t x, int64_t y, int64_t w, int64_t h, int64_t targetWidth, int64_t targetHeight){
    if(w==0||h==0){
        return cv::Mat();
    }
    if(this->valid==false){
        qDebug()<<"OpenSlideFileReader: getRegionMat called on an invalid object!";
        return cv::Mat();
    }
    double levelX = x/this->getLevelDownsample(level);
    double levelY = y/this->getLevelDownsample(level);
    if(x<0||y<0||w<0||h<0||level<0||level>=this->getLevelCount()||x>this->getLevelWidth(0)||y>this->getLevelHeight(0)||levelX+w>this->getLevelWidth(level)||levelY+h>this->getLevelHeight(level)){
        qWarning()<<"OpenSlideFileReader: getRegionMat called with invalid parameters: x:"<<x<<" y:"<<y<<" w:"<<w<<" h:"<<h<<" level:"<<level<<" levelWidth:"<<this->getLevelWidth(level)<<" levelHeight:"<<this->getLevelHeight(level)<<"!";
        return cv::Mat();
    }
    int64_t W = w==-1?this->getLevelWidth(level):w;
    int64_t H = h==-1?this->getLevelHeight(level):h;

    uint32_t* buffer = this->getRegionRaw(level, x, y, W, H);
    if(buffer==NULL){
        qWarning()<<"Returning an empty cv::Mat object.";
        return cv::Mat();
    }

    cv::Mat rgb(H, W, CV_8UC3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            uint32_t argb = buffer[y * W + x];
            uint8_t a = (argb >> 24) & 0xFF;
            uint8_t r = (argb >> 16) & 0xFF;
            uint8_t g = (argb >> 8) & 0xFF;
            uint8_t b = argb & 0xFF;
            rgb.at<cv::Vec3b>(y, x) = cv::Vec3b(r, g, b);
        }
    }

    if(targetWidth==-1||targetHeight==-1){
        cv::Mat ret = rgb.clone();
        delete [] buffer;
        return ret;
    }else{
        cv::Mat ret;
        cv::resize(rgb, ret, cv::Size(targetWidth, targetHeight), 0, 0, cv::INTER_NEAREST);
        delete [] buffer;
        return ret;
    }
}