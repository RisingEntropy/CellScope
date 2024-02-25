#ifndef QUEUEDASYNCTASK_H
#define QUEUEDASYNCTASK_H

#include "AsyncTask.h"
#include <QVector>
class QueuedAsyncTask:public AsyncTask{
    Q_OBJECT
public:
    QueuedAsyncTask() = default;
    ~QueuedAsyncTask();
    virtual REQUEST_RESULT requestPause() override;
    virtual REQUEST_RESULT requestResume() override;
    virtual REQUEST_RESULT requestCancel() override;
    void addTask(QSharedPointer<OnRequestTask> task);
    void addTask(OnRequestTask* task);
    QString getCurrentTaskName();
    virtual QString getTaskName() override;
    int reportTotalWorkload() override;
    virtual void run() override;
private slots:
    void subtaskReportProgressSlot(int value,int total);
    void subtaskStartSlot();
    void subtaskPauseSlot();
    void subtaskResumeSlot();
    void subtaskCancelSlot();

    void subtaskSuccessSlot();
    void subtaskFailSlot();
signals:
    void reportPrimiaryProgressSignal(int value, int total);
    void reportSecondaryProgressSignal(int value, int total);
private:
    bool userCancel = false;
    int totalWorkload = 0;
    int totalProgress = 0;
    int subtaskWorkload = 0;
    int subtaskProgress = 0;
    int currentTaskIndex = 0;
    QString currentTaskName="QueuedAsyncTask";
    QVector<QSharedPointer<OnRequestTask>> taskQueue;
};

#endif