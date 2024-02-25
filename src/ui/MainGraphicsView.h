#ifndef MAINGRAPHICSVIEW_H
#define MAINGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include "RenderThread.h"
#include <QSharedPointer>
#include <QImage>
#include "../io/OpenSlideFileReader.h"
class MainGraphicsView : public QGraphicsView{
    Q_OBJECT
public:
    MainGraphicsView(QWidget *parent = 0);
    ~MainGraphicsView();
    void setRenderThread(QSharedPointer<RenderThread> renderThread);
    void setReader(QSharedPointer<OpenSlideFileReader> reader);
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
private:
    void scaleScene();
    void initShow();
    void updateContent(QGraphicsPixmapItem*);
    bool mousePress = false;
    void zoom(float numSteps);
    void FOVChanged();
    // void scaleingTime(qreal x);
    // void zoomFinished();
    int scheduledZoom = 0;
    float sceneScale = 1.0;
    QPointF mousePos;
    QGraphicsScene *scene;
    QSharedPointer<RenderThread> renderThread;
    QSharedPointer<OpenSlideFileReader> reader;
    void pan(const QPoint& panTo);
    QPointF previousPan;
    QPointF zoomToScenePos;
    QPointF zoomToViewPos;
};


#endif