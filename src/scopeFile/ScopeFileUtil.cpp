#include "ScopeFileUtil.h"
#include "QtZlib/zconf.h"
#include "qglobal.h"
#include <QDataStream>
#include <stdint.h>
#include <QDebug>
#include <QtZlib/zlib.h>
bool ScopeFileUtil::validateFileHeader(QString path){
    QFile file = QFile(path);
    return ScopeFileUtil::validateFileHeader(&file);
    
}
bool ScopeFileUtil::validateFileHeader(QIODevice* file){
    int64_t fileSize = file->size();
    if(fileSize<48){
        /*
        * Even an empty scope file is 48 bytes long, they are:
        * 4 bytes for magic number
        * 4 bytes for version
        * 8 bytes for header position
        * 8 bytes for header offset
        * 8 bytes for levels_1
        * 8 bytes for levels_2
        * 8 bytes for meta info length
        */
        return false;
    }
    int64_t reservedPos = file->pos();
    bool shouldClose = false;
    if(!file->isOpen()){
        file->open(QIODevice::ReadOnly);
        shouldClose = true;
    }
    auto recoverDevice = [shouldClose, file, reservedPos] (){
            file->seek(reservedPos);
            if(shouldClose)file->close();
    };
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    int32_t magicNumber;
    int32_t version;
    uint64_t headerOffset, headerLength;
    int64_t levels_1,levels_2;
    stream >> magicNumber >> version >> headerOffset >> headerLength>>levels_1;
    if(magicNumber!=ScopeFileHeader::MAGIC_NUMBER){
        recoverDevice();
        return false;
    }
    if(version>ScopeFileHeader::CURRENT_VERSION){
        qWarning()<<"ScopeFileUtil: Reading a scope file of higher version, the header parsing might be incorrect";
    }
    if(headerOffset<0||headerLength<0||headerOffset+headerLength>fileSize){
        qWarning()<<"ScopeFileUtil: Header offset and length are invalid";
        recoverDevice();
        return false;
    }
    file->seek(headerOffset);
    stream>>levels_2;
    if(levels_1!=levels_2){
        qWarning()<<"ScopeFileUtil: levels_1 and levels_2 are not equal, the file might be corrupted";
        recoverDevice();
        return false;
    }
    
    for(int64_t i=0;i<levels_1;i++){
        int64_t levelWidth,levelHeight;
        unsigned char compressLevel;
        int64_t patchNum;
        stream >> levelWidth >> levelHeight >> compressLevel >> patchNum;
        if(levelWidth<0||levelHeight<0){
            qWarning()<<"ScopeFileUtil: Level width and height should be positive, however get w: "<<levelWidth<<" and h:"<<levelHeight<<" at level "<<i;
            recoverDevice();
            return false;
        }
        if(patchNum<0){
            qWarning()<<"ScopeFileUtil: Patch number should be non-negative, however get "<<patchNum<<" at level "<<i;
            recoverDevice();
            return false;
        }
        if(patchNum==0&&(levelHeight!=0&&levelWidth!=0)){// a level in shape 0xN or Nx0 is a valid file.
            qWarning()<<"ScopeFileUtil: A level with patch number 0 should have width or height 0, however get w: "<<levelWidth<<" and h:"<<levelHeight<<" at level "<<i;
            recoverDevice();
            return false;

        }
        if(compressLevel>=10){
            qWarning()<<"ScopeFileUtil: Compress level should be less than 10, however get "<<compressLevel<<" at level "<<i;
            recoverDevice();
            return false;
        }
        if (patchNum > 0) {
            // they can't equal to each other, because when patchNum is 0, there won't be any patchOffsets field
            if (fileSize <= file->pos() + patchNum * (sizeof(int64_t)*5)) {// make sure we can at least read patchNum patches infos
                //                  patchDataOffset+patchWidth+patchHeight+patchX+patchY
                recoverDevice();
                return false;
            }else{
                for(int64_t j=0;j<patchNum;j++){
                    uint64_t patchDataOffset;
                    int64_t patchWidth,patchHeight,patchX,patchY;
                    stream >> patchDataOffset >> patchWidth >> patchHeight >> patchX >> patchY;
                    if(patchDataOffset>fileSize
                        ||patchWidth<0||patchHeight<0||patchX<0||patchY<0
                        ||patchHeight>levelHeight||patchWidth>levelWidth
                        ||patchX+patchWidth>levelWidth||patchY+patchHeight>levelHeight){
                        recoverDevice();
                        return false;
                    }
                }
            }
        }
    }
    uint64_t metaInfoLength;
    stream>>metaInfoLength;
    if(metaInfoLength<0||metaInfoLength+file->pos()>fileSize){// header+metaInfoLength>file length-> bad file
        recoverDevice();
        return false;
    }
    recoverDevice();
    return true;
}

