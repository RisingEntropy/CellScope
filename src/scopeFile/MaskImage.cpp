#include "MaskImage.h"
#include "qdebug.h"
#include <cstddef>
#include <new>
#include <cstring>
#include <QDebug>
#include <stdint.h>
#include <vcruntime_string.h>
MaskImage::MaskImage(){
    this->data = nullptr;
    this->bufSize = -1;
    this->width = 0;
    this->height = 0;
}
MaskImage::MaskImage(int64_t width, int64_t height){
    try {
        this->data = QSharedPointer<unsigned char>(new unsigned char[(width*height+sizeof(unsigned char)-1)/sizeof(unsigned char)],
                                             [](unsigned char* p){delete[] p;});//use custom deleter to delete the array
        this->bufSize = (width*height+sizeof(unsigned char)-1)/sizeof(unsigned char);
    } catch (std::bad_alloc &memExp) {
        this->data = nullptr;
        this->bufSize = -1;
        qWarning()<<"MaskImage: Failed to allocate memory for MaskImage!";
        return;
    }
    this->width = width;
    this->height = height;

}
MaskImage::MaskImage(unsigned char* data, int64_t width, int64_t height, bool copy){
    // User must make sure the data is valid and reamin unchanged during the entire life if copy is set false. 
    // The buffer size should be (width*height+sizeof(unsigned char)-1)/sizeof(unsigned char).
    if(copy){
        try {
            this->data = QSharedPointer<unsigned char>(new unsigned char[(width*height+sizeof(unsigned char)-1)/sizeof(unsigned char)],
                                                 [](unsigned char* p){delete[] p;});//use custom deleter to delete the array
            this->bufSize = (width*height+sizeof(unsigned char)-1)/sizeof(unsigned char);
        } catch (std::bad_alloc &memExp) {
            this->data = nullptr;
            this->bufSize = -1;
            qWarning()<<"MaskImage: Failed to allocate memory for MaskImage!";
            return;
        }
        this->width = width;
        this->height = height;
        memcpy(this->data.data(), data, this->bufSize);
    }else{
        this->data = QSharedPointer<unsigned char>(data, [](unsigned char* p){});//do noting to delete this array, delete it by provider
        this->bufSize = (width*height+sizeof(unsigned char)-1)/sizeof(unsigned char);
        this->width = width;
        this->height = height;
    }
}
MaskImage::MaskImage(const MaskImage& maskImage){
    this->width = maskImage.width;
    this->height = maskImage.height;
    this->bufSize = maskImage.bufSize;
    if(maskImage.data==nullptr||maskImage.bufSize==-1){
        this->data = nullptr;
        this->bufSize = -1;
        return;
    }
    this->data = maskImage.data;
    
}
MaskImage& MaskImage::operator=(const MaskImage& maskImage){
    this->width = maskImage.width;
    this->height = maskImage.height;
    this->bufSize = maskImage.bufSize;
    if(maskImage.data==nullptr||maskImage.bufSize==-1){
        this->data = nullptr;
        this->bufSize = -1;
        return *this;
    }
    this->data = maskImage.data;
    return *this;
}
MaskImage MaskImage::fromMat(cv::Mat &mask){
    if(mask.type()!=CV_8UC1){
        qWarning()<<"MaskImage: Try to convert a non CV_8UC1 mat to MaskImage!";
        return MaskImage();
    }
    MaskImage ret = MaskImage(mask.cols, mask.rows);
    for(int64_t i=0;i<mask.cols;i++){
        for(int64_t j=0;j<mask.rows;j++){
            ret.setPixel(i, j, mask.at<uchar>(j, i)!=0);
        }
    }
    return ret;
}
MaskImage::~MaskImage(){
}
int64_t MaskImage::getWidth(){
    return this->width;
}
int64_t MaskImage::getHeight(){
    return this->height;
}
bool MaskImage::getPixel(int x,int y){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to get pixel from an empty MaskImage!";
        return false;
    }
    if(x<0||x>=this->width||y<0||y>=this->height){
        qWarning()<<"MaskImage: Try to get pixel out of range!";
        return false;
    }
    return this->accessBit(x, y);
}
bool MaskImage::setPixel(int x,int y, bool value){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to set pixel to an empty MaskImage!";
        return false;
    }
    if(x<0||x>=this->width||y<0||y>=this->height){
        qWarning()<<"MaskImage: Try to set pixel out of range!";
        return false;
    }
    this->setBit(x, y, value);
    return true;
}
int64_t MaskImage::getRawDataBufferSize(){
    return this->bufSize;
}
unsigned char* MaskImage::getRawData(){
    if(this->data==nullptr){
        qWarning()<<"MaskImage: Try to get raw data from an empty MaskImage!";
        return nullptr;
    }
    return this->data.data();
}
QImage MaskImage::cropToQImage(int64_t x, int64_t y, int64_t w, int64_t h, QImage::Format format){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to crop an empty MaskImage!";
        return QImage();
    }
    if(x<0||x>=this->width||y<0||y>=this->height||w<0||h<0||x+w>this->width||y+h>this->height){
        qWarning()<<"MaskImage: Try to crop out of range!";
        return QImage();
    }
    QImage image(this->data.data(),this->width, this->height, QImage::Format_Mono);
    return image.copy(x, y, w, h).convertToFormat(format);
}
MaskImage MaskImage::crop(int64_t x, int64_t y, int64_t w, int64_t h){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to crop an empty MaskImage!";
        return MaskImage();
    }
    if(x<0||x>=this->width||y<0||y>=this->height||w<0||h<0||x+w>this->width||y+h>this->height){
        qWarning()<<"MaskImage: Try to crop out of range!";
        return MaskImage();
    }
    MaskImage maskImage(w, h);
    for(int64_t i=0;i<w;i++){
        for(int64_t j=0;j<h;j++){
            maskImage.setPixel(i, j, this->accessBit(x+i, y+j));
        }
    }
    return maskImage;
}
QImage MaskImage::toQImage(QImage::Format format){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to convert an empty MaskImage to QImage!";
        return QImage();
    }
    return this->cropToQImage(0, 0, this->width, this->height, format);
}
cv::Mat MaskImage::cropToMat(int64_t x, int64_t y, int64_t w, int64_t h){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to crop an empty MaskImage!";
        return cv::Mat();
    }
    if(x<0||x>=this->width||y<0||y>=this->height||w<0||h<0||x+w>this->width||y+h>this->height){
        qWarning()<<"MaskImage: Try to crop out of range!";
        return cv::Mat();
    }
    cv::Mat mat(h, w, CV_8UC1);
    for(int64_t i=0;i<w;i++){
        for(int64_t j=0;j<h;j++){
            mat.at<uchar>(j, i) = this->accessBit(x+i, y+j)?255:0;
        }
    }
    return mat;
}
cv::Mat MaskImage::toMat(){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to convert an empty MaskImage to cv::Mat!";
        return cv::Mat();
    }
    return this->cropToMat(0, 0, this->width, this->height);
}
MaskImage MaskImage::resizeNearestAndCopy(int64_t w, int64_t h){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to resize an empty MaskImage!";
        return MaskImage();
    }
    MaskImage maskImage(w, h);
    for(int64_t i=0;i<w;i++){
        for(int64_t j=0;j<h;j++){
            maskImage.setPixel(i, j, this->accessBit(i*this->width/w, j*this->height/h));
        }
    }
    return maskImage;
}
MaskImage MaskImage::resizeNearestAndCopy(double scaleX, double scaleY){
    int64_t newX = this->width*scaleX, newY = this->height*scaleY;
    return this->resizeNearestAndCopy(newX, newY);
}
inline bool MaskImage::accessBit(int64_t x, int64_t y)
{
    int64_t position = y*this->width+x;
    int64_t byteIndex = position/8;
    int64_t bitIndex = position%8;
    return (this->data.data()[byteIndex]&(1<<bitIndex))!=0;
}
inline void MaskImage::setBit(int64_t x, int64_t y, bool bit){
    int64_t position = y*this->width+x;
    int64_t byteIndex = position/8;
    int64_t bitIndex = position%8;
    this->data.data()[byteIndex] = this->data.data()[byteIndex]&(~(1<<bitIndex));//clear target bit
    this->data.data()[byteIndex] = this->data.data()[byteIndex]|(bit<<bitIndex);//set target bit
}
void MaskImage::fill(bool value, int64_t x, int64_t y, int64_t w, int64_t h){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to fill an empty MaskImage!";
        return;
    }
    if(x<0||y<0||w<0||h<0||x>=this->width||y>=this->height||x+w>this->width||y+h>this->height){
        qWarning()<<"MaskImage: Try to fill out of range!";
        return;
    }
    for(int64_t i=0;i<w;i++){// TODO: optimize code here
        for(int64_t j=0;j<h;j++){
            this->setBit(x+i, y+j, value);
        }
    }
}
void MaskImage::fill(MaskImage& maskImage, int64_t x, int64_t y){
    if(this->bufSize==-1||this->data==nullptr){
        qWarning()<<"MaskImage: Try to fill an empty MaskImage!";
        return;
    }
    if(x<0||y<0||x>=this->width||y>=this->height||x+maskImage.width>this->width||y+maskImage.height>this->height){
        qWarning()<<"MaskImage: Try to fill out of range!";
        return;
    }
    for(int64_t i=0;i<maskImage.width;i++){// TODO: optimize code here
        for(int64_t j=0;j<maskImage.height;j++){
            this->setBit(x+i, y+j, maskImage.accessBit(i, j));
        }
    }
}
bool MaskImage::valid(){
    if((this->bufSize!=-1&&this->data!=nullptr)){
        return true;
    }
    qDebug()<<"MaskImage: strange, it seems we have encounterd a bug, this->bufSize="<<this->bufSize<<", this->data==nullptr:"<<(this->data==nullptr?"true":"false");
    return false;
}
