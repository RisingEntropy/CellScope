#include "TiledFileAIInferenceTask.h"
#include <QDebug>
#include "../scopeFile/ScopeFileMetaData.h"
#include "../scopeFile/ScopeFileHeader.h"
TiledFileAIInferenceTask::TiledFileAIInferenceTask(QSharedPointer<OpenSlideFileReader> reader, QSharedPointer<ScopeFileWriter> writer, QSharedPointer<Inferable> inferable, 
                                                    int patchSize, unsigned char compreessLevel, double dropEdgeRatio){
    this->reader = reader;
    this->writer = writer;
    this->inferable = inferable;
    this->patchSize = patchSize;
    this->compressLevel = compreessLevel;
    this->dropEdgeRatio = dropEdgeRatio;
    if(reader.isNull()){
        this->failReason = tr("TiledFileAIInferenceTask: The reader is null!");
        this->switchState(OnRequestTask::FAIL);
        qDebug()<<"reader is null";
        return;
    }
    if(writer.isNull()){
        this->failReason = tr("TiledFileAIInferenceTask: The writer is null!");
        this->switchState(OnRequestTask::FAIL);
        qDebug()<<"writer is null";
        return;
    }
    if(inferable.isNull()){
        this->failReason = tr("TiledFileAIInferenceTask: The inferable is null!");
        this->switchState(OnRequestTask::FAIL);
        qDebug()<<"inferable is null";
        return;
    }
    if(!this->reader->validFile()){
        this->failReason = tr("TiledFileAIInferenceTask: The reader is set to a valid file!");
        this->switchState(OnRequestTask::FAIL);
        return;
    }
}

TiledFileAIInferenceTask::~TiledFileAIInferenceTask(){
    this->inferable->clear();
    this->writer->close();
}

int TiledFileAIInferenceTask::reportTotalWorkload(){
    if(this->totalPatches==-1){
        this->loadAndRerrangeLevelInfo();
    }
    return this->totalPatches;
}

