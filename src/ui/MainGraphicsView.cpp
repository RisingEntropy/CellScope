#include "MainGraphicsView.h"
#include <QDebug>
#include <QScrollBar>
#include <QRectF>
#include <QImage>
#include <QTimeLine>
MainGraphicsView::MainGraphicsView(QWidget *parent):QGraphicsView(parent){
    // setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorViewCenter);
    setDragMode(QGraphicsView::DragMode::NoDrag);
    setContentsMargins(0,0,0,0);
    setAutoFillBackground(true);
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    setInteractive(true);
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    this->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    this->scene = new QGraphicsScene();
    this->scene->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    setScene(this->scene);
    QImage img;
    img.load("./R.png");

    this->scene->addItem(new QGraphicsPixmapItem(QPixmap::fromImage(img)));

}

MainGraphicsView::~MainGraphicsView(){
    delete this->scene;
}

void MainGraphicsView::setRenderThread(QSharedPointer<RenderThread> renderThread){
    this->renderThread = renderThread;
}

void MainGraphicsView::setReader(QSharedPointer<OpenSlideFileReader> reader){
    this->reader = reader;
}


void MainGraphicsView::mousePressEvent(QMouseEvent *event){
    this->mousePress = true;
    this->mousePos = event->pos();
    setCursor(Qt::ClosedHandCursor);
    event->ignore();
    previousPan = event->pos();
}

void MainGraphicsView::mouseReleaseEvent(QMouseEvent *event){
    this->mousePress = false;
    this->mousePos = event->pos();
    setCursor(Qt::ArrowCursor);
    event->ignore();
    previousPan = event->pos();
}

void MainGraphicsView::mouseMoveEvent(QMouseEvent *event){// only when mouse pressed and move will this function invoked
    this->mousePos = event->pos();
    if(mousePress){
        pan(event->pos());
    }
    previousPan = event->pos();
    QGraphicsView::mouseMoveEvent(event);
}


void MainGraphicsView::wheelEvent(QWheelEvent *event){
    int numDegrees = event->angleDelta().y();
    int numSteps = numDegrees / 15;
    zoomToScenePos = mapToScene(event->position().toPoint());
    zoomToViewPos = event->position();
    zoom(numSteps);
}
void MainGraphicsView::scaleScene(){
    
    
}

void MainGraphicsView::initShow(){
    // setEnabled(true);
    // setMouseTracking(true);
    // scene->clear();
    // scene->addPixmap(QPixmap::fromImage(this->img));
    // scene->update();
    // this->resetTransform();
    // this->setSceneRect(img.rect());
    // this->fitInView(QRect(0, 0, img.width(), img.height()), Qt::KeepAspectRatio);


}

void MainGraphicsView::updateContent(QGraphicsPixmapItem* item){

}

void MainGraphicsView::zoom(float numSteps){
    if(this->reader.isNull()||this->renderThread.isNull()){
        return;
    }
    this->scheduledZoom += numSteps;
    if(this->scheduledZoom*numSteps<0){//inverse zoom
        this->scheduledZoom = numSteps;
    }
    float factor = numSteps>0?1.1:0.9;
    // if(factor*this->transform().m11()>1.0||factor*this->transform().m11()<1/this->reader->getScaleBetweenLevels(0,this->reader->getLevelCount()-1)){
    //     factor = 1.0;
    // }

    this->scale(factor, factor);


    // centerOn(this->zoomToScenePos);
    // QPointF deltaViewPortPos = this->zoomToViewPos - QPointF(width()/2., height()/2.);
    // QPointF viewPortCenter = mapFromScene(this->zoomToScenePos) - deltaViewPortPos;
    // centerOn(mapToScene(viewPortCenter.toPoint()));

    FOVChanged();
    
}

void MainGraphicsView::FOVChanged(){
    QRectF FOV = this->mapToScene(this->rect()).boundingRect();
    int64_t bestLevel = this->reader->getBestLevelForDownsample(1.0/this->transform().m11());
    qreal x = FOV.x()/this->transform().m11();
    qreal y = FOV.y()/this->transform().m11();
    qreal w = FOV.width()/this->transform().m11();
    qreal h = FOV.height()/this->transform().m11();
    cv::Mat mat = this->reader->getRegionMat(bestLevel,qMax(0.,x),qMax(0.,y),qMin(w,this->reader->getLevelWidth(bestLevel)-1-x+1),qMin(h,this->reader->getLevelHeight(bestLevel)-1-y+1));
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(QImage(mat.data,mat.cols,mat.rows,mat.step,QImage::Format_RGB888)));
    item->setScale(1./this->transform().m11());
    item->setPos(x,y);
    this->scene->clear();
    this->scene->addItem(item);

}

// void MainGraphicsView::scaleingTime(qreal x){
//     qreal factor = 1.0 + qreal(this->scheduledZoom) * x/300.;
//     float maxDownsapmple = 1./this->sceneScale;
//     QRectF FOV = this->mapToScene(this->rect()).boundingRect();
//     QRectF FOVImage = QRectF(FOV.left() / this->sceneScale, FOV.top() / this->sceneScale, FOV.width() / this->sceneScale, FOV.height() / this->sceneScale);
//     float scaleX = static_cast<float>(this->reader->getLevelWidth(0))/ FOVImage.width();
//     float scaleY = static_cast<float>(this->reader->getLevelHeight(0))/ FOVImage.height();
//     float minScale = qMin(scaleX, scaleY);
//     float maxScale = qMax(scaleX, scaleY);
//     if((factor<1.0&&maxScale<0.5)||(factor>1.0&&minScale>2.0*maxDownsapmple)){
//         return;
//     }
//     scale(factor, factor);
//     centerOn(this->zoomToScenePos);
//     QPointF deltaViewPortPos = this->zoomToViewPos - QPointF(width()/2., height()/2.);
//     QPointF viewPortCenter = mapFromScene(this->zoomToScenePos) - deltaViewPortPos;
//     centerOn(mapToScene(viewPortCenter.toPoint()));
//     qDebug()<<FOVImage;
// }

// void MainGraphicsView::zoomFinished(){
//     if(this->scheduledZoom > 0){
//         scheduledZoom--;
//     }else{
//         scheduledZoom++;
//     }
//     sender()->deleteLater();
// }

void MainGraphicsView::pan(const QPoint &panTo){
    QScrollBar *hBar = horizontalScrollBar();
    QScrollBar *vBar = verticalScrollBar();
    QPointF delta = panTo - previousPan;
    previousPan = panTo;
    hBar->setValue(hBar->value() - delta.x()/this->transform().m11());
    vBar->setValue(vBar->value() - delta.y()/this->transform().m11());

    FOVChanged();
}
