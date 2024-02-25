#ifndef ONREQUESTTASK_H
#define ONREQUESTTASK_H
#include <QObject>
class OnRequestTask:public QObject{
    Q_OBJECT
public:
    enum REQUEST_RESULT{
        ACCEPT,
        REJECT
    };
    enum TASK_STATE{
        IDLE,
        WORKING,
        PAUSING,
        SUCCESS,
        FAIL
    };
    virtual REQUEST_RESULT requestStart() = 0;
    virtual REQUEST_RESULT requestPause() = 0;
    virtual REQUEST_RESULT requestResume() = 0;
    virtual REQUEST_RESULT requestCancel() = 0;
    virtual int reportTotalWorkload() = 0;
    virtual bool pausable() = 0;
    virtual QString getTaskName() = 0;
    virtual void run() = 0;
    
    virtual TASK_STATE getState();
    virtual QString getRefuseReason();
    virtual QString getFailReason();
    

protected:
    QString rejectReason;
    QString failReason;
    TASK_STATE state = IDLE;
    void switchState(TASK_STATE targetState);
    void setRejectReason(QString reason);
    void setFailReason(QString reason);
public slots:
    virtual void startSlot();
    virtual void pauseSlot();
    virtual void resumeSlot();
    virtual void cancelSlot();
signals:
    void reportProgressSignal(int value, int total);
    
    void startSignal();
    void pauseSignal();
    void resumeSignal();
    void cancelSignal();
    
    void successSignal();
    void failSignal(QString);
};

#endif // ONREQUESTTASK_H