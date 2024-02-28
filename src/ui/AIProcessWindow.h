#ifndef AIPROCESSWINDOW_H
#define AIPROCESSWINDOW_H
#include <QMainWindow>
#include <QWidget>
#include <QString>
#include <QSharedPointer>
#include "ui_AIProcessWindow.h"
#include <QTimer>
#include <QDateTime>
#include "../onRequestTask/QueuedAsyncTask.h"
namespace Ui {
class AIProcessWindow;
}

class AIProcessWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AIProcessWindow(QWidget *parent = 0);
    ~AIProcessWindow();
    void closeEvent( QCloseEvent * event )override;
private slots:
    void openImageFile();
    void chooseSavePatch();
    void start();
    void pause();
    void cancel();
    void updatePrimirayProgress(int value, int total);
    void updateSecondaryProgress(int value, int total);
    void taskStart();
    void taskFail(QString reason);
    void taskSuccess();
    void taskPause();
    void taskResume();
    void taskCancel();
    void timerOut();
    void cleanUP();
    
private:
    int primiaryTotal;
    int primiaryNow;
    int secondaryTotal;
    int secondaryNow;
    int64_t timeUsed;
    Ui::AIProcessWindow *ui;
    QString imagePath;
    QString savePath;
    QSharedPointer<QueuedAsyncTask> task;
    QTimer timer;
    QDateTime startTime;
    bool hasConnection = false;
    QMetaObject::Connection conn1;
    QMetaObject::Connection conn2;
    QMetaObject::Connection conn3;
    QMetaObject::Connection conn4;
    QMetaObject::Connection conn5;
    QMetaObject::Connection conn6;
    QMetaObject::Connection conn7;
    QMetaObject::Connection conn8;
};

#endif// AIPROCESSWINDOW_H