#include "Inferable.h"
#include "qjsondocument.h"
#include <QJsonDocument>
#include <QJsonObject>
QString Inferable::getProperty(QString key){
    if(this->properties.contains(key)){
        return this->properties[key];
    }else{
        return "";
    
    }
}
void Inferable::setProperty(QString key, QString value){
    this->properties[key] = value;
}
QList<QString> Inferable::getPropertyKeys(){
    return this->properties.keys();
}
void Inferable::setGPUEnable(bool enable){
    this->gpu = enable;
}
bool Inferable::GPUEnabled(){
    return this->gpu;
}
int Inferable::getIntProperty(QString key){
    if(this->properties.contains(key)){
        return this->properties[key].toInt();
    }else{
        return 0;
    }
}
double Inferable::getDoubleProperty(QString key){
    if(this->properties.contains(key)){
        return this->properties[key].toDouble();
    }else{
        return 0;
    }
}
bool Inferable::getBoolProperty(QString key){
    if(this->properties.contains(key)){
        return this->properties[key].toLower()=="true";
    }else{
        return false;
    }
}
void Inferable::setIntProperty(QString key, int value){
    this->properties[key] = QString::number(value);
}
void Inferable::setDoubleProperty(QString key, double value){
    this->properties[key] = QString::number(value);
}
void Inferable::setBoolProperty(QString key, bool value){
    this->properties[key] = value?"true":"false";
}
void Inferable::loadDefualtProperty(){
    // pass
}
void Inferable::loadFromJSON(QString json){
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    for(auto key: obj.keys()){
        this->properties[key] = obj[key].toString();
    }
}
QString Inferable::dumpToJSON(){
    QJsonObject obj;
    for(auto key: this->properties.keys()){
        obj[key] = this->properties[key];
    }
    QJsonDocument doc(obj);
    return doc.toJson();
}
    