#include "AIInferTask.h"
#include <QtAlgorithms>
#include <QDebug>
AIInferTask::AIInferTask(QSharedPointer<OpenSlideFileReader> reader,QSharedPointer<ScopeFileWriter> writer, QSharedPointer<Inferable> inferable, int patchSize, unsigned char compreessLevel){

    if(reader.isNull()){
        this->failReason = tr("AIInferTask: The reader is null!");
        this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
        this->switchStatus(Task::TaskStatus::STOPPED);
        qDebug()<<"reader is null";
        return;
    }
    if(writer.isNull()){
        this->failReason = tr("AIInferTask: The writer is null!");
        this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
        this->switchStatus(Task::TaskStatus::STOPPED);
        qDebug()<<"writer is null";
        return;
    }
    if(inferable.isNull()){
        this->failReason = tr("AIInferTask: The inferable is null!");
        this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
        this->switchStatus(Task::TaskStatus::STOPPED);
        qDebug()<<"inferable is null";
        return;
    }
    this->reader = reader;
    this->writer = writer;
    this->inferable = inferable;
    this->patchSize = patchSize;
    this->compressLevel = compreessLevel;
    if(!this->reader->validFile()){
        this->failReason = tr("AIInferTask: The reader is set to a valid file!");
        this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
        this->switchStatus(Task::TaskStatus::STOPPED);
        return;
    }
    
}

AIInferTask::~AIInferTask(){
    this->levelBuffer.clear();
}

int AIInferTask::reportTotoalWorkload(){
    if(this->totalPatches == -1){
        loadAndRerrangeLevelInfo();
    }
    return this->totalPatches;
}

void AIInferTask::work(){
    if(this->getStatus()==Task::TaskStatus::READY){
        this->switchStatus(Task::TaskStatus::WORKING);
        if(this->totalPatches == -1){
            loadAndRerrangeLevelInfo();
        }
        if(this->reader->getLevelCount()<1){
            qDebug()<<"QwQ, WTF????? a svs file with 0 level?????? It sucks my code!!!";
            this->failReason = tr("AIInferTask: The reader does not contain any level!");
            this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
            this->switchStatus(Task::TaskStatus::STOPPED);
            return;
        }
        if(this->writer->getCurrentLevels()!=0){
            this->failReason = tr("AIInferTask: AIInferTask receives a writer with NO previously add levels!!!!");
            this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
            this->switchStatus(Task::TaskStatus::STOPPED);
            return;
        }
        for(int64_t level = 0;level<this->reader->getLevelCount();level++){
            this->writer->addLevel(this->reader->getLevelWidth(level),this->reader->getLevelHeight(level),this->compressLevel);
        }
        auto findNextMultiple = [](int64_t x, int64_t y){return ((x + y - 1) / y) * y;};
        for(int64_t xOffset = 0;xOffset<findNextMultiple(this->reader->getLevelWidth(0),this->patchSize);xOffset+=this->patchSize){
            for(int64_t yOffset = 0;yOffset<findNextMultiple(this->reader->getLevelHeight(0),this->patchSize);yOffset+=this->patchSize){
                if(this->getStatus() == Task::TaskStatus::PAUSED){
                    qDebug()<<"AI task paused, waiting for resume...";
                    while(this->getStatus() == Task::TaskStatus::PAUSED);//wait
                    qDebug()<<"AI task resumed!";
                }
                if(this->getStatus() != Task::TaskStatus::WORKING){
                    this->failReason = tr("Oops, invalid state after the recovering the pause operation. This might be a bug, please report to the developer!");
                    this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
                    this->switchStatus(Task::TaskStatus::STOPPED);
                    return;
                }
                
                int64_t xPatchSize = this->patchSize, yPatchSize = this->patchSize;
                bool overflow = false;
                if(xOffset+this->patchSize>this->reader->getLevelWidth(0)){
                    xPatchSize = this->reader->getLevelWidth(0)-xOffset;
                    overflow = true;
                }
                if(yOffset+this->patchSize>this->reader->getLevelHeight(0)){
                    yPatchSize = this->reader->getLevelHeight(0)-yOffset;
                    overflow = true;
                }
                cv::Mat patch = this->reader->getRegionMat(0,xOffset,yOffset,xPatchSize,yPatchSize);
                if(overflow){
                    cv::copyMakeBorder(patch,patch,0,this->patchSize-yPatchSize,0,this->patchSize-xPatchSize,cv::BORDER_CONSTANT,cv::Scalar(0));
                }
                
                this->inferable->preprocess(patch);
                cv::Mat mask;
                if(!this->inferable->infer(patch,mask)){//performace bottleneck!!!
                    this->failReason = tr("AIInferTask: Fail to infer the patch at (")+QString::number(xOffset)+","+QString::number(yOffset)+"), please see log for more information";
                    this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
                    this->switchStatus(Task::TaskStatus::STOPPED);
                    return;
                }
                if(overflow){
                    mask = mask(cv::Rect(0,0,xPatchSize,yPatchSize));
                }
                
                MaskImage maskImage = MaskImage::fromMat(mask);//use small patch for higher levels, this will cause inefficient storage. User new algorithms later

                this->writer->addPatch(0,xOffset,yOffset,maskImage);
                for(int64_t level = 1;level<this->reader->getLevelCount();level++){
                    double scaleX = getLevelScaleXFromLevel0(level);
                    double scaleY = getLevelScaleYFromLevel0(level);
                    int64_t newX = xOffset/scaleX, newY = yOffset/scaleY;
                    int64_t newW = maskImage.getWidth()/scaleX, newH = maskImage.getHeight()/scaleY;
                    MaskImage newMaskImage = maskImage.resizeNearestAndCopy(newW,newH);
                    this->writer->addPatch(level,newX,newY,newMaskImage);
                }
                this->currentPatches++;
                this->updateProgress();
            }
        }
        if(this->currentPatches!=this->totalPatches){
            qWarning()<<"AIInferTask: The number of patches processed is not equal to the total number of patches, this is a bug, please report to the developer!";
        }
        this->setResultStatus(Task::TaskResultStatus::RESULT_SUCCESS);
        this->switchStatus(Task::TaskStatus::STOPPED);
        emit finishSignal();
    }else{
        qWarning()<<"AIInferTask: work() called when the task is in invalid state:"<<this->getStatus()<<", ignore it!";
        return;
    }
}

