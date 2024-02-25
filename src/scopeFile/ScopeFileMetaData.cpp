#include "ScopeFileMetaData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
void ScopeFileMetaData::setProperty(QString key, QString value){
    this->properties[key] = value;
}
QString ScopeFileMetaData::getProperty(QString key){
    return this->properties[key];
}
ScopeFileMetaData ScopeFileMetaData::loadFromJSONString(QString &jsonString)
{
    QJsonDocument doc;
    doc.fromJson(jsonString.toUtf8());
    ScopeFileMetaData metadata;
    QJsonObject obj = doc.object();
    metadata.networkVersion = obj["networkVersion"].toString();
    metadata.date = obj["date"].toString();
    metadata.comment = obj["comment"].toString();
    QStringList keys = obj.keys();
    for(QString & key: keys){
        if(key!="height" && key!="width" && key!="version" && key!="patient" && key!="date" && key!="network_version" && key!="comment"){
            metadata.properties[key] = obj[key].toString();
        }
    }
    return metadata;
}

QString ScopeFileMetaData::dumpToJSONString(const ScopeFileMetaData& metadata){
    QJsonObject obj;

    obj["patient"] = metadata.patient;
    obj["date"] = metadata.date;
    obj["network_version"] = metadata.networkVersion;
    obj["comment"] = metadata.comment;
    for(QString & key: metadata.properties.keys()){
        obj[key] = metadata.properties[key];
    }
    QJsonDocument doc(obj);
    return doc.toJson();
}