#ifndef SYNCTASK_H
#define SYNCTASK_H

#include "OnRequestTask.h"
class SyncTask:public OnRequestTask{
    Q_OBJECT
public:
    virtual REQUEST_RESULT requestStart() override;
    virtual REQUEST_RESULT requestPause() override;
    virtual REQUEST_RESULT requestResume() override;
    virtual REQUEST_RESULT requestCancel() override;
    virtual bool pausable();
};
#endif