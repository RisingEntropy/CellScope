#ifndef SCOPEFILEHEADER_H
#define SCOPEFILEHEADER_H
#include "qvector.h"
#include <QVector>
#include <stdint.h>
#include "ScopeFileMetaData.h"
// #include "../utils/ScopeFileUtil.h"
class ScopeFileHeader{
public:
    static const int32_t MAGIC_NUMBER = 0x037A;
    static const int32_t CURRENT_VERSION = 2;
    class LevelInfo{
        public:
            unsigned char compressLevel=255;
            int64_t width=-1;
            int64_t height=-1;
            int64_t patchNum=-1;
            QVector<uint64_t> patchDataOffsets;
            QVector<int64_t> patchWidths;
            QVector<int64_t> patchHeights;
            QVector<int64_t> patchXs;
            QVector<int64_t> patchYs;
    };
    int magicNumber = MAGIC_NUMBER;
    int version = CURRENT_VERSION;
    ScopeFileMetaData metaData;
    int64_t levels = 0;
    bool valid = false;
    const LevelInfo& getLevelInfo(int64_t level) const;
    void addLevelInfo(LevelInfo levelInfo);
private:
    QVector<LevelInfo> levelInfos;
};
#endif