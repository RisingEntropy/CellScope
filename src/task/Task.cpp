#include "Task.h"

Task::TaskStatus Task::getStatus(){
    return this->status;
}

void Task::setResultStatus(TaskResultStatus status){
    this->mutex.lock();
    this->resultStatus = status;
    this->mutex.unlock();
    if(status==TaskResultStatus::RESULT_FAILED){
        emit taskFailSignal(this->getFailReason());
    }
}

void Task::switchStatus(TaskStatus status)
{
    this->mutex.lock();
    this->status = status;
    this->mutex.unlock();
}
Task::TaskResultStatus Task::getResultStatus(){
    return this->resultStatus;
}
QString Task::getFailReason(){
    return "";
}

void Task::resume(){
    if(this->pausable()){
        this->switchStatus(TaskStatus::WORKING);
    }else{
        qDebug()<<"Try to resume a non-pausable task. This is a bug!";
    }
}
