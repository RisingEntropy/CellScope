#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSharedPointer>
#include <QMessageBox>
#include "../io/OpenSlideFileReader.h"
#include "AIProcessWindow.h"
#include <QFileDialog>
#include "../Utils.h"
#include <QDebug>
#include "Config.h"
#if defined(_MSC_VER) && (_MSC_VER >= 1600)    
# pragma execution_character_set("utf-8")    
#endif
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
    ui->setupUi(this);
    connect(ui->mainGraphicsView, &MainGraphicsView::updateCellSize, [this](int64_t size){
        ui->labelCurrentCellArea->setText(QString::number(size));
    });
    connect(ui->mainGraphicsView, &MainGraphicsView::updateFOVSize, [this](int64_t size){
        ui->labelCurrentArea->setText(QString::number(size));
    });
    connect(ui->mainGraphicsView, &MainGraphicsView::updateTotalSize, [this](int64_t size){
        ui->labelTotalArea->setText(QString::number(size));
    });
}

MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::ActionClicked(QAction* action){
    if(action==ui->actionAISegmenter){
        AIProcessWindow* w = new AIProcessWindow(this);
        w->setWindowModality(Qt::ApplicationModal);
        w->show();
    }else if(action==ui->actionOpen){
        QString path = QFileDialog::getOpenFileName(this,"Open file",QString(),"WSI Files(*.svs *.tif *.dcm *.vms *.vmu *.ndpi *.scn *.mrxs *.tiff *.svslide *.bif)");
        if(path.isEmpty()){
            return;
        }
        if(OpenSlideFileReader::validateFile(path)){
            this->tiledImagePath = path;
            turnMask(false);
            ui->mainGraphicsView->setOpenSlideFile(path);
            ui->SelectMastButton->setEnabled(true);
        }else{
            QMessageBox::warning(this,"Error","Invalid file format");
        }
        
    }else if(action==ui->actionPreference){
        QMessageBox::information(this,"Preference","Not implemented yet");
    }else if(action==ui->actionAbout){
        
        QMessageBox::information(this,tr("About"),tr("%1 \nVersion: %2\nAuthor: Haoyu Deng (邓皓宇) \nContact: haoyu_deng@std.uestc.edu.cn").arg(CONFIG_NAME).arg(CONFIG_VERSION));
    
    }
}
void MainWindow::SelectMask(){
    QString path = QFileDialog::getOpenFileName(this,"Open file",QString(),"AI Mask Files(*.aiscope)");
    if(path.isEmpty()){
        return;
    }
    if(ScopeFileUtil::validateFileHeader(path)){
        auto header = ScopeFileUtil::parseHeader(path);
        QString fastHash = header.metaData.getProperty("imageFileFastHash");
        if(fastHash!=QString::number(Utils::fashHash(this->tiledImagePath))){
            QMessageBox::warning(this,"Error","The mask file is not for the current image");
            return;
        }
        if(!header.metaData.getProperty("totalCellSize").isEmpty()){
            ui->labelTotalCellArea->setText(header.metaData.getProperty("totalCellSize"));
        }else{
            ui->labelTotalCellArea->setText("N/A");
        }
        
        ui->mainGraphicsView->setScopeFile(path); 
        ui->checkBoxSegmentationMask->setEnabled(true);
        ui->checkBoxSegmentationMask->setChecked(false);
        ui->checkBoxSegmentationMask->setText(tr("Show Mask"));
        ui->patientNameLabel->setText(header.metaData.patient);
        QDateTime dtime = QDateTime::fromString(header.metaData.date,"yyyy-MM-dd hh:mm:ss");
        ui->dateLabel->setText(dtime.toString("yyyy-MM-dd hh:mm:ss"));
        ui->networkVersionLabel->setText(header.metaData.networkVersion);
        ui->commentTextEdit->setText(header.metaData.comment);
        

    }else{
        QMessageBox::warning(this,"Error","Invalid file format");
    }
    
}

void MainWindow::turnMask(bool on){
    ui->mainGraphicsView->setMaskEnabled(on);
    if(on){
        ui->checkBoxSegmentationMask->setText(tr("Hide Mask"));
    }else{
        ui->checkBoxSegmentationMask->setText(tr("Show Mask"));
        ui->labelCurrentCellArea->setText("N/A");
    }
}
