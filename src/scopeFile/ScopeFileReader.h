#ifndef  SCOPE_READER_H
#define  SCOPE_READER_H
#include <QString>
#include <QImage>
#include <QFile>
#include <utility>
#include <QSharedPointer>
#include "../scopeFile/MaskImage.h"
#include "../scopeFile/ScopeFileHeader.h"
#include "../scopeFile/ScopeFileMetaData.h"
#include "../scopeFile/ScopeFileUtil.h"
class ScopeFileReader{
public:
    ScopeFileReader(QString fileName);
    ScopeFileReader(QSharedPointer<QFile> file);
    ~ScopeFileReader();
    bool valid();
    ScopeFileHeader& getHeader();
    ScopeFileMetaData& getMetaData();
    bool getPixel(int64_t level, int64_t x,int64_t y);
    MaskImage readRegion(int64_t level, int64_t x, int64_t y,int64_t w, int64_t h);
    
private:
    QSharedPointer<QFile> file;
    ScopeFileHeader header;
    ScopeFileMetaData metaData;
    QDataStream in;
    bool validFile = false;
    bool shouldDeleteFilePointer = false;
    MaskImage readPatch(int64_t level, int64_t patchSerialNumber);
};
#endif