#include "RenderThread.h"
#include <QDebug>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <QGraphicsPixmapItem>
RenderThread::RenderThread(int tileSize){
    this->tileSize = tileSize;
    this->moveToThread(&thread);
    connect(this,&RenderThread::internalRequestRegion,this,&RenderThread::_requestRegion);
    connect(this,&RenderThread::internalInstallTiledImage,this,&RenderThread::_installTiledImage);
    connect(this,&RenderThread::internalUninstallTiledImage,this,&RenderThread::_uninstallTiledImage);
    connect(this,&RenderThread::internalInstallScopeFile,this,&RenderThread::_installScopeFile);
    connect(this,&RenderThread::internalUninstallScopeFile,this,&RenderThread::_uninstallScopeFile);
    connect(this,&RenderThread::internalCountCellNums,this,&RenderThread::_countCellNums);
    connect(this,&RenderThread::internalCountCellSize,this,&RenderThread::_countCellSize);
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

void RenderThread::installTiledImage(QString path){
    emit internalInstallTiledImage(path);
}

void RenderThread::uninstallTiledImage(){
    emit internalUninstallTiledImage();
}

void RenderThread::installScopeFile(QString path){
    emit internalInstallScopeFile(path);
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
    int64_t x = FOVImage.left();
    int64_t y = FOVImage.top();
    int64_t width = FOVImage.width();
    int64_t height = FOVImage.height();
    cv::Mat region = this->tiledFileReader->getRegionMat(level,qMax(0LL,x),qMax(0LL,y),qMin(this->tiledFileReader->getLevelWidth(level)-x,width),qMin(this->tiledFileReader->getLevelHeight(level)-y,height));
    QImage image(region.data, region.cols, region.rows, region.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image);
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    item->setPos(qMax(0LL,x),qMax(0LL,y));
    emit addTile(item);

}

void RenderThread::_installTiledImage(QString path){
    this->tiledFileReader.reset(new OpenSlideFileReader(path));
    this->tiledFileReaderInstalled = true;
}
void RenderThread::_uninstallTiledImage(){
    this->tiledFileReader.reset();
    this->tiledFileReaderInstalled = false;
}
void RenderThread::_installScopeFile(QString path){
    this->scopeFileReader.reset(new ScopeFileReader(path));
    this->scopeFileReaderInstalled = true;
    emit updateScopeFileMetaData(this->scopeFileReader->getMetaData());

}
void RenderThread::_uninstallScopeFile(){
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
}
