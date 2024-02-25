#include "AsyncTask.h"
#include <QDebug>
AsyncTask::~AsyncTask(){


    if(this->getState()==WORKING||this->getState()==PAUSING){
        qWarning()<<"AsyncTask: The task is still in WORKING or PAUSING state when it's destroyed, is there logic misorders?";
    }
    if(this->thread.isRunning()){
        this->thread.quit();
        this->thread.wait();
    }
}
OnRequestTask::REQUEST_RESULT AsyncTask::requestStart(){
    if(this->getState()==IDLE){
        this->switchState(WORKING);
        this->moveToThread(&this->thread);
        connect(&this->thread, &QThread::started, this, &AsyncTask::run);
        this->thread.start();
        return ACCEPT;
    }else{
        this->setRejectReason("AsyncTask: The task is not in IDLE state, unable to start!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT AsyncTask::requestPause(){
    if(this->getState()==WORKING){
        qDebug()<<"AsyncTask: Task paused!";
        this->switchState(PAUSING);
        return ACCEPT;
    }else{
        this->setRejectReason("AsyncTask: The task is not in WORKING state, unable to pause!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT AsyncTask::requestResume(){
    if(this->getState()==PAUSING){
        this->switchState(WORKING);
        return ACCEPT;
    }else{
        this->setRejectReason("AsyncTask: The task is not in PAUSING state, unable to resume!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT AsyncTask::requestCancel(){
    if(this->getState()==IDLE||this->getState()==WORKING||this->getState()==PAUSING){
        this->switchState(FAIL);
    }else{
        QString state = (this->getState()==SUCCESS?"SUCCESS":"FAIL");
        qDebug()<<"AsyncTask: Task finished with state:"+state+",ignore request";
    }
    return ACCEPT;
}

bool AsyncTask::pausable(){
    return true;
}

void AsyncTask::run(){
    qDebug()<<"Fuck";
}
