#include "QueuedAsyncTask.h"
#include <QSharedPointer>
#include <QDebug>
QueuedAsyncTask::~QueuedAsyncTask(){
    if(this->getState()==WORKING||this->getState()==PAUSING){
        qWarning()<<"QueuedAsyncTask: The task is still in WORKING or PAUSING state when it's destroyed, is there logic misorders?";
    }
}
OnRequestTask::REQUEST_RESULT QueuedAsyncTask::requestPause(){
    if(this->getState()==OnRequestTask::IDLE){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in IDLE state, it's not allowed to pause"));
        return REJECT;
    }else if(this->getState()==WORKING){

        if(this->taskQueue[this->currentTaskIndex]->requestPause()==ACCEPT){
            this->switchState(OnRequestTask::PAUSING);
            return ACCEPT;
        }else{
            this->setRejectReason(tr("QueuedAsyncTask: The current subtask rejected pause request with reason:")+
                        this->taskQueue[this->currentTaskIndex]->getRefuseReason()+tr(", try it later!"));
            return REJECT;
        }
    }else if(this->getState()==PAUSING){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in PAUSING state, it's not allowed to pause again"));
        return REJECT;
    }else if(this->getState()==FAIL){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in FAIL state, can not pause"));
        return REJECT;
    }else if(this->getState()==SUCCESS){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in SUCCESS state, can not pause"));
        return REJECT;
    }else{
        this->setRejectReason(tr("QueuedAsyncTask: The task is in invalid state, seeing this means bugs occurred, report to the developer"));
        return REJECT;
    }
    return REJECT;
}

OnRequestTask::REQUEST_RESULT QueuedAsyncTask::requestResume(){
    if(this->getState()==OnRequestTask::IDLE){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in IDLE state, it's not allowed to resume"));
        return REJECT;
    }else if(this->getState()==WORKING){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in WORKING state, can not resume"));
        return REJECT;
    }else if(this->getState()==PAUSING){
        if(this->taskQueue[this->currentTaskIndex]->requestResume()==ACCEPT){
            this->switchState(OnRequestTask::WORKING);
            return ACCEPT;
        }else{
            this->setRejectReason(tr("QueuedAsyncTask: The current subtask rejected resume request with reason:")+
                        this->taskQueue[this->currentTaskIndex]->getRefuseReason()+tr(", try it later!"));
            return REJECT;
        }
        
    }else if(this->getState()==FAIL){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in FAIL state, can not resume"));
        return REJECT;
    }else if(this->getState()==SUCCESS){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in SUCCESS state, can not resume"));
        return REJECT;
    }else{
        this->setRejectReason(tr("QueuedAsyncTask: The task is in invalid state, seeing this means bugs occurred, report to the developer"));
        return REJECT;
    }
    return REJECT;
}

OnRequestTask::REQUEST_RESULT QueuedAsyncTask::requestCancel(){
    if(this->getState()==OnRequestTask::IDLE){
        this->switchState(OnRequestTask::FAIL);
        this->userCancel = true;
        return ACCEPT;
    }else if(this->getState()==WORKING){
        if(this->taskQueue[this->currentTaskIndex]->requestCancel()==ACCEPT){
            this->switchState(OnRequestTask::FAIL);
            this->userCancel = true;
            return ACCEPT;
        }else{
            this->setRejectReason(tr("QueuedAsyncTask: The current subtask rejected cancel request with reason:")+
                        this->taskQueue[this->currentTaskIndex]->getRefuseReason()+tr(", try it later!"));
            return REJECT;
        }
    }else if(this->getState()==PAUSING){
        if(this->taskQueue[this->currentTaskIndex]->requestCancel()==ACCEPT){
            this->switchState(OnRequestTask::FAIL);
            this->userCancel = true;
            return ACCEPT;
        }else{
            this->setRejectReason(tr("QueuedAsyncTask: The current subtask rejected resume request with reason:")+
                        this->taskQueue[this->currentTaskIndex]->getRefuseReason()+tr(", try it later!"));
            return REJECT;
        }
    }else if(this->getState()==FAIL){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in FAIL state, no need to cancel"));
        return REJECT;
    }else if(this->getState()==SUCCESS){
        this->setRejectReason(tr("QueuedAsyncTask: The task is in SUCCESS state, no need to cancel"));
        return REJECT;
    }else{
        this->setRejectReason(tr("QueuedAsyncTask: The task is in invalid state, seeing this means bugs occurred, report to the developer"));
        return REJECT;
    }
    return REJECT;
}
void QueuedAsyncTask::addTask(QSharedPointer<OnRequestTask> task)
{
    this->taskQueue.append(task);
}
void QueuedAsyncTask::addTask(OnRequestTask* task){
    this->taskQueue.append(QSharedPointer<OnRequestTask>(task));
}
QString QueuedAsyncTask::getCurrentTaskName(){
    return this->currentTaskName;
}

