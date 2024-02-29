#include "ScopeFileReader.h"
#include "qglobal.h"
#include <QDebug>
#include <QtZlib/zlib.h>
#include <stdint.h>
ScopeFileReader::ScopeFileReader(QString fileName){
    shouldDeleteFilePointer = true;
    this->file = QSharedPointer<QFile>(new QFile(fileName));
    this->in.setDevice(this->file.data());
    if(!file->exists()){
        this->validFile = false;
        return;
    }
    this->header = ScopeFileUtil::parseHeader(file.data());
    if(!this->header.valid){
        this->validFile = false;
        return;
    }
    this->metaData = header.metaData;
    this->validFile = true;
}
ScopeFileReader::ScopeFileReader(QSharedPointer<QFile> file){
    shouldDeleteFilePointer = false;
    this->file = file;
    this->in.setDevice(this->file.data());
    if(!file->exists()){
        this->validFile = false;
        return;
    }
    this->validFile = true;
}
ScopeFileReader::~ScopeFileReader(){
    if(shouldDeleteFilePointer){
        if(this->file->isOpen()){
            this->file->close();
        }
    }
    
}
bool ScopeFileReader::valid(){
    return this->validFile;
}
ScopeFileHeader& ScopeFileReader::getHeader(){
    return this->header;
}
ScopeFileMetaData& ScopeFileReader::getMetaData(){
    return this->metaData;
}
bool ScopeFileReader::getPixel(int64_t level,int64_t x,int64_t y){
    return false;
}
MaskImage ScopeFileReader::readRegion(int64_t level, int64_t x, int64_t y, int64_t w, int64_t h){
    if(!this->validFile){
        qWarning()<<"ScopeFileReader: Try to read from an invalid file!";
        return MaskImage(nullptr,0,0,false);
    }
    if(level<0 || level>=this->header.levels){
        qWarning()<<"ScopeFileReader: Try to read from an invalid level!";
        return MaskImage(nullptr,0,0,false);
    }
    if(x<0 || y<0 || x+w>this->header.getLevelInfo(level).width || y+h>this->header.getLevelInfo(level).height){
        qWarning()<<"ScopeFileReader: Try to read from an invalid region! x:"<<x<<" y:"<<y<<" w:"<<w<<" h:"<<h<<"levelWidth:"<<this->header.getLevelInfo(level).width<<" levelHeight:"<<this->header.getLevelInfo(level).height;
        return MaskImage(nullptr,0,0,false);
    }
    if(!this->file->isOpen()){
        this->file->open(QIODevice::ReadOnly);
    }
    
    const ScopeFileHeader::LevelInfo& levelInfo = this->header.getLevelInfo(level);
    unsigned char compressLevel = levelInfo.compressLevel;
    
    MaskImage maskImage(w,h);
    maskImage.setCellCount(0);
    int64_t regionX1 = x;
    int64_t regionY1 = y;
    int64_t regionX2 = x+w-1;
    int64_t regionY2 = y+h-1;
    auto between = [](int64_t x,int64_t a,int64_t b){return x>=a&&x<=b;};
    for(int64_t i = 0;i<levelInfo.patchNum;i++){
        int64_t patchX = levelInfo.patchXs[i];
        int64_t patchY = levelInfo.patchYs[i];
        int64_t patchWidth = levelInfo.patchWidths[i];
        int64_t patchHeight = levelInfo.patchHeights[i];
        uint64_t patchDataOffset = levelInfo.patchDataOffsets[i];
        int64_t patchX1 = patchX;
        int64_t patchY1 = patchY;
        int64_t patchX2 = patchX+patchWidth-1;
        int64_t patchY2 = patchY+patchHeight-1;

        //calculate IOU
        int64_t x1 = qMax(patchX1, regionX1);
        int64_t y1 = qMax(patchY1, regionY1);
        int64_t x2 = qMin(patchX2, regionX2);
        int64_t y2 = qMin(patchY2, regionY2);

        int64_t iou = (qMax(x2-x1+1, 0LL))*(qMax(y2-y1+1,0LL));
        if(iou==0){
            continue;// no 
        }
        //region entirely inlcuded in a specific patch
        if(patchX1<=regionX1&&patchY1<=regionY1&&patchX2>=regionX2&&patchY2>=regionY2){
            MaskImage patch = this->readPatch(level, i);
            return patch.crop(regionX1-patchX1, regionY1-patchY1, w, h);
        }
        //patch entire included in a specific region
        if(between(patchX1, regionX1, regionX2)&&between(patchY1, regionY1, regionY2)&&between(patchX2, regionX1, regionX2)&&between(patchY2, regionY1, regionY2)){
            MaskImage patch = this->readPatch(level, i);
            maskImage.fill(patch,patchX-regionX1,patchY-regionY1);
            continue;
        }
        int64_t x1InsidePatch;
        int64_t y1InsidePatch;
        int64_t x2InsidePatch;
        int64_t y2InsidePatch;
        int64_t x1InsideRegion;
        int64_t y1InsideRegion;
        int64_t x2InsideRegion;
        int64_t y2InsideRegion;

        x1InsidePatch = qMax(0LL, regionX1-patchX1);
        y1InsidePatch = qMax(0LL, regionY1-patchY1);
        x2InsidePatch = patchX2<regionX2?patchX2-patchX1:regionX2-patchX1;
        y2InsidePatch = patchY2<regionY2?patchY2-patchY1:regionY2-patchY1;
        x1InsideRegion = qMax(0LL, patchX1-regionX1);
        y1InsideRegion = qMax(0LL, patchY1-regionY1);
        x2InsideRegion = between(patchX2, regionX1, regionX2)?patchX2-regionX1:regionX2-regionX1;
        y2InsideRegion = between(patchY2, regionY1, regionY2)?patchY2-regionY1:regionY2-regionY1;

        if((x2InsidePatch-x1InsidePatch+1)*(y2InsidePatch-y1InsidePatch+1)!=(x2InsideRegion-x1InsideRegion+1)*(y2InsideRegion-y1InsideRegion+1)
            ||(x2InsidePatch-x1InsidePatch+1)*(y2InsidePatch-y1InsidePatch+1)!=iou){
            QString error = QString("!!!!!!!!ScopeFileReader: QwQ, it seems there's a bug in the algorithm, patchX1:%1, patchY1:%2, patchX2:%3, patchY2:%4, regionX1:%5, regionY1:%6, regionX2:%7, regionY2:%8")
                        .arg(patchX1).arg(patchY1).arg(patchX2).arg(patchY2).arg(regionX1).arg(regionY1).arg(regionX2).arg(regionY2);
            qDebug()<<error;
        }
        MaskImage patch = this->readPatch(level, i);
        MaskImage patchSubRegion = patch.crop(x1InsidePatch, y1InsidePatch, (x2InsidePatch-x1InsidePatch+1), (y2InsidePatch-y1InsidePatch+1));
        maskImage.fill(patchSubRegion, x1InsideRegion, y1InsideRegion);
    }
    return maskImage;
}
MaskImage ScopeFileReader::readPatch(int64_t level, int64_t patchSerialNumber){
    uint64_t patchDataOffset = this->header.getLevelInfo(level).patchDataOffsets[patchSerialNumber];
    int64_t patchWidth = this->header.getLevelInfo(level).patchWidths[patchSerialNumber];
    int64_t patchHeight = this->header.getLevelInfo(level).patchHeights[patchSerialNumber];
    uint64_t dataSizeCompressed, dataSizeUncompressed, actualUncompressedSize;
    int64_t actualSerialNumber, cellCount;

    in.device()->seek(patchDataOffset);
    in>>actualSerialNumber>>dataSizeCompressed>>dataSizeUncompressed>>cellCount;
    if(actualSerialNumber!=patchSerialNumber){
        qWarning()<<"ScopeReader: The actual serial number is different from the expected serial number! Please check the file!";
        return MaskImage(nullptr,0,0,false);
    }
    actualUncompressedSize = dataSizeUncompressed;
    unsigned char* compressedBuf = new unsigned char[dataSizeCompressed];
    unsigned char* uncompressedBuf = new unsigned char[dataSizeUncompressed];
    in.readRawData((char*)compressedBuf,dataSizeCompressed);
    z_uLong actualUncompressedSize2 = actualUncompressedSize;
    z_uLong dataSizeCompressed2 = dataSizeCompressed;
    int ret = uncompress2(uncompressedBuf, &actualUncompressedSize2, compressedBuf, &dataSizeCompressed2);
    actualUncompressedSize = actualUncompressedSize2;

    if(ret!=Z_OK){
        qWarning()<<"ScopeReader: Failed to uncompress patch data! Please check the file!";
        delete[] compressedBuf;
        delete[] uncompressedBuf;
        return MaskImage(nullptr,0,0,false);
    }
    if(dataSizeUncompressed!=actualUncompressedSize){
        qWarning()<<"ScopeReader: The actual uncompressed size is different from the expected size! Please check the file!";
        delete[] compressedBuf;
        delete[] uncompressedBuf;
        return MaskImage(nullptr,0,0,false);
    }
    MaskImage patch(uncompressedBuf,patchWidth,patchHeight,true);// performance might be slew down here, leve it later
    patch.setCellCount(cellCount);
    delete[] compressedBuf;
    delete[] uncompressedBuf;
    return patch;
}
/*

        // For debug
        // divide into 8 case, see res/ScopeFileStructure.xlsx Sheet2
        // the patch is partially inside the region, fill the included part
        // if(patchX1<regionX1&&patchY1<regionY1&&between(patchX2, regionX1, regionX2)&&between(patchY2, regionY1, regionY2)){ // Case 1
        //     x1InsidePatch = regionX1-patchX1;
        //     y1InsidePatch = regionY1-patchY1;
        //     x2InsidePatch = patchX2;
        //     y2InsidePatch = patchY2;
        //     x1InsideRegion = 0;
        //     y1InsideRegion = 0;
        //     x2InsideRegion = patchX2-regionX1;
        //     y2InsideRegion = patchY2-regionY1;
        // }else if(patchX1<regionX1&&between(patchY1, regionY1, regionY2)&&between(patchX2, regionX1, regionX2)&&between(patchY2, regionY1, regionY2)){ // Case 8
        //     x1InsidePatch = regionX1-patchX1;
        //     y1InsidePatch = 0;
        //     x2InsidePatch = patchX2;
        //     y2InsidePatch = patchY2;
        //     x1InsideRegion = 0;
        //     y1InsideRegion = patchY1-regionY1;
        //     x2InsideRegion = patchX2-regionX1;
        //     y2InsideRegion = patchY2-regionY1;
        // }else if(patchX1<regionX1&&between(patchY1, regionY1, regionY2)&&between(patchX2, regionX1, regionX2)&&patchY2>regionY2){ // Case 7
        //     x1InsidePatch = regionX1-patchX1;
        //     y1InsidePatch = 0;
        //     x2InsidePatch = patchX2;
        //     y2InsidePatch = regionY2-patchY1;
        //     x1InsideRegion = 0;
        //     y1InsideRegion = patchY1-regionY1;
        //     x2InsideRegion = patchX2-regionX1;
        //     y2InsideRegion = regionY2;
        // }else if(between(patchX1, regionX1, regionX2)&&between(patchY1, regionY1, regionY2)&&between(patchX2, regionX1, regionX2)&&patchY2>regionY2){  // Case 6
        //     x1InsidePatch = 0;
        //     y1InsidePatch = 0;
        //     x2InsidePatch = patchX2;
        //     y2InsidePatch = regionY2-patchY1;// cordinates can be directly used, height and weight must -1
        //     x1InsideRegion = patchX1-regionX1;
        //     y1InsideRegion = patchY1-regionY1;
        //     x2InsideRegion = patchX2-regionX1;
        //     y2InsideRegion = regionY2;
        // }else if(between(patchX1, regionX1, regionX2)&&between(patchY1, regionY1, regionY2)&&patchX2>regionX2&&patchY2>regionY2){ // Case 5
        //     x1InsidePatch = 0;
        //     y1InsidePatch = 0;
        //     x2InsidePatch = patchX2-regionX2;
        //     y2InsidePatch = regionY2;
        //     x1InsideRegion = patchX1-regionX1;
        //     y1InsideRegion = patchY1-regionY1;
        //     x2InsideRegion = regionX2;
        //     y2InsideRegion = regionY2;
        // }else if(between(patchX1, regionX1, regionX2)&&between(patchY1, regionY1, regionY2)&&patchX2>regionX2&&between(patchY2, regionY1, regionY2)){ // Case 4
        //     x1InsidePatch = 0;
        //     y1InsidePatch = 0;
        //     x2InsidePatch = regionX2-patchX1;
        //     y2InsidePatch = patchY2;
        //     x1InsideRegion = patchX1-regionX1;
        //     y1InsideRegion = patchY1-regionY1;
        //     x2InsideRegion = regionX2;
        //     y2InsideRegion = patchY2-regionY1;
        // }else if(between(patchX1, regionX1, regionX2)&&patchY1<regionY1&&between(patchX2, regionX1, regionX2)&&between(patchY2, regionY1, regionY2)){ // Case 3
        //     x1InsidePatch = 0;
        //     y1InsidePatch = regionY1-patchY1;
        //     x2InsidePatch = regionX2-patchX1;
        //     y2InsidePatch = patchY2;
        //     x1InsideRegion = patchX1-regionX1;
        //     y1InsideRegion = 0;
        //     x2InsideRegion = regionX2;
        //     y2InsideRegion = patchY2-regionY1;
        // }else if(between(patchX1,regionX1,regionX2)&&patchY1<regionY2&&between(patchX2, regionX1, regionX2)&&between(patchY2, regionY1, regionY2)){// Case 2
        //     x1InsidePatch = 0;
        //     y1InsidePatch = regionY1-patchY1;
        //     x2InsidePatch = patchX2;
        //     y2InsidePatch = patchY2;
        //     x1InsideRegion = patchX1-regionX1;
        //     y1InsideRegion = 0;
        //     x2InsideRegion = patchX2-regionX1;
        //     y2InsideRegion = patchY2-regionY1;
        // }
*/