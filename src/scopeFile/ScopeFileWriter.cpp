#include "ScopeFileWriter.h"
#include "QtZlib/zconf.h"
#include "qdebug.h"
#include <QDataStream>
#include <cstddef>
#include <stdint.h>
#include <QDebug>
#include <QtZlib/zlib.h>
#include "ScopeFileUtil.h"
ScopeFileWriter::ScopeFileWriter(QString& filename){
    this->shouldDeleteFilePointer = true;
    this->file = new QFile(filename);
    this->header.valid = true;
    file->open(QIODevice::WriteOnly);
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    int64_t headerOffsetPlaceholder = 0, headerLengthPlaceholder = 0, levels_1Placeholder = 0;
    stream<<ScopeFileHeader::MAGIC_NUMBER<<ScopeFileHeader::CURRENT_VERSION<<headerOffsetPlaceholder<<headerLengthPlaceholder<<levels_1Placeholder;
    this->currentPosition = file->pos();
}
ScopeFileWriter::ScopeFileWriter(QFile* file){
    shouldDeleteFilePointer = false;
    this->file = file;
    this->header.valid = true;
    if(!this->file->isOpen()){
        file->open(QIODevice::WriteOnly);
    }
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    int64_t headerOffsetPlaceholder = 0, headerLengthPlaceholder = 0, levels_1Placeholder = 0;
    stream<<ScopeFileHeader::MAGIC_NUMBER<<ScopeFileHeader::CURRENT_VERSION<<headerOffsetPlaceholder<<headerLengthPlaceholder<<levels_1Placeholder;
    this->currentPosition = file->pos();
}
ScopeFileWriter::~ScopeFileWriter(){
    if(!this->finished){
        finish();
    }
    if(shouldDeleteFilePointer)delete file;
}
ScopeFileHeader &ScopeFileWriter::getHeader(){
    return this->header;
}

bool ScopeFileWriter::addLevel(int64_t width, int64_t height, unsigned char compressLevel){
    if(width<0||height<0||compressLevel>9){
        qWarning()<<"ScopeFileWriter: Invalid parameters when adding a new level";
        return false;
    }
    ScopeFileHeader::LevelInfo* levelInfo = new ScopeFileHeader::LevelInfo();
    levelInfo->width = width;
    levelInfo->height = height;
    levelInfo->compressLevel = compressLevel;
    levelInfo->patchNum = 0;
    levelInfos.push_back(levelInfo);
    this->header.levels = levelInfos.size();
    this->levelSizes.append(0);
    return true;
}
bool ScopeFileWriter::addPatch(int64_t level, int64_t x, int64_t y, MaskImage& mask){
    if(level<0||level>=header.levels){
        qWarning()<<"ScopeFileWriter: Invalid level "<<level<<" when adding a new patch, maximum level: "<<this->header.levels-1;
        return false;
    }
    if(!mask.valid()){
        qWarning()<<"ScopeFileWriter: Invalid mask when adding a new patch";
        return false;
    }
    //x+width==xxx is ok, because x+width is not real coordinate, real coordinate is x+width-1
    if(x<0||y<0||x>=levelInfos[level]->width||y>=levelInfos[level]->height||x+mask.getWidth()>levelInfos[level]->width||y+mask.getHeight()>levelInfos[level]->height){
        qWarning()<<"ScopeFileWriter: Invalid position when adding a new patch, level: "<<level<<", x: "<<x<<", y: "<<y<<", mask width: "<<mask.getWidth()<<", mask height: "<<mask.getHeight()<<", level width: "<<levelInfos[level]->width<<", level height: "<<levelInfos[level]->height;
        return false;
    }

    this->levelSizes[level]+=mask.getHeight()*mask.getWidth();
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.device()->seek(currentPosition);
    
    int64_t serialNumber = levelInfos[level]->patchNum, compressedDataSize, uncompressedDataSize;
    levelInfos[level]->patchNum++;
    levelInfos[level]->patchDataOffsets.push_back(currentPosition);
    levelInfos[level]->patchWidths.push_back(mask.getWidth());
    levelInfos[level]->patchHeights.push_back(mask.getHeight());
    levelInfos[level]->patchXs.push_back(x);
    levelInfos[level]->patchYs.push_back(y);
    uncompressedDataSize = mask.getRawDataBufferSize();
    unsigned char * rawData = mask.getRawData();
    if(rawData==nullptr){
        qDebug()<<"ScopeFileWriter: Ooops, the mask is valid, but the data buffer is nullptr, bugs!";
        return false;
    }
    // qDebug()<<"rawData[0]="<<rawData[0];
    compressedDataSize = compressBound(uncompressedDataSize);
    unsigned char* compressedData = nullptr;
    try {
        compressedData = new unsigned char[compressedDataSize];
    } catch (std::bad_alloc& e) {
        qWarning()<<"ScopeFileWriter: Failed to allocate memory for compressing the patch data! Maybe OOM!";
        return false;
    }
    z_uLong compressedDataSize2 = compressedDataSize;
    z_uLong uncompressedDataSize2 = uncompressedDataSize;
    int ret = compress2(compressedData, &compressedDataSize2, rawData, uncompressedDataSize2, levelInfos[level]->compressLevel);
    compressedDataSize = compressedDataSize2;
    // qDebug()<<uncompressedDataSize;
    if(ret!=Z_OK){
        qDebug()<<"ScopeFileWriter: Failed to compress the patch data! return code:"<<ret;
        return false;
    }
    stream<<serialNumber<<compressedDataSize<<uncompressedDataSize;
    stream.writeRawData(reinterpret_cast<char *>(compressedData), compressedDataSize);
    currentPosition = file->pos();
    delete[] compressedData;
    return true;
}
bool ScopeFileWriter::finish(){
    int64_t headerOffset = currentPosition;
    int64_t headerLength = 0;
    for(int i = 0;i<levelInfos.size();i++){
        if(this->levelInfos[i]->height*this->levelInfos[i]->width!=this->levelSizes[i]){
            qWarning()<<"ScopeFileWriter: The size of level "<<i<<" is not equal to the sum of all patches' size!"<<"get:"<<this->levelSizes[i]<<", expect:"<<this->levelInfos[i]->height*this->levelInfos[i]->width;
        }
    }
    for(auto levelInfo:levelInfos){
        this->header.addLevelInfo(*levelInfo);
    }
    for(auto levelInfo:levelInfos){
        delete levelInfo;
    }

    ScopeFileUtil::writeHeader(this->file, this->header);
    headerLength = file->pos() - headerOffset;
    file->seek(8);
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    stream<<headerOffset<<headerLength;
    stream<<header.levels;
    this->flush();
    this->finished = true;
    return true;
}
void ScopeFileWriter::flush(){
    this->file->flush();
}
bool ScopeFileWriter::close(){
    if(!this->finished){
        if(!finish()){
            qDebug()<<"ScopeFileWriter: Failed to finish the file! Maybe bugs!";
            if(this->file->isOpen()){
                this->file->close();
            }
            
            return false;
        };
    }

    if(this->file->isOpen()){
        this->file->close();
    }
    return true;
}

int64_t ScopeFileWriter::getCurrentLevels(){
    return this->header.levels;
}
