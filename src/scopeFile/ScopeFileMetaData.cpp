#include "ScopeFileMetaData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
void ScopeFileMetaData::setProperty(QString key, QString value){
    this->properties[key] = value;
}
QString ScopeFileMetaData::getProperty(QString key)const{
    if(!this->properties.contains(key)){
        return QString();
    }
    return this->properties[key];
}
ScopeFileMetaData ScopeFileMetaData::loadFromJSONString(QString &jsonString)
{
    QJsonDocument doc;
    doc = QJsonDocument::fromJson(jsonString.toUtf8());
    ScopeFileMetaData metadata;
    QJsonObject obj = doc.object();

    metadata.patient = obj["patient"].toString();
    metadata.date = obj["date"].toString();
    metadata.networkVersion = obj["networkVersion"].toString();
    metadata.comment = obj["comment"].toString();
    QStringList keys = obj.keys();
    
    for(QString & key: keys){
        metadata.properties[key] = obj[key].toString();
    }
    return metadata;
}

QString ScopeFileMetaData::dumpToJSONString(const ScopeFileMetaData& metadata){
    QJsonObject obj;

    obj["patient"] = metadata.patient;
    obj["date"] = metadata.date;
    obj["networkVersion"] = metadata.networkVersion;
    obj["comment"] = metadata.comment;
    for(QString & key: metadata.properties.keys()){
        obj[key] = metadata.properties[key];
    }
    QJsonDocument doc(obj);
    return doc.toJson();
}