bool AIInferTask::done(){
    return this->getStatus()==Task::TaskStatus::STOPPED;
}

void AIInferTask::pause(){
    if(this->status == Task::TaskStatus::WORKING){
        this->switchStatus(Task::TaskStatus::PAUSED);
    }else if(this->status == Task::TaskStatus::PAUSED){
        qWarning()<<"AIInferTask: pause() called when the task is already paused, ignore it!";
    }else{
        qWarning()<<"AIInferTask: pause() called when the task in invalid state:"<<this->getStatus()<<", ignore it!";
    }
}

bool AIInferTask::pausable(){
    return true;
}

void AIInferTask::forceStop(){
    this->setResultStatus(Task::TaskResultStatus::RESULT_FAILED);
    this->switchStatus(Task::TaskStatus::STOPPED);
    this->inferable->clear();
}

QString AIInferTask::taskName(){
    return tr("AIInferTask with Inferable:")+inferable->name()+tr(",comment on this model:")+inferable->info();
}

QString AIInferTask::getFailReason(){
    return this->failReason;
}


int64_t AIInferTask::getTotalCellArea(){
    if(this->getResultStatus()!=Task::TaskResultStatus::RESULT_SUCCESS){
        qWarning()<<"AIInferTask: getTotalCellArea() called when the task is not success";
        return -1;
    }
    return this->totalCellArea;
    
}

void AIInferTask::updateProgress(){
    emit updateProgressSignal(this->currentPatches,this->totalPatches);
}

inline double AIInferTask::getLevelScaleXFromLevel0(int64_t level){
    if(level>=this->levelInfos.size()){
        qWarning()<<"AIInferTask: getLevelScaleX() called with invalid level:"<<level<<", return 1.0";
        return 1.0;
    }
    if(level==0){
        return 1.0;
    }
    
    return (double)this->reader->getLevelWidth(0)/this->reader->getLevelWidth(level);
}

inline double AIInferTask::getLevelScaleYFromLevel0(int64_t level){
    if(level>=this->levelInfos.size()){
        qWarning()<<"AIInferTask: getLevelScaleY() called with invalid level:"<<level<<", return 1.0";
        return 1.0;
    }
    if(level==0){
        return 1.0;
    }
    return (double)this->reader->getLevelHeight(0)/this->reader->getLevelHeight(level);
}

inline int64_t AIInferTask::getLevelMapping(int64_t rawLevel){
    for(int i=0;i<this->levelInfos.size();i++){
        if(std::get<0>(this->levelInfos[i]) == rawLevel){
            return i;
        }
    }
    qWarning()<<"AIInferTask: getLevelMapping() called with invalid level:"<<rawLevel<<", return -1";
    return -1;
}

inline int64_t AIInferTask::getRawLevel(int64_t sortedLevel){
    return std::get<0>(this->levelInfos[sortedLevel]);
}

inline void AIInferTask::loadAndRerrangeLevelInfo(){
    for(int64_t i = 0;i<this->reader->getLevelCount();i++){
        int64_t w = this->reader->getLevelWidth(i);
        int64_t h = this->reader->getLevelHeight(i);
        this->levelInfos.append(std::make_tuple(i,h,w));
    }
    qSort(this->levelInfos.begin(),this->levelInfos.end(),[](const std::tuple<int64_t,int64_t,int64_t>& a,const std::tuple<int64_t,int64_t,int64_t>& b){
        return std::get<1>(a)*std::get<1>(a) > std::get<1>(b)*std::get<2>(b);
    });// make sure level 0 is the biggest level
    totalPatches = ((std::get<1>(this->levelInfos[0])+this->patchSize-1)/this->patchSize)*((std::get<2>(this->levelInfos[0])+this->patchSize-1)/this->patchSize);
}
