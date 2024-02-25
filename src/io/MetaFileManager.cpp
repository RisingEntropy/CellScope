#include "MetaFileManager.h"
#include "qdir.h"
#include "qjsondocument.h"
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
MetaFileManager::MetaFileManager(QString metaInfoPath){
    this->metaInfoPath = metaInfoPath;
    if(!QFile::exists(metaInfoPath)){
        qCritical()<<"MetaFileManager: Meta info path does not exist. MetaFileManager will not be valid.";
        return;
    }
    this->validManager = true;
    QFile file(metaInfoPath+"/meta.json");
    if(file.exists()){
        file.open(QIODevice::ReadOnly);
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        QJsonObject fileGroups = doc.object();
        for(auto group: fileGroups.keys()){
            QJsonArray files = fileGroups[group].toArray();
            for(auto file: files){
                this->fileGroups[group].insert(file.toString());
                this->locks[group+"/"+file.toString()] = false;
            }
        }
    }
}
void MetaFileManager::reload(){
    if(!QFile::exists(metaInfoPath)){
        qCritical()<<"MetaFileManager: Meta info path does not exist. MetaFileManager will not be valid.";
        return;
    }
    this->validManager = true;
    QFile file(metaInfoPath+"/meta.json");
    if(file.exists()){
        file.open(QIODevice::ReadOnly);
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        QJsonObject fileGroups = doc.object();
        for(auto group: fileGroups.keys()){
            QJsonArray files = fileGroups[group].toArray();
            for(auto file: files){
                this->fileGroups[group].insert(file.toString());
                this->locks[group+"/"+file.toString()] = false;
            }
        }
    }
}
MetaFileManager::~MetaFileManager(){
    for(auto group: this->fileGroups.keys()){
        for(auto file: this->fileGroups[group]){
            if(this->isFileLocked(file, group)){
                this->unlockFile(file, group);
            }
        }
    }
    for(auto file: this->files.keys()){
        if(this->files[file]->isOpen()){
            this->files[file]->close();
        }
    }
    dumpInfo();
}
QSharedPointer<QFile> MetaFileManager::allocateFile(QString name, MetaFileType type, QString group){
    if(!this->validManager){
        qCritical()<<"MetaFileManager: MetaFileManager is not valid. Cannot allocate file.";
        return nullptr;
    }
    if(type==MetaFileType::META_FILE_TYPE_TEMPORARY){
        group = "temp";
    }
    if(group=="temp"&&type==MetaFileType::META_FILE_TYPE_PERMANENT){
        qCritical()<<"MetaFileManager: Cannot allocate permanent file in temporary group.";
        return nullptr;
    }
    if(this->locks.contains(group+"/"+name)&&this->locks[group+"/"+name]){
        qCritical()<<"MetaFileManager: File is already locked. Cannot allocate file.";
        return nullptr;
    }
    if(!this->fileGroups.contains(group)){
        this->fileGroups[group] = QSet<QString>();
    }
    this->fileGroups[group].insert(name);
    if(!QFile::exists(this->metaInfoPath+"/"+group)){
        QDir dir;
        dir.mkpath(this->metaInfoPath+"/"+group);
    }
    QSharedPointer<QFile> file(new QFile(this->metaInfoPath+"/"+group+"/"+name),[this,name,group](QFile* file){
        if(file->isOpen()){
            file->close();
        }
        this->unlockFile(name, group);
        delete file;
    });
    this->lockFile(name, group);
    this->files[group+"/"+name] = file;
    return file;
}
void MetaFileManager::lockFile(QString name, QString group){
    if(!this->validManager){
        qCritical()<<"MetaFileManager: MetaFileManager is not valid. Cannot lock file.";
        return;
    }
    if(!this->fileGroups.contains(group)){
        qCritical()<<"MetaFileManager: Group does not exist. Cannot lock file.";
        return;
    }
    if(!this->fileGroups[group].contains(name)){
        qCritical()<<"MetaFileManager: File does not exist in group. Cannot lock file.";
        return;
    }
    if(this->locks[group+"/"+name]){
        qWarning()<<"MetaFileManager: File is already locked, reduplicated locking means there might be conflicts.";
    }
    this->locks[group+"/"+name] = true;

}
void MetaFileManager::unlockFile(QString name, QString group){
    if(!this->validManager){
        qCritical()<<"MetaFileManager: MetaFileManager is not valid. Cannot unlock file.";
        return;
    }

    if(!this->fileGroups.contains(group)){
        qCritical()<<"MetaFileManager: Group does not exist. Cannot unlock file.";
        return;
    }
    if(!this->fileGroups[group].contains(name)){
        qCritical()<<"MetaFileManager: File does not exist in group. Cannot unlock file.";
        return;
    }
    if((!this->locks.contains(group+"/"+name))||(!this->locks[group+"/"+name])){
        qCritical()<<"MetaFileManager: File is not locked, ignore.";
        return;
    }
    this->locks[group+"/"+name] = false;
    if(this->files[group+"/"+name]->isOpen()){
        this->files[group+"/"+name]->close();
    }
    this->files.remove(group+"/"+name);
    if(group=="temp"){
        this->removeFile(name, group);
    }
}
bool MetaFileManager::hasFile(QString name, QString group){
    return this->fileGroups.contains(group)&&this->fileGroups[group].contains(name);
}
bool MetaFileManager::isFileLocked(QString name, QString group)
{
    if(!this->validManager){
        qCritical()<<"MetaFileManager: MetaFileManager is not valid. Cannot check if file is locked.";
        return false;
    }
    if(!this->fileGroups.contains(group)){
        qCritical()<<"MetaFileManager: Group does not exist. Cannot check if file is locked.";
        return false;
    }
    if(!this->fileGroups[group].contains(name)){
        qCritical()<<"MetaFileManager: File does not exist in group. Cannot check if file is locked.";
        return false;
    }
    if(!this->locks.contains(group+"/"+name)){
        return false;
    }
    return this->locks[group+"/"+name];
}
bool MetaFileManager::removeFile(QString name, QString group){
    if (!this->validManager) {
        qCritical() << "MetaFileManager: MetaFileManager is not valid. Cannot remove file.";
        return false;
    }
    if (!this->fileGroups.contains(group)) {
        qCritical() << "MetaFileManager: Group does not exist. Cannot remove file.";
        return false;
    }
    if (!this->fileGroups[group].contains(name)) {
        qCritical() << "MetaFileManager: File does not exist in group. Cannot remove file.";
        return false;
    }
    if(this->isFileLocked(name, group)){
        qCritical()<<"MetaFileManager: File is locked. Cannot remove file.";
        return false;
    }
    if(QFile::exists(this->metaInfoPath+"/"+group+"/"+name))QFile::remove(this->metaInfoPath+"/"+group+"/"+name);
    this->fileGroups[group].remove(name);
    this->locks.remove(group+"/"+name);
    return true;
}
void MetaFileManager::dumpInfo(){
    if (!this->validManager) {
        return;
    }
    QJsonDocument doc;
    QJsonObject fileGroups;
    for(auto group: this->fileGroups.keys()){
        QJsonArray files;
        for(auto file: this->fileGroups[group]){
            files.append(file);
        }
        fileGroups[group] = files;
    }
    doc.setObject(fileGroups);
    QFile file(this->metaInfoPath+"/meta.json");
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}
bool MetaFileManager::valid(){
    return this->validManager;
}

