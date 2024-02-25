#ifndef INFERABLE_H
#define INFERABLE_H
#include "opencv2/opencv.hpp"
#include <QString>
#include <QList>
#include <QMap>
class Inferable {
public:
    virtual ~Inferable() = default;
    virtual void loadModel() = 0;
    virtual void preprocess(cv::Mat &input) = 0;
    virtual bool infer(cv::Mat& input, cv::Mat& output) = 0;
    virtual void clear() = 0;
    virtual bool valid() = 0;
    virtual QString invalidReason() = 0;
    virtual QString name() = 0;
    virtual QString info() = 0;
    virtual void setGPUEnable(bool);
    virtual bool GPUEnabled();
    virtual QString getProperty(QString key);
    virtual int getIntProperty(QString key);
    virtual double getDoubleProperty(QString key);
    virtual bool getBoolProperty(QString key);
    virtual void setIntProperty(QString key, int value);
    virtual void setDoubleProperty(QString key, double value);
    virtual void setBoolProperty(QString key, bool value);
    virtual void loadDefualtProperty();
    virtual void loadFromJSON(QString json);
    virtual QString dumpToJSON();
    virtual void setProperty(QString key, QString value);
    virtual QList<QString> getPropertyKeys();
protected:

    QMap<QString, QString> properties;
    bool gpu = false;
};

#endif