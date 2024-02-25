#ifndef MASKIMAGE_H
#define MASKIMAGE_H
#include "qcolor.h"
#include "qimage.h"
#include "qnamespace.h"
#include <stdint.h>
#include <utility>
#include <QImage>
#include <QSharedPointer>
#include <opencv2/opencv.hpp>
class MaskImage{
public:
    
    MaskImage(int64_t width, int64_t height);
    MaskImage(unsigned char* data, int64_t width, int64_t height, bool copy);
    MaskImage(const MaskImage& maskImage);
    MaskImage& operator=(const MaskImage& maskImage);
    static MaskImage fromMat(cv::Mat &mask);
    int64_t getWidth();
    int64_t getHeight();
    MaskImage crop(int64_t x, int64_t y, int64_t w, int64_t h);
    void fill(bool value, int64_t x, int64_t y, int64_t w, int64_t h);
    void fill(MaskImage& maskImage, int64_t x, int64_t y);
    QImage cropToQImage(int64_t x, int64_t y, int64_t w, int64_t h, QImage::Format format=QImage::Format_ARGB32);
    QImage toQImage(QImage::Format format=QImage::Format_ARGB32);
    cv::Mat cropToMat(int64_t x, int64_t y, int64_t w, int64_t h);
    cv::Mat toMat();
    MaskImage resizeNearestAndCopy(int64_t w, int64_t h);
    MaskImage resizeNearestAndCopy(double scaleX, double scaleY);
    bool getPixel(int x,int y);
    bool setPixel(int x,int y,bool value);
    unsigned char* getRawData();
    int64_t getRawDataBufferSize();
    template<bool BIT>
    int64_t bitCount();
    bool valid();
    ~MaskImage();
private:
    MaskImage(); // do not allow creating an instance outside of the class
    int64_t width;
    int64_t height;
    int64_t bufSize;
    QSharedPointer<unsigned char> data;
    inline bool accessBit(int64_t x, int64_t y);
    inline void setBit(int64_t x, int64_t y, bool bit);
};
template <bool BIT>
inline int64_t MaskImage::bitCount(){
    int64_t count = 0;
    for(int64_t x = 0; x < width; x++){
        for(int64_t y = 0; y < height; y++){
            if(accessBit(x,y)==BIT){
                count++;
            }
        }
    }
    return count;
}

#endif

