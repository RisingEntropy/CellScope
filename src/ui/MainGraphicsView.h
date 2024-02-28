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
    
    void setScopeFile(QString filename);
    void setOpenSlideFile(QString filename);
    void setMaskEnabled(bool);
signals:
    void updateFOVSize(int64_t FOVSize);
    void updateCellSize(int64_t CellSize);
    void updateTotalSize(int64_t TotalSize);
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent * event ) override;
private slots:
    void onCellSizeUpdate(int64_t size);
private:
    void initShow();
    void addNewItem(QSharedPointer<QGraphicsItem> item);
    void removeItem(QSharedPointer<QGraphicsItem> item);

    void zoom(int numDegrees);
    void FOVChanged(QRectF FOV);
    void pan(const QPoint& panTo);
    int64_t currentViewSize = 0;
    RenderThread renderThread;
    bool mousePress = false;
    QPointF mousePos;
    QGraphicsScene *scene;
    QSharedPointer<OpenSlideFileReader> reader = nullptr;
    QPointF previousPan;
    QPointF zoomToScenePos;
    QPointF zoomToViewPos;

};


#endif