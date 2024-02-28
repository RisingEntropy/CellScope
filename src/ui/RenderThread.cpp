#include "RenderThread.h"
#include <QDebug>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include "../scopeFile/MaskImage.h"
#include <QGraphicsPixmapItem>
RenderThread::RenderThread(int tileSize){
    this->tileSize = tileSize;
    this->moveToThread(&thread);
    connect(this,&RenderThread::internalRequestRegion,this,&RenderThread::_requestRegion);

    connect(this,&RenderThread::internalInstallTiledImage,this,&RenderThread::_installTiledImage);
    connect(this,&RenderThread::internalInstallTiledImagePointer,this,&RenderThread::_installTiledImagePointer);
    connect(this,&RenderThread::internalUninstallTiledImage,this,&RenderThread::_uninstallTiledImage);

    connect(this,&RenderThread::internalInstallScopeFile,this,&RenderThread::_installScopeFile);
    connect(this,&RenderThread::internalInstallScopeFilePointer,this,&RenderThread::_installScopeFilePointer);
    connect(this,&RenderThread::internalUninstallScopeFile,this,&RenderThread::_uninstallScopeFile);

    connect(this,&RenderThread::internalCountCellNums,this,&RenderThread::_countCellNums);
    connect(this,&RenderThread::internalCountCellSize,this,&RenderThread::_countCellSize);
    connect(this,&RenderThread::internalRenderMask,this,&RenderThread::_renderMask);
    thread.start();
    while(!thread.isRunning());
    qDebug()<<"RenderThread started";
}
void RenderThread::requestRegion(int64_t level, QRectF FOVImage){

    emit internalRequestRegion(level,FOVImage);
}
RenderThread::~RenderThread()
{
    thread.quit();
    thread.wait();
    qDebug()<<"RenderThread deconstructed";
}

void RenderThread::stop(){
    thread.quit();
    thread.wait();
    qDebug()<<"RenderThread stopped";
}

void RenderThread::installTiledImage(QString path){
    emit internalInstallTiledImage(path);
}

void RenderThread::installTiledImage(QSharedPointer<OpenSlideFileReader> reader){
    emit internalInstallTiledImagePointer(reader);
}

void RenderThread::uninstallTiledImage(){
    emit internalUninstallTiledImage();
}

void RenderThread::installScopeFile(QString path){
    emit internalInstallScopeFile(path);
}

void RenderThread::installScopeFile(QSharedPointer<ScopeFileReader> reader){
    emit internalInstallScopeFilePointer(reader);
}

void RenderThread::uninstallScopeFile(){
    emit internalUninstallScopeFile();
}

void RenderThread::countCellNums(){
    emit internalCountCellNums();
}

void RenderThread::countCellSize(){
    emit internalCountCellSize();
}

void RenderThread::renderMask(bool renderMaskFlag){
    emit internalRenderMask(renderMaskFlag);
}




void RenderThread::_requestRegion(int64_t level, QRectF FOVImage){
    if(this->tiledFileReader.isNull()){
        qDebug()<<"tileFileReader is null, cannot render shift.";
        return;
    }

    int64_t levelWidth = this->tiledFileReader->getLevelWidth(level);
    int64_t levelHeight = this->tiledFileReader->getLevelHeight(level);

    auto overlap = [](int64_t x, int64_t y, int64_t w, int64_t h, QRectF FOV){
        return (x+w > FOV.x() && x < FOV.x()+FOV.width() && y+h > FOV.y() && y < FOV.y()+FOV.height());
    };
    auto maxMultiple = [](int64_t x, int64_t y){return x / y * y;};
    auto levelScale = this->tiledFileReader->getScaleBetweenLevels(0, level);
    for(auto &x:this->cache[level].keys()){
        if(!overlap(std::get<0>(x)*levelScale,std::get<1>(x)*levelScale,this->tileSize*levelScale,this->tileSize*levelScale,FOVImage)){
            emit removeTile(this->cache[level][x]);
            this->cache[level].remove(x);
            this->sizeCache[level].remove(x);
        }
    }
    for(int64_t i = 0;i<this->tiledFileReader->getLevelCount();i++){
        if(i!=level){
            for(auto &x:this->cache[i].keys()){
                if(overlap(std::get<0>(x),std::get<1>(x),this->tileSize,this->tileSize,FOVImage)){
                    emit removeTile(this->cache[i][x]);
                    this->cache[i].remove(x);
                    this->sizeCache[i].remove(x);
                }
            }
        }
    }
    int64_t xInLevel = FOVImage.x() / this->tiledFileReader->getScaleBetweenLevels(0, level);
    int64_t yInLevel = FOVImage.y() / this->tiledFileReader->getScaleBetweenLevels(0, level);
    int64_t widthInLevel = FOVImage.width() / this->tiledFileReader->getScaleBetweenLevels(0, level);
    int64_t heightInLevel = FOVImage.height() / this->tiledFileReader->getScaleBetweenLevels(0, level);
    int64_t cellSize = 0;
    for(int64_t x = maxMultiple(xInLevel,this->tileSize);x <=xInLevel+widthInLevel;x+=this->tileSize){
        for(int64_t y = maxMultiple(yInLevel,this->tileSize);y <=yInLevel+heightInLevel;y+=this->tileSize){
            int64_t actualX = x*this->tiledFileReader->getScaleBetweenLevels(0, level);
            int64_t actualY = y*this->tiledFileReader->getScaleBetweenLevels(0, level);
            auto key = std::make_tuple(x,y,level);
            if(this->cache[level].contains(key)){
                cellSize+=this->sizeCache[level][key];
                continue;
            }
            if(x>=levelWidth || y>=levelHeight){
                continue;
            }
            auto img = this->tiledFileReader->getRegionMat(level,actualX,actualY,qMin(this->tileSize*1LL,levelWidth-1-x+1),qMin(this->tileSize*1LL,levelHeight-1-y+1));
            if(img.empty()){
                continue;
            }
            int64_t sze = 0;
            if(this->renderMaskFlag){
                auto maskImage = this->scopeFileReader->readRegion(level, x,y,
                                                                    qMin(this->tileSize*1LL,levelWidth-1-x+1),
                                                                    qMin(this->tileSize*1LL,levelHeight-1-y+1));
                if(!maskImage.valid()){
                    qDebug()<<"maskImage is invalid. x:"<<x<<",y:"<<y<<",tileSize"<< this->tileSize<<",level:"<<level<<" levelWidth:"<<levelWidth<<" levelHeight:"<<levelHeight;
                    continue;
                }
                cv::Mat mask = maskImage.toMat();
                if(!mask.empty()){
                    sze = cv::sum(mask)[0]/255;
                    cv::cvtColor(mask,mask, cv::COLOR_GRAY2RGB);
                    cv::addWeighted(img,(1-this->maskWeight),mask,this->maskWeight,0,img);
                }else{
                    qDebug()<<"mask is empty";
                }
            }
            cellSize+=sze*levelScale*levelScale;
            this->sizeCache[level][key] = sze*levelScale*levelScale;
            auto item = QSharedPointer<QGraphicsPixmapItem>(new QGraphicsPixmapItem(QPixmap::fromImage(QImage(img.data,img.cols,img.rows,img.step,QImage::Format_RGB888))));
            item->setPos(x*this->tiledFileReader->getScaleBetweenLevels(0, level),y*this->tiledFileReader->getScaleBetweenLevels(0, level));
            item->setScale(this->tiledFileReader->getScaleBetweenLevels(0, level));
            
            this->cache[level][key] = item;
            emit addTile(item);
        }
    }
    if(this->renderMaskFlag){
        emit updateCurrentViewCellSize(cellSize);
    }
    
}

