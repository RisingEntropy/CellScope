#ifndef ASYNCTASK_H
#define ASYNCTASK_H

#include <QThread>
#include "OnRequestTask.h"
class AsyncTask:public OnRequestTask{
    Q_OBJECT
public:
    ~AsyncTask();
    virtual REQUEST_RESULT requestStart() override;
    virtual REQUEST_RESULT requestPause() override;
    virtual REQUEST_RESULT requestResume() override;
    virtual REQUEST_RESULT requestCancel() override;
    virtual bool pausable();
    virtual QString getTaskName() = 0;
    virtual void run() override;
protected:
    QThread thread;
};

#endif