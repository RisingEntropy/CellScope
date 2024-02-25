#include "OnRequestTask.h"
#include <QDebug>
OnRequestTask::TASK_STATE OnRequestTask::getState(){
    return this->state;
}

QString OnRequestTask::getRefuseReason(){
    return this->rejectReason;
}

QString OnRequestTask::getFailReason(){
    return this->failReason;
}

void OnRequestTask::switchState(TASK_STATE targetState){
    this->state = targetState;
}

void OnRequestTask::setRejectReason(QString reason){
    this->rejectReason = reason;
}

void OnRequestTask::setFailReason(QString reason){
    this->failReason = reason;
}
void OnRequestTask::startSlot(){
    this->requestStart();
    qDebug()<<"Slot!";
}
void OnRequestTask::pauseSlot(){
    this->requestPause();
}
void OnRequestTask::resumeSlot(){
    this->requestResume();
}

void OnRequestTask::cancelSlot(){
    this->requestCancel();
}
