#ifndef SIMPLETASK_H
#define SIMPLETASK_H
#include <functional>
#include "Task.h"
#include <QString>
class SimpleTask:public Task{
public:
    SimpleTask(std::function<Task::TaskResultStatus(QString &)> task,QString name="");
    ~SimpleTask() = default;
    int reportTotoalWorkload() override;
    void work() override;
    bool done() override;
    void pause() override;
    bool pausable() override;
    void forceStop() override;
    QString taskName() override;
private:
    QString failReason;
    void updateProgress() override;
    std::function<Task::TaskResultStatus(QString &)> task;
    QString name;
};

#endif