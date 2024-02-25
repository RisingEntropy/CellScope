#ifndef SEQUENCEDASYNCTASK_H
#define SEQUENCEDASYNCTASK_H
#include "Task.h"
#include <QThread>
#include <QVector>
class SequencedAsyncTask:public Task{
    Q_OBJECT
public:
    SequencedAsyncTask();
    ~SequencedAsyncTask();
    int reportTotoalWorkload() override;
    void work() override;
    bool done() override;
    void pause() override;
    bool pausable() override;
    void forceStop() override;
    QString taskName() override;
    void addTask(QSharedPointer<Task> task);
    void addTask(Task* task);
    QString getFailReason() override;
    QString getCurrentTaskName();
protected:
    void updateProgress() override;
    void run();
private:
    QString failReason;
    QString currentTaskName;
    QThread thread;
    QVector<QSharedPointer<Task> > subtasks;
    int currentTask = -1;
    int totalWorkload = 0;
    int currentProgress = 0;
    int secondaryTotoalWorkload = 0;
    int secondaryCurrentProgress = 0;
signals:
    void updatePrimaryProgressSignal(int currentProgress, int totalWorkload);
    void updateSecondaryProgressSignal(int currentProgress, int totalWorkload);
public slots:
    void subtaskProgressUpdate(int currentProgress, int totalWorkload);
};

#endif