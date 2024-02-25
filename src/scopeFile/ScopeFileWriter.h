#ifndef SCOPEFILEWRITER_H
#define SCOPEFILEWRITER_H
#include <QFile>
#include <QVector>
#include <stdint.h>
#include <utility>
#include "MaskImage.h"
#include "ScopeFileMetaData.h"
#include "ScopeFileHeader.h"
#include "qvector.h"
class ScopeFileWriter{
public:
    ScopeFileWriter(QString& filename);
    ScopeFileWriter(QFile* file);
    ~ScopeFileWriter();
    ScopeFileHeader& getHeader();
    bool addLevel(int64_t width, int64_t height, unsigned char compressLevel);
    bool addPatch(int64_t level, int64_t x, int64_t y, MaskImage& mask);
    bool finish();
    void flush();
    bool close();
    int64_t getCurrentLevels();
private:
    QFile *file;
    ScopeFileHeader header;
    bool finished = false;
    bool shouldDeleteFilePointer = false;
    uint64_t currentPosition;
    QVector<int64_t> levelSizes;
    QVector<ScopeFileHeader::LevelInfo*> levelInfos;
};

#endif