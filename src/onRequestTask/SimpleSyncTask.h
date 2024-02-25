#ifndef SIMPLESYNCTASK_H
#define SIMPLESYNCTASK_H
#include <functional>
#include "SyncTask.h"
class SimpleSyncTask:public SyncTask{
    Q_OBJECT
public:
    SimpleSyncTask(std::function<OnRequestTask::TASK_STATE(QString& failReason)> task,QString taskName="");
    ~SimpleSyncTask();
    virtual REQUEST_RESULT requestPause() override;
    virtual REQUEST_RESULT requestResume() override;
    virtual REQUEST_RESULT requestCancel() override;
    virtual int reportTotalWorkload();
    virtual bool pausable() override;
    virtual QString getTaskName() override;
    virtual void run() override;

private:
    std::function<OnRequestTask::TASK_STATE(QString& failReason)> task;
    QString taskName;

};

#endif