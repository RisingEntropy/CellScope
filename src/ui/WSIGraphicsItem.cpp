#include "WSIGraphicsItem.h"

WSIGraphicsItem::WSIGraphicsItem(OpenSlideFileReader *reader, int64_t level, int64_t x, int64_t y, int64_t width, int64_t height):
    reader(reader), level(level), x(x), y(y), width(width), height(height){

}

QRectF WSIGraphicsItem::boundingRect() const{
    return QRectF();
}

void WSIGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
}
