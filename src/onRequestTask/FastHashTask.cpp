#include "FastHashTask.h"
#include <random>
#include <QDataStream>
#include <QDebug>
FastHashTask::FastHashTask(QString filename){
    this->file.setFileName(filename);
    this->fileName = filename;
}

OnRequestTask::REQUEST_RESULT FastHashTask::requestPause(){
    setRejectReason(tr("FastHashTask: This kind of task is unpausable, unable to pause!"));
    return REQUEST_RESULT::REJECT;
}

OnRequestTask::REQUEST_RESULT FastHashTask::requestResume(){
    setRejectReason(tr("FastHashTask: This kind of task is unpausable, unable to resume!"));
    return REQUEST_RESULT::REJECT;
}

OnRequestTask::REQUEST_RESULT FastHashTask::requestCancel(){
    setRejectReason(tr("FastHashTask: cannot cancel now, try it later"));
    return REQUEST_RESULT::REJECT;
}

int FastHashTask::reportTotalWorkload(){
    return 1;
}

bool FastHashTask::pausable(){
    return false;
}

QString FastHashTask::getTaskName(){
    return "FastHashTask of file:"+this->fileName;
}

void FastHashTask::run(){

    this->hash = 0;
    std::default_random_engine e;
    e.seed(this->file.size());
    if(!this->file.isReadable()){
        this->failReason = tr("FastHashTask: cannot read file");
        emit failSignal(this->failReason);
    }
    QDataStream stream(&this->file);
    this->file.open(QIODevice::ReadOnly);
    if(this->file.size()<sizeof(int64_t)){
        stream.readRawData((char*)&this->hash,this->file.size());
        this->file.close();
        emit reportProgressSignal(1,1);
        emit successSignal();
        return;
    }
    
    int64_t tmp;
    for(int i = 0;i<1024;i++){
        int64_t pos = e()%(this->file.size()-sizeof(int64_t));
        this->file.seek(pos);
        stream>>tmp;
        this->hash^=(tmp^e());
    }
    this->file.close();
    emit reportProgressSignal(1,1);
    emit successSignal();
}

int64_t FastHashTask::getHash(){
    return this->hash;
}