void TiledFileAIInferenceTask::run(){
    if(this->getState()!=OnRequestTask::WORKING){
        this->failReason = tr("The task is not in WORKING state!");
        emit failSignal(failReason);
        return;
    }
    emit startSignal();
    if(this->totalPatches == -1){
        loadAndRerrangeLevelInfo();
    }
    if(this->reader->getLevelCount()<1){
        qDebug()<<"QwQ, WTF????? a svs file with 0 level?????? It sucks my code!!!";
        this->failReason = tr("The reader does not contain any level!");
        this->switchState(OnRequestTask::FAIL);
        emit failSignal(this->failReason);
        return;
    }
    if(this->writer->getCurrentLevels()!=0){
        this->failReason = tr("AI infer task receives a writer with NO previously add levels!!!!");
        this->switchState(OnRequestTask::FAIL);
        emit failSignal(this->failReason);
        return;
    }
    for(int64_t level = 0;level<this->reader->getLevelCount();level++){
        this->writer->addLevel(this->reader->getLevelWidth(level),this->reader->getLevelHeight(level),this->compressLevel);
    }
    auto findNextMultiple = [](int64_t x, int64_t y){return ((x + y - 1) / y) * y;};
    for(int64_t xOffset = 0;xOffset<findNextMultiple(this->reader->getLevelWidth(0),this->patchSize*(1-this->dropEdgeRatio));xOffset+=this->patchSize*(1-this->dropEdgeRatio)){
        for(int64_t yOffset = 0;yOffset<findNextMultiple(this->reader->getLevelHeight(0),this->patchSize*(1-this->dropEdgeRatio));yOffset+=this->patchSize*(1-this->dropEdgeRatio)){
            if(this->getState() == PAUSING){
                emit pauseSignal();
                while(this->getState() == PAUSING);//wait
                if(this->getState() == IDLE){
                    this->failReason = tr("Oops, invalid state after the recovering the pause operation. This might be a bug, please report to the developer!");
                    this->switchState(OnRequestTask::FAIL);
                    this->inferable->clear();
                    emit failSignal(this->failReason);
                    return;
                }
                emit resumeSignal();
            }
            if(this->getState() == FAIL){//only when user manually cancel the task, the state will be FAIL for this task
                this->inferable->clear();
                failReason = tr("User cancelled");
                emit cancelSignal();
                return;
            }
            // int64_t xPatchSize = this->patchSize, yPatchSize = this->patchSize;
            // bool overflow = false;
            // if(xOffset+this->patchSize>=this->reader->getLevelWidth(0)){
            //     xPatchSize = this->reader->getLevelWidth(0)-xOffset;
            //     overflow = true;
            // }
            // if(yOffset+this->patchSize>=this->reader->getLevelHeight(0)){
            //     yPatchSize = this->reader->getLevelHeight(0)-yOffset;
            //     overflow = true;
            // }
            // cv::Mat patch = this->reader->getRegionMat(0,xOffset,yOffset,xPatchSize,yPatchSize);
            // if(overflow){
            //     cv::copyMakeBorder(patch,patch,0,this->patchSize-yPatchSize,0,this->patchSize-xPatchSize,cv::BORDER_CONSTANT,cv::Scalar(0));
            // }


            cv::Mat mask;
            cv::Mat patch;
            this->cropPatch(xOffset,yOffset,patch);
            this->inferable->preprocess(patch);
            if(!this->inferable->infer(patch,mask)){//performace bottleneck!!!
                this->failReason = tr("AIInferTask: Fail to infer the patch at (")+QString::number(xOffset)+","+QString::number(yOffset)+"), please see log for more information";
                this->switchState(OnRequestTask::FAIL);
                emit failSignal(this->failReason);
                return;
            }
            this->cropMask(xOffset,yOffset,mask);
            MaskImage maskImage = MaskImage::fromMat(mask);//use small patch for higher levels, this will cause inefficient storage. User new algorithms later
            this->writer->addPatch(0,xOffset,yOffset,maskImage);
            this->totalCellArea += maskImage.bitCount<true>();
            for(int64_t level = 1;level<this->reader->getLevelCount();level++){
                double scaleX = getLevelScaleXFromLevel0(level);
                double scaleY = getLevelScaleYFromLevel0(level);
                int64_t newX = 1.0*xOffset/scaleX, newY = 1.0*yOffset/scaleY;
                int64_t newW = 1.0*maskImage.getWidth()/scaleX, newH = 1.0*maskImage.getHeight()/scaleY;
                MaskImage newMaskImage = maskImage.resizeNearestAndCopy(newW,newH);
                this->writer->addPatch(level,newX,newY,newMaskImage);
            }
            this->currentPatches++;
            emit reportProgressSignal(this->currentPatches, this->totalPatches);
        }
    }
    if(this->currentPatches!=this->totalPatches){
        qWarning()<<"AIInferTask: The number of patches processed is not equal to the total number of patches, this is a bug, please report to the developer!";
    }
    this->writer->getHeader().metaData.setProperty("TotoalCellArea",QString::number(this->totalCellArea));
    this->switchState(OnRequestTask::SUCCESS);
    emit successSignal();
}

QString TiledFileAIInferenceTask::getTaskName(){
    return tr("AI Infer Task using model: zgnUNet 20240220 for VIM stein");
}

int64_t TiledFileAIInferenceTask::getTotalCellArea(){
    return this->totalCellArea;
}

inline double TiledFileAIInferenceTask::getLevelScaleXFromLevel0(int64_t level)
{
    if(level>=this->levelInfos.size()){
        qWarning()<<"TiledFileAIInferenceTask: getLevelScaleX() called with invalid level:"<<level<<", return 1.0";
        return 1.0;
    }
    if(level==0){
        return 1.0;
    }
    
    return (double)this->reader->getLevelWidth(0)/this->reader->getLevelWidth(level);
}

inline double TiledFileAIInferenceTask::getLevelScaleYFromLevel0(int64_t level){
    if(level>=this->levelInfos.size()){
        qWarning()<<"TiledFileAIInferenceTask: getLevelScaleY() called with invalid level:"<<level<<", return 1.0";
        return 1.0;
    }
    if(level==0){
        return 1.0;
    }
    return 1.0*this->reader->getLevelHeight(0)/(this->reader->getLevelHeight(level)*1.0);
}