const ScopeFileHeader ScopeFileUtil::parseHeader(QString path){
    QFile file = QFile(path);
    return ScopeFileUtil::parseHeader(&file);
    
}
const ScopeFileHeader ScopeFileUtil::parseHeader(QIODevice* file){
    if(!ScopeFileUtil::validateFileHeader(file)){
        auto header = ScopeFileHeader();
        header.valid = false;
        return header;
    }
    bool shouldClose = false;
    if(!file->isOpen()){
        file->open(QIODevice::ReadOnly);
        shouldClose = true;
    }
    int64_t reservedPos = file->pos();
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    int32_t magicNumber;
    int32_t version;
    uint64_t headerOffset, headerLength;
    int64_t levels_1, levels_2;
    stream >> magicNumber >> version >> headerOffset >> headerLength >> levels_1;
    auto header = ScopeFileHeader();
    header.magicNumber = magicNumber;
    header.version = version;
    header.levels = levels_1;
    file->seek(headerOffset);
    stream>>levels_2;
    if(levels_1!=levels_2){
        qDebug()<<"ScopeFileUtil: levels_1 and levels_2 are not equal when parsing header, bugs might happen qwq, continue to parse header";
    }
    for(int64_t i=0;i<levels_1;i++){
        int64_t levelWidth,levelHeight;
        unsigned char compressLevel;
        int64_t patchNum;
        stream >> levelWidth >> levelHeight >> compressLevel >> patchNum;
        auto levelInfo = ScopeFileHeader::LevelInfo();
        levelInfo.width = levelWidth;
        levelInfo.height = levelHeight;
        levelInfo.compressLevel = compressLevel;
        levelInfo.patchNum = patchNum;
        if(patchNum>0){
            for(int64_t j=0;j<patchNum;j++){
                uint64_t patchDataOffset;
                int64_t patchWidth,patchHeight,patchX,patchY;
                stream >> patchDataOffset >> patchWidth >> patchHeight >> patchX >> patchY;
                levelInfo.patchDataOffsets.append(patchDataOffset);
                levelInfo.patchWidths.append(patchWidth);
                levelInfo.patchHeights.append(patchHeight);
                levelInfo.patchXs.append(patchX);
                levelInfo.patchYs.append(patchY);
            }
        }
        header.addLevelInfo(levelInfo);
    }
    uint64_t metaInfoLength;
    stream>>metaInfoLength;
    char* buf = new char[metaInfoLength];
    stream.readRawData(buf,metaInfoLength);
    QString metaInfo = QString::fromUtf8(buf,metaInfoLength);
    header.metaData = ScopeFileMetaData::loadFromJSONString(metaInfo);
    delete[] buf;
    file->seek(reservedPos);
    header.valid = true;
    return header;
}
bool ScopeFileUtil::thoroughlyValidateFile(QString path){
    QFile file(path);
    return ScopeFileUtil::thoroughlyValidateFile(&file);
}
bool ScopeFileUtil::thoroughlyValidateFile(QIODevice* file){
    ScopeFileHeader header = ScopeFileUtil::parseHeader(file);
    if(!header.valid){
        return false;
    }
    int64_t reservedPos = file->pos();
    bool shouldClose = false;
    if(!file->isOpen()){
        file->open(QIODevice::ReadOnly);
        shouldClose = true;
    }
    QDataStream stream(file);
    stream.setByteOrder(QDataStream::BigEndian);
    for(int64_t level=0;level<header.levels;level++){
        ScopeFileHeader::LevelInfo levelInfo = header.getLevelInfo(level);
        for(int64_t patch = 0;patch<levelInfo.patchNum;patch++){
            uint64_t patchDataOffset = levelInfo.patchDataOffsets[patch];
            int64_t patchWidth = levelInfo.patchWidths[patch];
            int64_t patchHeight = levelInfo.patchHeights[patch];
            int64_t patchX = levelInfo.patchXs[patch];
            int64_t patchY = levelInfo.patchYs[patch];
            file->seek(patchDataOffset);
            int64_t patchSerialNumber;
            uint64_t compressedSize, uncompressedSize;
            stream >> patchSerialNumber >> compressedSize >> uncompressedSize;
            if(patchSerialNumber!=patch){
                qWarning()<<"ScopeFileUtil: Patch serial number is not equal to the expected value, the file might be corrupted";
                file->seek(reservedPos);
                if(shouldClose)file->close();
                return false;
            }
            char* uncompressedBuf = new char[uncompressedSize];
            char* compressedBuf = new char[compressedSize];
            uint64_t acutalUncompressedSize = uncompressedSize;
            if(stream.readRawData(compressedBuf, static_cast<int>(compressedSize))!=compressedSize){
                qWarning()<<"ScopeFileUtil: Failed to read compressed data from file";
                delete[] uncompressedBuf;
                delete[] compressedBuf;
                file->seek(reservedPos);
                if(shouldClose)file->close();
                return false;
            }
            if(uncompress(reinterpret_cast<Bytef*>(uncompressedBuf), reinterpret_cast<uLongf*>(&acutalUncompressedSize), reinterpret_cast<Bytef*>(compressedBuf), static_cast<uLong>(compressedSize))!=Z_OK){
                qWarning()<<"ScopeFileUtil: Failed to uncompress data from file";
                delete[] uncompressedBuf;
                delete[] compressedBuf;
                file->seek(reservedPos);
                if(shouldClose)file->close();
                return false;
            }
            if(acutalUncompressedSize!=uncompressedSize){
                qWarning()<<"ScopeFileUtil: Uncompressed size is not equal to the expected value, the file might be corrupted";
                delete[] uncompressedBuf;
                delete[] compressedBuf;
                file->seek(reservedPos);
                if(shouldClose)file->close();
                return false;
            }
            delete[] uncompressedBuf;
            delete[] compressedBuf;
        }
    }
    file->seek(reservedPos);
    if(shouldClose)file->close();
    return true;
}
int64_t ScopeFileUtil::writeHeader(QIODevice* file, const ScopeFileHeader& header){
    if(!header.valid){
        qWarning()<<"ScopeFileUtil: Invalid header, refuse to write to file";
        return -1;
    }
    try{
        if(!file->isOpen()){
            if(!file->open(QIODevice::WriteOnly)){
                qWarning()<<"ScopeFileUtil: Failed to open QIODevice for writing header.";
                return -1;
            }
        }
        QDataStream stream(file);
        stream.setByteOrder(QDataStream::BigEndian);
        uint64_t initialPos = file->pos();
        stream<<header.levels;
        for(int64_t level=0;level<header.levels;level++){
            const ScopeFileHeader::LevelInfo& levelInfo = header.getLevelInfo(level);
            stream<<levelInfo.width<<levelInfo.height<<levelInfo.compressLevel<<levelInfo.patchNum;
            for(int64_t patch=0;patch<levelInfo.patchNum;patch++){
                stream<<levelInfo.patchDataOffsets[patch]<<levelInfo.patchWidths[patch]<<levelInfo.patchHeights[patch]<<levelInfo.patchXs[patch]<<levelInfo.patchYs[patch];
            }
        }
        QByteArray metaInfo = ScopeFileMetaData::dumpToJSONString(header.metaData).toUtf8();
        uint64_t metaInfoSize = metaInfo.size();
        stream<<metaInfoSize;
        stream.writeRawData(metaInfo.data(), metaInfoSize);
        uint64_t finalPos = file->pos();
        return finalPos-initialPos+1;
    }catch(...){
        qWarning()<<"ScopeFileUtil: Failed to write header to file";
        return -1;
    }
}