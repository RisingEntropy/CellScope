#include "SequencedAsyncTask.h"
#include <QDebug>
SequencedAsyncTask::SequencedAsyncTask(){
    this->switchStatus(TaskStatus::READY);
}

SequencedAsyncTask::~SequencedAsyncTask(){
    
}
int SequencedAsyncTask::reportTotoalWorkload(){
    int total = 0;
    for(auto task:subtasks){
        total += task->reportTotoalWorkload();
    }
    return total;
}
void SequencedAsyncTask::work(){
    if(this->getStatus()==TaskStatus::READY){
        if(this->subtasks.size()==0){
            qWarning()<<"Trying to execute an empty task";
            this->switchStatus(TaskStatus::STOPPED);
            this->currentProgress = 1; //make sure later progress processing won't crash
            this->totalWorkload = 1;
            this->secondaryCurrentProgress = 1;
            this->secondaryTotoalWorkload = 1;
            this->updateProgress();
            emit finishSignal();
        }else{
            this->switchStatus(TaskStatus::WORKING);
            QObject::connect(&thread,&QThread::started,this,&SequencedAsyncTask::run);
            QObject::connect(this,&SequencedAsyncTask::finishSignal,&thread,&QThread::quit);
            this->moveToThread(&thread);
            this->setResultStatus(TaskResultStatus::RESULT_WORKING);
            thread.start();
        }
        
    }else if(this->getStatus()==TaskStatus::PAUSED){
        this->switchStatus(TaskStatus::WORKING);
    }else{
        qWarning()<<"SequencedAsyncTask: Cannot work, current status:"<<this->getStatus()<<" is not READY or PAUSED.";
    }
    
}
bool SequencedAsyncTask::done(){
    return this->getStatus()==TaskStatus::STOPPED;
}
void SequencedAsyncTask::pause(){
    if(this->getStatus()==TaskStatus::WORKING){
        this->subtasks[this->currentTask]->pause();
        this->switchStatus(TaskStatus::PAUSED);
    }else if(this->getStatus()==TaskStatus::PAUSED){
        qWarning()<<"SequencedAsyncTask: Try to pause a already paused task, ignore request.";
    }else{
        qWarning()<<"SequencedAsyncTask: Cannot pause, current status:"<<this->getStatus()<<" is not WORKING.";
    }
}
bool SequencedAsyncTask::pausable(){
    return true;
}
void SequencedAsyncTask::forceStop(){
    this->subtasks[this->currentTask]->forceStop();
    this->failReason = "User canceled.";
    this->setResultStatus(TaskResultStatus::RESULT_FAILED);
    this->switchStatus(TaskStatus::STOPPED);
    this->thread.quit();
}
void SequencedAsyncTask::updateProgress(){
    emit updateProgressSignal(this->currentProgress, this->totalWorkload);
    emit updatePrimaryProgressSignal(this->currentProgress, this->totalWorkload);
    emit updateSecondaryProgressSignal(this->secondaryCurrentProgress, this->secondaryTotoalWorkload);
}
void SequencedAsyncTask::addTask(QSharedPointer<Task> task){
    if(this->getStatus()==TaskStatus::READY){
        this->subtasks.append(QSharedPointer<Task>(task));
    }else{
        qWarning()<<"SequencedAsyncTask: Cannot add task to a non-ready task, ignore it";
    }       
}
void SequencedAsyncTask::addTask(Task *task){
    if(this->getStatus()==TaskStatus::READY){
        this->subtasks.append(QSharedPointer<Task>(task));
    }else{
        qWarning()<<"SequencedAsyncTask: Cannot add task to a non-ready task, ignore it";
    }       
}
QString SequencedAsyncTask::getFailReason(){
    return this->failReason;
}
void SequencedAsyncTask::run(){
    for(auto task:subtasks){
        this->totalWorkload += task->reportTotoalWorkload();
    }
    this->currentTask = 0;
    for(auto task:subtasks){
        auto connection = QObject::connect(task.data(), &Task::updateProgressSignal, this, &SequencedAsyncTask::subtaskProgressUpdate);
        this->secondaryTotoalWorkload = task->reportTotoalWorkload();
        this->secondaryCurrentProgress = 0;
        this->currentTaskName = task->taskName();
        if(this->getStatus()==TaskStatus::WORKING){
            task->work();
            if(task->getResultStatus()!=TaskResultStatus::RESULT_SUCCESS){
                qWarning()<<"SequencedAsyncTask: Receive a failure when executing a subtask:" << task->taskName() << " with reason:" << task->getFailReason();
                this->switchStatus(TaskStatus::FAILED);
                this->failReason = task->getFailReason();
                this->setResultStatus(TaskResultStatus::RESULT_FAILED);
                QObject::disconnect(connection);
                return;
            }
            QObject::disconnect(connection);
            this->currentTask++;
        }else if(this->getStatus()==TaskStatus::PAUSED){
            while(this->getStatus()==TaskStatus::PAUSED);//wait
            if(this->getStatus()==TaskStatus::WORKING){
                task->work();
                if(task->getResultStatus()!=TaskResultStatus::RESULT_SUCCESS){
                    qWarning()<<"SequencedAsyncTask: Receive a failure when executing a subtask:" << task->taskName() << " with reason:" << task->getFailReason() << " after recovery from a pause.";
                    this->switchStatus(TaskStatus::FAILED);
                    this->failReason = task->getFailReason();
                    this->setResultStatus(TaskResultStatus::RESULT_FAILED);
                    QObject::disconnect(connection);
                    return;
                }
                QObject::disconnect(connection);
                this->currentTask++;
            }else{
                qWarning()<<"SequencedAsyncTask: Invalid status when executing subtask, current status:"<<this->getStatus()<<" is not WORKING or PAUSED after recovery from a pause.";
                this->switchStatus(TaskStatus::FAILED);
                this->failReason = "Invalid status when executing subtask";
                this->setResultStatus(TaskResultStatus::RESULT_FAILED);
                QObject::disconnect(connection);
                return;
            }
        }else{
            qWarning()<<"SequencedAsyncTask: Invalid status when executing subtask, current status:"<<this->getStatus()<<" is not WORKING or PAUSED.";
            this->switchStatus(TaskStatus::FAILED);
            this->failReason = "Invalid status when executing subtask";
            this->setResultStatus(TaskResultStatus::RESULT_FAILED);
            QObject::disconnect(connection);
            return;
        }
    }
    this->setResultStatus(TaskResultStatus::RESULT_SUCCESS);
    this->switchStatus(TaskStatus::STOPPED);
    emit finishSignal();
}

QString SequencedAsyncTask::taskName(){
    return QObject::tr("Sequenced Asynchronous Task");
}
void SequencedAsyncTask::subtaskProgressUpdate(int currentProgress, int totalWorkload){
    this->currentProgress+=currentProgress-this->secondaryCurrentProgress;
    this->secondaryCurrentProgress = currentProgress;
    this->secondaryTotoalWorkload = totalWorkload;
    this->updateProgress();
}
QString SequencedAsyncTask::getCurrentTaskName(){
    return this->currentTaskName;
}