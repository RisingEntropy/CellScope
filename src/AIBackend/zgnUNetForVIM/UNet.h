#ifndef UNET_H
#define UNET_H

#include "../Inferable.h"
#include <ncnn/net.h>
class UNet : public Inferable {
public:
    UNet();
    ~UNet();
    UNet(QString paramPath, QString binPath);
    void setParamPath(QString paramPath);
    void setBinPath(QString binPath);
    void loadModel() override;
    void preprocess(cv::Mat &input) override;
    bool infer(cv::Mat& input, cv::Mat& output) override;
    void clear() override;
    bool valid() override;
    QString invalidReason() override;
    QString name() override;
    QString info() override;
    void loadDefualtProperty() override;
    void setGPUEnable(bool) override;
private:
    bool hasGPUInstance = false;
    QString invalidReason_;
    ncnn::Net net;
    QString paramPath,binPath;
    bool valid_ = false;
    bool modelLoaded = false;
};
#endif