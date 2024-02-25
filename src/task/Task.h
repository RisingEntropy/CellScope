#ifndef TASK_H
#define TASK_H
#include <QObject>
#include <QString>
#include <QMutex>
#include <QDebug>
class Task:public QObject{
    Q_OBJECT
public:
    enum TaskStatus{
        NOT_READY,
        READY,
        WORKING,
        PAUSED,
        FAILED,
        STOPPED
    };
    enum TaskResultStatus{
        RESULT_UNKNOWN,
        RESULT_SUCCESS,
        RESULT_FAILED,
        RESULT_WORKING
    };
    virtual ~Task() = default;
    virtual int reportTotoalWorkload() = 0;
    virtual void work() = 0;
    virtual bool done() = 0;
    virtual void pause() = 0;
    virtual bool pausable() = 0;
    virtual void forceStop() = 0;
    virtual QString taskName() = 0;
    virtual QString getFailReason();
    virtual void resume();

    virtual TaskResultStatus getResultStatus();
    virtual TaskStatus getStatus();
protected:
    virtual void setResultStatus(TaskResultStatus status);
    virtual void switchStatus(TaskStatus status);
    Task::TaskStatus status = TaskStatus::READY;
    Task::TaskResultStatus resultStatus = TaskResultStatus::RESULT_UNKNOWN;
    QMutex mutex;
    virtual void updateProgress() = 0;
signals:
    void taskFailSignal(QString failReason);
    void updateProgressSignal(int currentProgress, int totalWorkload);
    void finishSignal();
    void pauseSignal();
    void resumeSignal();
};

#endif