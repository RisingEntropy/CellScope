#include "AIProcessWindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <AIUtility.h>
#include <Inferable.h>
#include <zgnUNetForVIM/UNet.h>
#include "../GlobalResources.h"
#include "../onRequestTask/SimpleSyncTask.h"
#include "../onRequestTask/QueuedAsyncTask.h"
#include "../onRequestTask/TiledFileAIInferenceTask.h"
#include "../onRequestTask/FastHashTask.h"
#include "../Utils.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QComboBox>
AIProcessWindow::AIProcessWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AIProcessWindow){
    ui->setupUi(this);
    if(AIUtility::hasGPU()){
        ui->GPUCheckBox->setEnabled(true);
    }
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->comboBox->addItems(modelZoo.getModelList());
    QObject::connect(&this->timer, &QTimer::timeout, this, &AIProcessWindow::timerOut);
}
AIProcessWindow::~AIProcessWindow(){
    delete ui;
}
void AIProcessWindow::closeEvent(QCloseEvent *event){
    if(!this->task.isNull()){
        if(this->task->getState()==OnRequestTask::WORKING){
            QMessageBox::warning(this, tr("Warning"), tr("Task is still running, please wait or cancel it!"));
            event->ignore();
            return;
        }
    }
    this->cleanUP();
    event->accept();
}
void AIProcessWindow::openImageFile(){
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    this->imagePath = QFileDialog::getOpenFileName(this, tr("Open Image"), desktop, tr("WSI Files(*.svs *.tif *.dcm *.vms *.vmu *.ndpi *.scn *.mrxs *.tiff *.svslide *.bif)"));
    if(this->imagePath==""){
        QMessageBox::warning(this, tr("Warning"), tr("No image file selected!"));
        this->ui->imageFilePathLabel->setText("");
        return;
    }
    this->ui->imageFilePathLabel->setText(this->imagePath);
    if(this->savePath!=""){
        ui->startButton->setEnabled(true);
    }
}
void AIProcessWindow::chooseSavePatch(){
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    this->savePath = QFileDialog::getSaveFileName(this, tr("Save Image"), desktop, tr("AI Scope File(*.aiscope)"));
    if(this->savePath==""){
        QMessageBox::warning(this, tr("Warning"), tr("No save path selected!"));
        this->ui->maskFilePathLabel->setText("");
        return;
    }
    this->ui->maskFilePathLabel->setText(this->savePath);
    if(this->imagePath!=""){
        ui->startButton->setEnabled(true);
    }
}
void AIProcessWindow::start(){
    this->task.reset(new QueuedAsyncTask());
    primiaryNow = 0;
    primiaryTotal = 0;
    secondaryNow = 0;
    secondaryTotal = 0;
    timeUsed = 0;
    ui->primiaryProgressBar->setValue(0);
    ui->secondaryProgressBar->setValue(0);
    ui->primiaryProgressBar->setStyleSheet("");
    ui->secondaryProgressBar->setStyleSheet("");

    if(ui->patientLineEdit->text().isEmpty()){
        QMessageBox::warning(this, tr("Warning"), tr("Patient name is empty!"));
        return;
    }

    QSharedPointer<OpenSlideFileReader> reader(new OpenSlideFileReader(this->imagePath));
    QSharedPointer<ScopeFileWriter> writer(new ScopeFileWriter(this->savePath));
    QSharedPointer<Inferable> inferable = modelZoo.getModel(ui->comboBox->currentText());
    inferable->setGPUEnable(ui->GPUCheckBox->isChecked());
    this->task->addTask(new SimpleSyncTask([inferable, writer, this](QString& failReason){
        inferable->loadModel();
        if(inferable->valid()){
            writer->getHeader().metaData.setProperty("patient", this->ui->patientLineEdit->text());
            writer->getHeader().metaData.setProperty("date", this->ui->dateTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss"));
            writer->getHeader().metaData.setProperty("networkVersion", inferable->name());
            writer->getHeader().metaData.setProperty("comment", this->ui->textEdit->toPlainText());
            return OnRequestTask::SUCCESS;
        }else{
            failReason = "AI model is not valid, reason: "+inferable->invalidReason();
            return OnRequestTask::FAIL;
        }
    }, "Initializing AI model:"+inferable->name()+"..."));
    QSharedPointer<TiledFileAIInferenceTask> aiInferTask(new TiledFileAIInferenceTask(reader, writer, inferable, 
                                            globalSettings.getIntValue("defaultPatchSize"), globalSettings.getIntValue("defaultCompressLevel"),
                                            globalSettings.getDoubleValue("dropEdgeRatio")));

    this->task->addTask(aiInferTask);

    this->task->addTask(new SimpleSyncTask([writer, reader, inferable,this, aiInferTask](QString& failReason){
        writer->getHeader().metaData.setProperty("imageFileFastHash", QString::number(Utils::fashHash(this->imagePath)));
        writer->getHeader().metaData.setProperty("totalCellSize", QString::number(aiInferTask->getTotalCellArea()));
        if(writer->finish()){
            return OnRequestTask::SUCCESS;
        }else{
            failReason = "Writing result to file failed.";
            return OnRequestTask::FAIL;
        }
        writer->close();
    }, "Writing result to file..."));

    if(this->hasConnection){
        QObject::disconnect(this->conn1);
        QObject::disconnect(this->conn2);
        QObject::disconnect(this->conn3);
        QObject::disconnect(this->conn4);
        QObject::disconnect(this->conn5);
        QObject::disconnect(this->conn6);
        QObject::disconnect(this->conn7);
    }
    hasConnection = true;
    this->conn1 = connect(this->task.data(),&QueuedAsyncTask::reportPrimiaryProgressSignal,this,&AIProcessWindow::updatePrimirayProgress);
    this->conn2 = connect(this->task.data(),&QueuedAsyncTask::reportSecondaryProgressSignal,this,&AIProcessWindow::updateSecondaryProgress);
    this->conn3 = connect(this->task.data(),&QueuedAsyncTask::startSignal,this,&AIProcessWindow::taskStart);
    this->conn4 = connect(this->task.data(),&QueuedAsyncTask::failSignal,this,&AIProcessWindow::taskFail);
    this->conn5 = connect(this->task.data(),&QueuedAsyncTask::successSignal,this,&AIProcessWindow::taskSuccess);
    this->conn6 = connect(this->task.data(),&QueuedAsyncTask::pauseSignal,this,&AIProcessWindow::taskPause);
    this->conn7 = connect(this->task.data(),&QueuedAsyncTask::resumeSignal,this,&AIProcessWindow::taskResume);
    this->conn8 = connect(this->task.data(),&QueuedAsyncTask::cancelSignal,this,&AIProcessWindow::taskCancel);
    this->startTime = QDateTime::currentDateTime();
    if(this->task->requestStart()==OnRequestTask::REJECT){
        QMessageBox::warning(this, tr("Reject"), tr("Launching fail!")+this->task->getRefuseReason(), QMessageBox::Ok);
        QObject::disconnect(this->conn1);
        QObject::disconnect(this->conn2);
        QObject::disconnect(this->conn3);
        QObject::disconnect(this->conn4);
        QObject::disconnect(this->conn5);
        QObject::disconnect(this->conn6);
        QObject::disconnect(this->conn7);
        QObject::disconnect(this->conn8);
        hasConnection = false;
        return;
    }
    this->ui->pauseButton->setEnabled(true);
    this->ui->cancelButton->setEnabled(true);
    this->ui->startButton->setEnabled(false);
}
void AIProcessWindow::pause(){
    if(this->task->getState()==OnRequestTask::WORKING){
        if(this->task->requestPause()==OnRequestTask::ACCEPT){
            qDebug()<<"Pause request accepted";
        }else{
            QMessageBox::warning(this, tr("Reject"), tr("Pausing request rejected, it's now not pausable, try it later!"), QMessageBox::Ok);
        }
    }else if(this->task->getState()==OnRequestTask::PAUSING){
        if(this->task->requestResume()==OnRequestTask::ACCEPT){
            //do nothing, pass it to slot function
        }else{
            QMessageBox::warning(this, tr("Reject"), tr("Resuming request rejected, it's maybe a bug, please restart the software"), QMessageBox::Ok);
        }
    }else{
        QMessageBox::warning(this, tr("Error"), tr("Error, invalid state when try to pause current task!"), QMessageBox::Ok);
    }
}
void AIProcessWindow::cancel(){
    if(this->task->requestCancel()==OnRequestTask::ACCEPT){

    }else{
        QMessageBox::warning(this, tr("Reject"), tr("Cancel request rejected, it's not not cancelable, try it later"), QMessageBox::Ok);
    }
    
}

void AIProcessWindow::updatePrimirayProgress(int value, int total){
    this->primiaryNow = value;
    this->primiaryTotal = total;
    ui->primiaryProgressBar->setMaximum(total);
    ui->primiaryProgressBar->setValue(value);
    ui->currentTaskNameLabel->setText(this->task->getCurrentTaskName());
}

void AIProcessWindow::updateSecondaryProgress(int value, int total){
    this->secondaryNow = value;
    this->secondaryTotal = total;
    ui->secondaryProgressBar->setMaximum(total);
    ui->secondaryProgressBar->setValue(value);
}

void AIProcessWindow::taskStart(){
    this->timer.start(300);
}

void AIProcessWindow::taskFail(QString reason){
    this->timer.stop();
    primiaryNow = primiaryTotal = 0;
    secondaryNow = secondaryTotal = 0;
    this->ui->primiaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    this->ui->secondaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    ui->currentTaskNameLabel->setText(tr("Task failed, reason: ")+reason);
    ui->primiaryProgressBar->setStyleSheet("QProgressBar::chunk{background:red}");
    ui->secondaryProgressBar->setStyleSheet("QProgressBar::chunk{background:red}");
    this->task.clear();
    QFile file(this->savePath);
    if(file.exists()){
        file.remove();
    }
    this->ui->cancelButton->setEnabled(false);
    this->ui->pauseButton->setEnabled(false);
    this->ui->startButton->setEnabled(true);   
    QMessageBox::information(this, tr("Fail"), tr("Task failed, reason: ")+reason, QMessageBox::Ok);
}

void AIProcessWindow::taskSuccess(){
    this->task.clear();
    this->timer.stop();
    this->ui->primiaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    this->ui->secondaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    primiaryNow = primiaryTotal = 0;
    secondaryNow = secondaryTotal = 0;
    ui->primiaryProgressBar->setValue(0);
    ui->secondaryProgressBar->setValue(0);
    ui->currentTaskNameLabel->setText(tr("Task finished."));
    ui->primiaryProgressBar->setStyleSheet("");
    ui->secondaryProgressBar->setStyleSheet("");
    QMessageBox::information(this, tr("Success"), tr("Complete!"), QMessageBox::Ok);
    this->ui->cancelButton->setEnabled(false);
    this->ui->pauseButton->setEnabled(false);
    this->ui->startButton->setEnabled(true);   
}


void AIProcessWindow::taskPause(){
    this->timer.stop();
    this->ui->pauseButton->setText(tr("Resume"));
    ui->currentTaskNameLabel->setText(tr("Task paused."));
    QMessageBox::information(this, tr("Pause"), tr("Task paused."), QMessageBox::Ok);
}

void AIProcessWindow::taskResume(){
    this->timer.start(300);
    this->ui->pauseButton->setText(tr("Pause"));
    ui->currentTaskNameLabel->setText(tr("Task resuming."));
}

void AIProcessWindow::taskCancel(){
    this->timer.stop();
    primiaryNow = primiaryTotal = 0;
    secondaryNow = secondaryTotal = 0;
    this->ui->primiaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    this->ui->secondaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
    ui->currentTaskNameLabel->setText(tr("Task canceled."));
    ui->primiaryProgressBar->setStyleSheet("QProgressBar::chunk{background:red}");
    ui->secondaryProgressBar->setStyleSheet("QProgressBar::chunk{background:red}");
    this->task.clear();
    QFile file(this->savePath);
    if(file.exists()){
        file.remove();
    }else{
        qDebug()<<"File not exists";
    }
    this->ui->cancelButton->setEnabled(false);
    this->ui->pauseButton->setEnabled(false);
    this->ui->startButton->setEnabled(true);   
    QMessageBox::information(this, tr("Cancel"), tr("Task canceled"), QMessageBox::Ok);
}

void AIProcessWindow::timerOut(){
    QDateTime now = QDateTime::currentDateTime();
    int64_t secsTillNow = this->startTime.secsTo(now);
    timeUsed += secsTillNow-timeUsed;
    if(this->primiaryTotal==0){
        this->ui->primiaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
        this->ui->secondaryTimeRemainingLabel->setText("--:--:-- / --:--:--");
        return;
    }

    double primiaryPercentage = (double)this->primiaryNow/this->primiaryTotal;
    double secondaryPercentage = (double)this->secondaryNow/this->secondaryTotal;
    int64_t primiaryTimeRemaining = (int64_t)(secsTillNow/primiaryPercentage*(1-primiaryPercentage));
    int64_t secondaryTimeRemaining = (int64_t)(secsTillNow/secondaryPercentage*(1-secondaryPercentage));
    this->ui->primiaryTimeRemainingLabel->setText(
        QString("%1:%2:%3 / %4:%5:%6").arg(timeUsed/3600, 2, 10, QLatin1Char('0')).arg((timeUsed%3600)/60, 2, 10, QLatin1Char('0')).arg(timeUsed%60, 2, 10, QLatin1Char('0'))
        .arg(primiaryTimeRemaining/3600, 2, 10, QLatin1Char('0')).arg((primiaryTimeRemaining%3600)/60, 2, 10, QLatin1Char('0')).arg(primiaryTimeRemaining%60, 2, 10, QLatin1Char('0'))
    );
    this->ui->secondaryTimeRemainingLabel->setText(
        QString("%1:%2:%3 / %4:%5:%6").arg(timeUsed/3600, 2, 10, QLatin1Char('0')).arg((timeUsed%3600)/60, 2, 10, QLatin1Char('0')).arg(timeUsed%60, 2, 10, QLatin1Char('0'))
        .arg(secondaryTimeRemaining/3600, 2, 10, QLatin1Char('0')).arg((secondaryTimeRemaining%3600)/60, 2, 10, QLatin1Char('0')).arg(secondaryTimeRemaining%60, 2, 10, QLatin1Char('0'))
    );

}

void AIProcessWindow::cleanUP(){
    if(!this->task.isNull()){
        this->task->requestCancel();
    }
    this->timer.stop();
}
