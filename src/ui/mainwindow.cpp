#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSharedPointer>
#include "../io/OpenSlideFileReader.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
    ui->setupUi(this);
    this->renderThread = QSharedPointer<RenderThread>(new RenderThread());
    this->renderThread->installTiledImage(R"(C:\Users\RisingEntropy\Downloads\001961-2a  vim_d_8_20220926195850_m1.svs)");
    QSharedPointer<OpenSlideFileReader> reader(new OpenSlideFileReader(R"(C:\Users\RisingEntropy\Downloads\001961-2a  vim_d_8_20220926195850_m1.svs)"));
    ui->mainGraphicsView->setRenderThread(this->renderThread);
    ui->mainGraphicsView->setReader(reader);



}

MainWindow::~MainWindow(){
    delete this->mainGraphicsView;
    delete ui;

}
