#include "SimpleSyncTask.h"
#include <QDebug>
SimpleSyncTask::SimpleSyncTask(std::function<OnRequestTask::TASK_STATE(QString &failReason)> task, QString taskName){
    if(taskName.isEmpty()){
        this->taskName = "SimpleSyncTask";
    }else{
        this->taskName = taskName;
    }
    this->task = task;
}

SimpleSyncTask::~SimpleSyncTask(){

}

OnRequestTask::REQUEST_RESULT SimpleSyncTask::requestPause(){
    this->setRejectReason("SimpleSyncTask: This kind of task is unpausable!");
    return REJECT;
}

OnRequestTask::REQUEST_RESULT SimpleSyncTask::requestResume(){
    this->setRejectReason("SimpleSyncTask: This kind of task is unpausable, unable to resume!");
    return REJECT;
}

OnRequestTask::REQUEST_RESULT SimpleSyncTask::requestCancel(){
    this->setRejectReason("SimpleSyncTask: This kind of task is uncancelable");
    return REJECT;
}

int SimpleSyncTask::reportTotalWorkload(){
    return 1;
}

bool SimpleSyncTask::pausable(){
    return false;// You shouldn't put a complicated operations into this function, therefore this kind of task is set unpausable.
}

QString SimpleSyncTask::getTaskName(){
    return this->taskName;
}

void SimpleSyncTask::run(){
    if(this->task(this->failReason)==OnRequestTask::SUCCESS){
        this->switchState(OnRequestTask::SUCCESS);
        emit reportProgressSignal(1,1);
        emit successSignal();
    }else{
        this->switchState(OnRequestTask::FAIL);
        emit reportProgressSignal(1,1);
        emit failSignal(this->failReason);
    }
}
