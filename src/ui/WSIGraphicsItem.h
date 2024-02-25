#ifndef WSIGRAPHICSITEM_H
#define WSIGRAPHICSITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QRectF>
#include "../io/OpenSlideFileReader.h"
class WSIGraphicsItem: public QGraphicsItem{
public:
    WSIGraphicsItem(OpenSlideFileReader* reader, int64_t level, int64_t x, int64_t y, int64_t width, int64_t height);
protected:
    virtual QRectF boundingRect()const override;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    int64_t level;
    int64_t x;
    int64_t y;
    int64_t width;
    int64_t height;
    OpenSlideFileReader* reader;
    QPixmap pixmap;
    bool loaded = false;
};

#endif