QString QueuedAsyncTask::getTaskName(){
    return "QueuedAsyncTask";
}

int QueuedAsyncTask::reportTotalWorkload(){
    int tot = 0;
    for(auto task:taskQueue){
        tot += task->reportTotalWorkload();
    }
    return tot;
}

void QueuedAsyncTask::run(){

    if(this->taskQueue.isEmpty()){
        this->switchState(OnRequestTask::SUCCESS);
        emit successSignal();
        return;
    }else{
        emit startSignal();
        for(auto task:this->taskQueue){
            this->totalWorkload += task->reportTotalWorkload();
        }

        this->totalProgress = 0;
        for(int i = 0;i<this->taskQueue.size();i++){// this function do not maintain states, just do actions according to the current state
            currentTaskIndex = i;
            currentTaskName = this->taskQueue[i]->getTaskName();
            auto connection1 = connect(this->taskQueue[i].data(),&OnRequestTask::reportProgressSignal,this,&QueuedAsyncTask::subtaskReportProgressSlot);
            auto connection2 = connect(this->taskQueue[i].data(),&OnRequestTask::startSignal,this,&QueuedAsyncTask::subtaskStartSlot);
            auto connection3 = connect(this->taskQueue[i].data(),&OnRequestTask::pauseSignal,this,&QueuedAsyncTask::subtaskPauseSlot);
            auto connection4 = connect(this->taskQueue[i].data(),&OnRequestTask::resumeSignal,this,&QueuedAsyncTask::subtaskResumeSlot);
            auto connection5 = connect(this->taskQueue[i].data(),&OnRequestTask::cancelSignal,this,&QueuedAsyncTask::subtaskCancelSlot);
            auto connection6 = connect(this->taskQueue[i].data(),&OnRequestTask::successSignal,this,&QueuedAsyncTask::subtaskSuccessSlot);
            auto connection7 = connect(this->taskQueue[i].data(),&OnRequestTask::failSignal,this,&QueuedAsyncTask::subtaskFailSlot);
            auto disconnectAll = [=](){disconnect(connection1);disconnect(connection2);disconnect(connection3);disconnect(connection4);disconnect(connection5);disconnect(connection6);disconnect(connection7);};
            if(this->getState() == OnRequestTask::PAUSING){
                emit pauseSignal();
                while(this->getState()==OnRequestTask::PAUSING);
                if(this->getState()==OnRequestTask::IDLE){
                    qWarning()<<QString("QueuedAsyncTask: Invalid state after pausing:%1").arg(this->getState())+", ignore to execute";
                    this->switchState(OnRequestTask::WORKING);
                }
                emit resumeSignal();
            }
            if(this->getState() == OnRequestTask::FAIL){// 2 reasons for failure, user cancel or subtask fail
                //for user cancel, we have to emit a signal here
                disconnectAll();
                if(this->userCancel){
                    emit cancelSignal();
                }
                
                return;
            }
            if(this->getState() == OnRequestTask::SUCCESS){
                qDebug()<<"QueuedAsyncTask: receives a success state but tasks are not finished yet, it's a bug, report to the developer";
                return;
            }
            if(this->getState() == OnRequestTask::WORKING){
                this->taskQueue[i]->requestStart();
                while(this->taskQueue[i]->getState()!=OnRequestTask::SUCCESS&&this->taskQueue[i]->getState()!=OnRequestTask::FAIL){//wait subtasks to finish
                    QThread::msleep(10);
                }
            }else{
                qDebug()<<"QueuedAsyncTask: receive a idel state when executing a task, it's a bug, report to the developer";
            }
            disconnectAll();
        }
    }
    this->switchState(OnRequestTask::SUCCESS);
    emit successSignal();
}
void QueuedAsyncTask::subtaskReportProgressSlot(int value,int total){
    this->totalProgress += value - this->subtaskProgress;
    this->subtaskProgress = value;
    this->subtaskWorkload = total;
    emit reportProgressSignal(this->totalProgress,this->totalWorkload);
    emit reportPrimiaryProgressSignal(this->totalProgress,this->totalWorkload);
    emit reportSecondaryProgressSignal(this->subtaskProgress,this->subtaskWorkload);
}
void QueuedAsyncTask::subtaskStartSlot(){
    
}

void QueuedAsyncTask::subtaskPauseSlot(){
    emit pauseSignal();//this works, but  logically with bugs, even though it happens at a very low possibility
}

void QueuedAsyncTask::subtaskResumeSlot(){
    emit resumeSignal();
}

void QueuedAsyncTask::subtaskCancelSlot(){
    // emit cancelSignal();
}

void QueuedAsyncTask::subtaskSuccessSlot(){
    // leave for future
}

void QueuedAsyncTask::subtaskFailSlot(){
    this->switchState(OnRequestTask::FAIL);
    this->setFailReason(tr("Subtask failed with reason:")+this->taskQueue[currentTaskIndex]->getFailReason());
    emit failSignal(this->failReason);
}
