#ifndef OPEN_SLIDE_FILE_READER_H
#define OPEN_SLIDE_FILE_READER_H
#include "qvector.h"
#include <openslide/openslide.h>
#include <QImage>
#include <QVector>
#include <stdint.h>
#include <opencv2/opencv.hpp>
class OpenSlideFileReader{
public:
    OpenSlideFileReader(QString fileName);
    ~OpenSlideFileReader();
    static bool validateFile(QString fileName);
    bool validFile();
    void releaseAllocatedSlides();
    void releasePreviousSlides(int count);
    int32_t getLevelCount();
    int64_t getLevelWidth(int32_t level);
    int64_t getLevelHeight(int32_t level);
    double getLevelDownsample(int32_t level);
    double getScaleBetweenLevels(int32_t level1, int32_t level2);
    int32_t getBestLevelForDownsample(double downsample);
    cv::Mat getRegionMat(int32_t level, int64_t x, int64_t y, int64_t w=-1, int64_t h=-1, int64_t targetWidth=-1, int64_t targetHeight=-1);

private:
    openslide_t *file;
    bool valid = false;
    uint32_t* getRegionRaw(int32_t level, int64_t x, int64_t y, int64_t w=-1, int64_t h=-1);
};
#endif