void RenderThread::_installTiledImage(QString path){
    this->tiledFileReader.reset(new OpenSlideFileReader(path));
    this->cache.clear();
    for(int64_t i = 0;i<this->tiledFileReader->getLevelCount();i++){
        this->cache.push_back(QMap<std::tuple<int64_t, int64_t, int64_t>, QSharedPointer<QGraphicsPixmapItem> >());
        this->sizeCache.push_back(QMap<std::tuple<int64_t, int64_t, int64_t>, int64_t>());
    }
    this->tiledFileReaderInstalled = true;

}
void RenderThread::_installTiledImagePointer(QSharedPointer<OpenSlideFileReader> reader){
    this->tiledFileReader = reader;
    this->cache.clear();
    for(int64_t i = 0;i<this->tiledFileReader->getLevelCount();i++){
        this->cache.push_back(QMap<std::tuple<int64_t, int64_t, int64_t>, QSharedPointer<QGraphicsPixmapItem> >());
        this->sizeCache.push_back(QMap<std::tuple<int64_t, int64_t, int64_t>, int64_t>());
    }
    this->tiledFileReaderInstalled = true;
}
void RenderThread::_uninstallTiledImage(){
    if(!this->tiledFileReader.isNull()){
        for(int64_t i = 0;i<this->tiledFileReader->getLevelCount();i++){
            for(auto &x:this->cache[i].keys()){
                emit removeTile(this->cache[i][x]);
                this->cache[i].remove(x);
                this->sizeCache[i].remove(x);
            }
        }
        this->tiledFileReader.reset();
    }
    this->tiledFileReaderInstalled = false;
}
void RenderThread::_installScopeFile(QString path){
    this->scopeFileReader.reset(new ScopeFileReader(path));
    this->scopeFileReaderInstalled = true;
    emit updateScopeFileMetaData(this->scopeFileReader->getMetaData());

}
void RenderThread::_installScopeFilePointer(QSharedPointer<ScopeFileReader> reader){
    this->scopeFileReader = reader;
    this->scopeFileReaderInstalled = true;
    emit updateScopeFileMetaData(this->scopeFileReader->getMetaData());
}
void RenderThread::_uninstallScopeFile()
{
    this->scopeFileReaderInstalled = false;
    this->scopeFileReader.reset();
}
void RenderThread::_countCellNums(){
    int64_t count = 0;
    emit updateCurrentViewCellNums(count);
}
void RenderThread::_countCellSize(){
    int64_t size = 0;
    emit updateCurrentViewCellSize(size);
}

void RenderThread::_renderMask(bool renderMaskFlag){
    this->renderMaskFlag = renderMaskFlag;
    if(this->tiledFileReader.isNull()){
        return;
    }
    for(int64_t i = 0;i<this->tiledFileReader->getLevelCount();i++){
        for(auto &x:this->cache[i].keys()){
            emit removeTile(this->cache[i][x]);
            this->cache[i].remove(x);
            this->sizeCache[i].remove(x);
        }
    }
}
