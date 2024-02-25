#include "SimpleTask.h"

void SimpleTask::updateProgress(){
    emit updateProgressSignal(1,1);
}

SimpleTask::SimpleTask(std::function<Task::TaskResultStatus(QString &)> task, QString name){
    this->task = task;
    this->switchStatus(TaskStatus::READY);
    this->name = name;
}
int  SimpleTask::reportTotoalWorkload(){
    return 1;
}
void SimpleTask::work(){
    this->switchStatus(TaskStatus::WORKING);
    auto resultStatus = this->task(this->failReason);
    updateProgress();
    this->setResultStatus(resultStatus);
    this->switchStatus(TaskStatus::STOPPED);
    emit finishSignal();
}
bool SimpleTask::done(){
    return this->getStatus() == TaskStatus::STOPPED;
}
void SimpleTask::pause(){
    return;
}
bool SimpleTask::pausable(){
    return false;
}
void SimpleTask::forceStop(){
    this->switchStatus(TaskStatus::STOPPED);
}
QString SimpleTask::taskName(){
    if(this->name==""){
        return "SimpleTask";
    }else{
        return this->name;
    }
}