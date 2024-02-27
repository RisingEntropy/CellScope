#include "GlobalSettings.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "AssiatantUtils.h"
#include <QDebug>
int GlobalSettings::getIntValue(QString key){
    if (this->map.contains(key)){
        return this->map[key].toInt();
    }else{
        qDebug()<<"Try to get a non-exist key: "<<key<<" from GlobalSettings, return 0.";
        return 0;
    }
}
double GlobalSettings::getDoubleValue(QString key){
    if (this->map.contains(key)){
        return this->map[key].toDouble();
    }else{
        qDebug()<<"Try to get a non-exist key: "<<key<<" from GlobalSettings, return 0.0.";
        return 0.0;
    }
}
QString GlobalSettings::getStringValue(QString key){
    if(this->map.contains(key) == false){
        qDebug()<<"Try to get a non-exist key: "<<key<<" from GlobalSettings, return empty string.";
    }
    return this->map[key];
}
void GlobalSettings::setIntValue(QString key, int value){
    this->map[key] = QString::number(value);
}
void GlobalSettings::setDoubleValue(QString key, double value){
    this->map[key] = QString::number(value);
}
void GlobalSettings::setStringValue(QString key, QString value){
    this->map[key] = value;
}
void GlobalSettings::loadFromJSONString(QString jsonString){
    this->map.clear();
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    QJsonObject obj = doc.object();
    for(auto key:obj.keys()){
        this->map[key] = obj[key].toString();
    }
}
QString GlobalSettings::dumpToJSONString(){
    QJsonObject obj;
    for(auto key:this->map.keys()){
        obj[key] = this->map[key];
    }
    QJsonDocument doc(obj);
    return doc.toJson();
}
void GlobalSettings::loadDefualt(){
    this->setIntValue("defaultPatchSize", 512);
    this->setIntValue("defaultCompressLevel", 4);
    this->setStringValue("zgnUNetParamPath", "./models/zgnUNet/ncnn.param");
    this->setStringValue("zgnUNetBinPath","./models/zgnUNet/ncnn.bin");
    this->setStringValue("dropEdgeRatio","0.2");
    this->setIntValue("displayBufferPatchSize", 256);
    this->setDoubleValue("maxZoom", 2.0);
    this->setDoubleValue("minZoom", 1.0/256);
}