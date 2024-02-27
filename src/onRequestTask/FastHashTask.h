#ifndef FASKHASHTASK_H
#define FASKHASHTASK_H

#include "SyncTask.h"
#include <QFile>
class FastHashTask : public SyncTask{
    Q_OBJECT
public:
    FastHashTask(QString filename);
    virtual REQUEST_RESULT requestPause() override;
    virtual REQUEST_RESULT requestResume() override;
    virtual REQUEST_RESULT requestCancel() override;
    virtual int reportTotalWorkload();
    virtual bool pausable() override;
    virtual QString getTaskName() override;
    virtual void run() override;
    int64_t getHash();
private:
    int64_t hash = 0;
    QFile file;
    QString fileName;
};

#endif