#include "ScopeFileHeader.h"
#include <QDebug>
const ScopeFileHeader::LevelInfo& ScopeFileHeader::getLevelInfo(int64_t level) const{
    if(level<0 || level>=this->levelInfos.size()){
        qDebug()<<"Invalid level";
    }
    return this->levelInfos[level];
}
void ScopeFileHeader::addLevelInfo(LevelInfo levelInfo){
    this->levelInfos.append(levelInfo);
}