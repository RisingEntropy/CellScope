#include <vcruntime.h>
#ifndef SCOPE_FILE_METADATA_H
#define SCOPE_FILE_METADATA_H
#include <QString>
#include <QMap>
class ScopeFileMetaData{
public:
    QString networkVersion;
    QString patient;
    QString date;
    QString comment;
    QMap<QString,QString> properties;
    void setProperty(QString key, QString value);
    QString getProperty(QString key);
    static ScopeFileMetaData loadFromJSONString(QString& jsonString);
    static QString dumpToJSONString(const ScopeFileMetaData& metadata);
};
#endif