inline int64_t TiledFileAIInferenceTask::getLevelMapping(int64_t rawLevel){
    for(int i=0;i<this->levelInfos.size();i++){
        if(std::get<0>(this->levelInfos[i]) == rawLevel){
            return i;
        }
    }
    qWarning()<<"TiledFileAIInferenceTask: getLevelMapping() called with invalid level:"<<rawLevel<<", return -1";
    return -1;
}

inline int64_t TiledFileAIInferenceTask::getRawLevel(int64_t sortedLevel){
    return std::get<0>(this->levelInfos[sortedLevel]);
}

inline void TiledFileAIInferenceTask::loadAndRerrangeLevelInfo(){
    for(int64_t i = 0;i<this->reader->getLevelCount();i++){
        int64_t w = this->reader->getLevelWidth(i);
        int64_t h = this->reader->getLevelHeight(i);
        this->levelInfos.append(std::make_tuple(i,h,w));
    }
    qSort(this->levelInfos.begin(),this->levelInfos.end(),[](const std::tuple<int64_t,int64_t,int64_t>& a,const std::tuple<int64_t,int64_t,int64_t>& b){
        return std::get<1>(a)*std::get<1>(a) > std::get<1>(b)*std::get<2>(b);
    });// make sure level 0 is the biggest level
    int64_t realPatch = this->patchSize*(1-this->dropEdgeRatio);
    totalPatches = ((std::get<1>(this->levelInfos[0])+this->patchSize-1)/(realPatch)*((std::get<2>(this->levelInfos[0])+realPatch-1)/(realPatch)));
}

inline void TiledFileAIInferenceTask::cropPatch(int64_t x, int64_t y, cv::Mat &result){
    int64_t actualX1 = x-this->dropEdgeRatio/2.0*patchSize;
    int64_t actualY1 = y-this->dropEdgeRatio/2.0*patchSize;
    int64_t actualX2 = x+patchSize+this->dropEdgeRatio/2.0*patchSize;
    int64_t actualY2 = y+patchSize+this->dropEdgeRatio/2.0*patchSize;
    int64_t paddingX1 = 0, paddingY1 = 0, paddingX2 = 0, paddingY2 = 0;
    actualX2+=patchSize-(actualX2-actualX1+1);
    actualY2+=patchSize-(actualY2-actualY1+1);
    if(actualX1<0){
        paddingX1 = -actualX1;
        actualX1 = 0;
    }
    if(actualY1<0){
        paddingY1 = -actualY1;
        actualY1 = 0;
    }
    if(actualX2>=this->reader->getLevelWidth(0)){
        paddingX2 = actualX2-this->reader->getLevelWidth(0)+1;
        actualX2 = this->reader->getLevelWidth(0)-1;
    }
    if(actualY2>=this->reader->getLevelHeight(0)){
        paddingY2 = actualY2-this->reader->getLevelHeight(0)+1;
        actualY2 = this->reader->getLevelHeight(0)-1;
    }
    result = this->reader->getRegionMat(0,actualX1,actualY1,actualX2-actualX1+1,actualY2-actualY1+1);
    cv::copyMakeBorder(result,result,paddingY1,paddingY2,paddingX1,paddingX2,cv::BORDER_CONSTANT,cv::Scalar(0));
}

inline void TiledFileAIInferenceTask::cropMask(int64_t x, int64_t y, cv::Mat &mask){
    int64_t actualX1 = x-this->dropEdgeRatio/2.0*patchSize;
    int64_t actualY1 = y-this->dropEdgeRatio/2.0*patchSize;
    int64_t actualX2 = x+patchSize+this->dropEdgeRatio/2.0*patchSize;
    int64_t actualY2 = y+patchSize+this->dropEdgeRatio/2.0*patchSize;
    mask = mask(cv::Rect(x-actualX1,y-actualY1,
                        qMin((int64_t)(patchSize*(1-this->dropEdgeRatio)),this->reader->getLevelWidth(0)-1-x+1),//width = coord_r - coord_l +1, coord_r = xxx.getLevelWidth(0)
                        qMin((int64_t)(patchSize*(1-this->dropEdgeRatio)),this->reader->getLevelHeight(0)-1-y+1)));
    
}
