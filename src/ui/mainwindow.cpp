#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSharedPointer>
#include <QMessageBox>
#include "../io/OpenSlideFileReader.h"
#include "AIProcessWindow.h"
#include <QFileDialog>
#include "../Utils.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
    ui->setupUi(this);
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
    }
}
void MainWindow::SelectMask(){
    QString path = QFileDialog::getOpenFileName(this,"Open file",QString(),"AI Mask Files(*.aiscope)");
    if(path.isEmpty()){
        return;
    }
    if(ScopeFileUtil::validateFileHeader(path)){
        QString fastHash = ScopeFileUtil::parseHeader(path).metaData.getProperty("imageFileFastHash");
        if(fastHash!=QString::number(Utils::fashHash(this->tiledImagePath))){
            QMessageBox::warning(this,"Error","The mask file is not for the current image");
            return;
        }
        ui->mainGraphicsView->setScopeFile(path); 
        ui->checkBoxSegmentationMask->setEnabled(true);
        ui->checkBoxSegmentationMask->setChecked(false);
        ui->checkBoxSegmentationMask->setText(tr("Show Mask"));
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
    }
}
