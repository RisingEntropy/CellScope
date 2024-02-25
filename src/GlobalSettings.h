#ifndef GLOBAL_SETTINGS_H
#define GLOBAL_SETTINGS_H
#include "qmap.h"
#include <QMap>

class GlobalSettings{
public:
    int getIntValue(QString key);
    double getDoubleValue(QString key);
    QString getStringValue(QString key);
    void setIntValue(QString key, int value);
    void setDoubleValue(QString key, double value);
    void setStringValue(QString key, QString value);
    void loadFromJSONString(QString jsonString);
    void loadDefualt();
    QString dumpToJSONString();
private:
    QMap<QString, QString> map;
};
extern GlobalSettings globalSettings;
#endif