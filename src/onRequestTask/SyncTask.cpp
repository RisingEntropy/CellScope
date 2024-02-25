#include "SyncTask.h"
#include <QDebug>
OnRequestTask::REQUEST_RESULT SyncTask::requestStart(){
    if(this->getState()==IDLE){
        this->switchState(WORKING);
        this->run();
        return ACCEPT;
    }else{
        this->setRejectReason("SyncTask: Task is not in IDLE state!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT SyncTask::requestPause(){
    if(this->getState()==WORKING){
        this->switchState(PAUSING);
        return ACCEPT;
    }else{
        this->setRejectReason("SyncTask: Task is not in WORKING state!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT SyncTask::requestResume(){
    if(this->getState()==PAUSING){
        this->switchState(WORKING);
        return ACCEPT;
    }else{
        this->setRejectReason("SyncTask: Task is not in PAUSING state!");
        return REJECT;
    }
}

OnRequestTask::REQUEST_RESULT SyncTask::requestCancel(){
    if(this->getState()==WORKING||this->getState()==PAUSING||this->getState()==IDLE){
        this->switchState(FAIL);
        this->setFailReason(tr("SyncTask: Task canceled by user"));
    }else{
        QString state = (this->getState()==SUCCESS?"SUCCESS":"FAIL");
        qDebug()<<"SyncTask: Task finished with state:"+state+",ignore request";
    }
    return ACCEPT;
}

bool SyncTask::pausable(){
    return true;
}
