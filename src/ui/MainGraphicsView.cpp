#include "MainGraphicsView.h"
#include <QDebug>
#include <QScrollBar>
#include <QRectF>
#include <QImage>
#include <QTimeLine>
#include "../GlobalResources.h"
MainGraphicsView::MainGraphicsView(QWidget *parent):QGraphicsView(parent){
    qRegisterMetaType<QSharedPointer<QGraphicsPixmapItem>>("QSharedPointer<QGraphicsPixmapItem>");
    qRegisterMetaType<QSharedPointer<OpenSlideFileReader>>("QSharedPointer<OpenSlideFileReader>");

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setDragMode(QGraphicsView::DragMode::NoDrag);
    setContentsMargins(0,0,0,0);
    setAutoFillBackground(true);
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    setInteractive(true);
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    this->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    this->scene = new QGraphicsScene();
    this->scene->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    setScene(this->scene);

    connect(&this->renderThread, &RenderThread::addTile, this, &MainGraphicsView::addNewItem);
    connect(&this->renderThread, &RenderThread::removeTile, this, &MainGraphicsView::removeItem);
    connect(&this->renderThread, &RenderThread::updateCurrentViewCellSize, this, &MainGraphicsView::onCellSizeUpdate);
}

MainGraphicsView::~MainGraphicsView(){
    this->renderThread.stop();
}



void MainGraphicsView::setScopeFile(QString filename){
    this->renderThread.installScopeFile(filename);
    
}

void MainGraphicsView::setOpenSlideFile(QString filename){
    this->renderThread.uninstallTiledImage();
    this->reader.reset(new OpenSlideFileReader(filename));
    this->renderThread.installTiledImage(this->reader);
    initShow();
    emit updateTotalSize(this->reader->getLevelWidth(0)*this->reader->getLevelHeight(0));
}

void MainGraphicsView::setMaskEnabled(bool mask){
    this->renderThread.renderMask(mask);
    FOVChanged(this->mapToScene(rect()).boundingRect());
}

void MainGraphicsView::mousePressEvent(QMouseEvent *event){
    this->mousePress = true;
    this->mousePos = event->pos();
    setCursor(Qt::ClosedHandCursor);
    previousPan = event->pos();
    QGraphicsView::mousePressEvent(event);
}

void MainGraphicsView::mouseReleaseEvent(QMouseEvent *event){
    this->mousePress = false;
    this->mousePos = event->pos();
    setCursor(Qt::ArrowCursor);
    previousPan = event->pos();
    QGraphicsView::mouseReleaseEvent(event);
}

void MainGraphicsView::mouseMoveEvent(QMouseEvent *event){
    this->mousePos = event->pos();
    if(mousePress){
        pan(event->pos());
    }
    previousPan = event->pos();
    QGraphicsView::mouseMoveEvent(event);

}

void MainGraphicsView::resizeEvent(QResizeEvent *event){
    QRect rect = QRect(QPoint(0, 0), event->size());
    QRectF FOV = this->mapToScene(rect).boundingRect();
    QGraphicsView::resizeEvent(event);
    FOVChanged(FOV);
}

void MainGraphicsView::wheelEvent(QWheelEvent *event){
    int numDegrees = event->angleDelta().y();
    zoomToScenePos = mapToScene(event->position().toPoint());
    zoomToViewPos = event->position();
    zoom(numDegrees);
}


void MainGraphicsView::initShow(){
    if(this->reader.isNull()){
        return;
    }
    this->setSceneRect(QRectF(0,0,this->reader->getLevelWidth(0),this->reader->getLevelHeight(0)));
    double currentScale = this->transform().m11();
    this->scale(globalSettings.getDoubleValue("minZoom")/currentScale,globalSettings.getDoubleValue("minZoom")/currentScale);
    FOVChanged(this->mapToScene(rect()).boundingRect());
}

void MainGraphicsView::addNewItem(QSharedPointer<QGraphicsItem> item){
    this->currentViewSize+=item->boundingRect().width()*item->boundingRect().height()*item->scale()*item->scale();
    this->scene->addItem(item.data());
    QRectF FOV = this->mapToScene(rect()).boundingRect();
    emit updateFOVSize(qMin(this->currentViewSize, int64_t(FOV.width()*FOV.height())));
}

void MainGraphicsView::removeItem(QSharedPointer<QGraphicsItem> item){
    this->currentViewSize-=item->boundingRect().width()*item->boundingRect().height()*item->scale()*item->scale();
    this->scene->removeItem(item.data());
    QRectF FOV = this->mapToScene(rect()).boundingRect();
    emit updateFOVSize(qMin(this->currentViewSize, int64_t(FOV.width()*FOV.height())));
}

void MainGraphicsView::zoom(int numDegrees){
    if(this->reader.isNull()){
        return;
    }
    double factor = 1.0;
    if(numDegrees>0){
        factor = 1.1;
    }else{
        factor = 0.9;
    }
    double m11 = this->transform().m11();
    if(m11*factor > globalSettings.getDoubleValue("maxZoom")){
        return;
    }else if(m11*factor < globalSettings.getDoubleValue("minZoom")){
        return;
    }

    this->scale(factor, factor);
    centerOn(this->zoomToScenePos);
    QPointF deltaViewPortPos = this->zoomToViewPos - QPointF(width()/2., height()/2.);
    QPointF viewPortCenter = mapFromScene(this->zoomToScenePos) - deltaViewPortPos;
    centerOn(mapToScene(viewPortCenter.toPoint()));

    QRectF FOV = this->mapToScene(rect()).boundingRect();
    FOVChanged(FOV);
}
void MainGraphicsView::pan(const QPoint &panTo){
    if(this->reader.isNull()){
        return;
    }
    QScrollBar *hBar = horizontalScrollBar();
    QScrollBar *vBar = verticalScrollBar();
    QPointF delta = panTo - previousPan;
    previousPan = panTo;
    hBar->setValue(hBar->value() - delta.x());
    vBar->setValue(vBar->value() - delta.y());
    QRectF FOV = this->mapToScene(rect()).boundingRect();
    FOVChanged(FOV);
}
void MainGraphicsView::FOVChanged(QRectF FOV){
    if(this->reader.isNull()){
        return;
    }
    int64_t level = this->reader->getBestLevelForDownsample(1.0/this->transform().m11());
    this->renderThread.requestRegion(level, FOV);
    emit updateFOVSize(qMin(this->currentViewSize, int64_t(FOV.width()*FOV.height())));
}

void MainGraphicsView::onCellSizeUpdate(int64_t size){
    emit updateCellSize(size);
}