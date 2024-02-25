#include "io/OpenSlideFileReader.h"
#include "io/MetaFileManager.h"
#include "GlobalResources.h"
#include <QFile>
#include <QDebug>
#include <ncnn/net.h>
#include <QSharedPointer>
#include "openslide/openslide.h"
#include <opencv2/opencv.hpp>
#include "scopeFile/ScopeFileReader.h"
#include <QDir>
#include <zgnUNetForVIM/UNet.h>
#include "ui/mainwindow.h"
#include "ui/AIProcessWindow.h"
#include "ui/RenderThread.h"
#include "scopeFile/ScopeFileReader.h"
#include <QObject>
GlobalSettings globalSettings;
MetaFileManager metaFileManager("./config");
void init(){
    qRegisterMetaType<int64_t>("int64_t");
    if(!metaFileManager.valid()){
        qDebug()<<"Meta file manager is not valid. Initializing a new one";
        QDir dir("./config");
        if(!dir.exists()){
            dir.mkpath("./config");
        }else{
            qDebug()<<"Config directory already exists, but the MetaFileManager is not valid. This is a bug!";
            exit(0);
        }
        metaFileManager.reload();
        if(!metaFileManager.valid()){
            qDebug()<<"Failed to initialize MetaFileManager again. This is a bug!";
            exit(0);
        }
    }
    if(metaFileManager.hasFile("globalSettings.json","settings")){
        auto file = metaFileManager.allocateFile("globalSettings.json",MetaFileManager::META_FILE_TYPE_PERMANENT,"settings");
        file->open(QFile::ReadOnly);
        QString json = QString::fromUtf8(file->readAll());
        globalSettings.loadFromJSONString(json);
        file->close();
        metaFileManager.unlockFile("globalSettings.json","settings");
    }else{
        qDebug()<<"No global settings found, load default settings.";
        globalSettings.loadDefualt();
        auto file = metaFileManager.allocateFile("globalSettings.json",MetaFileManager::META_FILE_TYPE_PERMANENT,"settings");
        file->open(QFile::WriteOnly);
        file->write(globalSettings.dumpToJSONString().toUtf8());
        file->close();
        metaFileManager.unlockFile("globalSettings.json","settings");
    }
}
void cleanUp(){
    qDebug()<<"Cleaning up...";
    ncnn::destroy_gpu_instance();
}

int main(int argc, char *argv[]){
    QApplication a(argc,argv);
    init();
    qDebug()<<"main thread:"<<QThread::currentThreadId();
    // AIProcessWindow w;
    // w.show();
    // RenderThread render;
    
    // QSharedPointer<OpenSlideFileReader> reader(new OpenSlideFileReader(R"(C:\Users\RisingEntropy\Downloads\001961-2a  vim_d_8_20220926195850_m1.svs)"));
    
    MainWindow w;
    w.show();
    QObject::connect(&a, &QApplication::aboutToQuit, &cleanUp);
    return a.exec();
}
