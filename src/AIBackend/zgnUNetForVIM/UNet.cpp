#include "UNet.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "qdebug.h"
#include <ncnn/net.h>
#include <ncnn/mat.h>
#include <QDebug>
#include <QObject>
#include <vcruntime_string.h>
zgnUNetForVIM::UNet::UNet(){
    this->paramPath = "";
    this->binPath = "";
    loadDefualtProperty();
}
zgnUNetForVIM::UNet::UNet(QString paramPath, QString binPath){
    this->paramPath = paramPath;
    this->binPath = binPath;
    loadDefualtProperty();
}
void zgnUNetForVIM::UNet::setParamPath(QString paramPath){
    this->paramPath = paramPath;
}
void zgnUNetForVIM::UNet::setBinPath(QString binPath){
    this->binPath = binPath;
}
void zgnUNetForVIM::UNet::loadModel(){
    if(this->net.load_param(this->paramPath.toStdString().c_str())){
        this->valid_ = false;
        this->invalidReason_ = "Failed to load param file.";
        return;
    }
    if(this->net.load_model(this->binPath.toStdString().c_str())){
        this->valid_ = false;
        this->invalidReason_ = "Failed to load bin file.";
        return;
    }
    this->valid_ = true;
    this->modelLoaded = true;
}
void zgnUNetForVIM::UNet::preprocess(cv::Mat &input){
}
bool zgnUNetForVIM::UNet::infer(cv::Mat& input, cv::Mat& output){
    if(input.type()!=CV_8UC3){
        qWarning()<<"UNet: wrong input format";
        return false;
    }
    if(!this->modelLoaded){
        qWarning()<<"UNet: Model not loaded";
        return false;
    }
    ncnn::Mat in;
    if(this->getProperty("doResize").toLower()=="true"){
        if(this->getIntProperty("resizeWidth")<=0||this->getIntProperty("resizeHeight")<=0){
            return false;
        }else {
            in = ncnn::Mat::from_pixels_resize(input.data, ncnn::Mat::PIXEL_RGB2RGBA, input.cols, input.rows, this->getIntProperty("resizeWidth"), this->getIntProperty("resizeHeight"));
        }
    }else{
        in = ncnn::Mat::from_pixels(input.data, ncnn::Mat::PIXEL_RGB2RGBA, input.cols, input.rows);
    }
    
    
    float norm[4] = {float(this->getDoubleProperty("RNormalize")), float(this->getDoubleProperty("GNormalize")), 
                     float(this->getDoubleProperty("BNormalize")), float(this->getDoubleProperty("ANormalize"))};
    in.substract_mean_normalize(0, norm);
    ncnn::Extractor ex = this->net.create_extractor();
    if(this->gpu){
        ex.set_vulkan_compute(true);
    }
    if(this->getBoolProperty("lightMode")){
        ex.set_light_mode(true);
    }
    ncnn::Mat out;
    ex.input("input", in);
    ex.extract("output", out);
    cv::Mat result(out.h, out.w, CV_32FC1, out.data);
    cv::threshold(result, result, this->getDoubleProperty("threshold"),9999, cv::THRESH_BINARY);
    result.convertTo(output, CV_8UC1);
    return true;
}
void zgnUNetForVIM::UNet::clear(){
    //clean ncnn network
    this->net.clear();
    this->modelLoaded = false;
    this->valid_ = false;
}
bool zgnUNetForVIM::UNet::valid(){
    return this->valid_;
}
QString zgnUNetForVIM::UNet::invalidReason(){
    return this->invalidReason_;
}
QString zgnUNetForVIM::UNet::name(){
    return QString("zgnUNetForVIM");
}
QString zgnUNetForVIM::UNet::info(){
    return QString(QObject::tr("VIM stin"));
}
void zgnUNetForVIM::UNet::loadDefualtProperty(){
    this->setBoolProperty("doResize", false);
    this->setDoubleProperty("RNormalize", 1/255.);
    this->setDoubleProperty("GNormalize", 1/255.);
    this->setDoubleProperty("BNormalize", 1/255.);
    this->setDoubleProperty("ANormalize", 1/255.);
    this->setBoolProperty("lightMode", false);
    this->setDoubleProperty("threshold", 1.5);
}
void zgnUNetForVIM::UNet::setGPUEnable(bool enable){
    if(this->modelLoaded){
        qWarning()<<"UNet: Please set GPU enable before loading model.";
    }else{
        this->net.opt.use_vulkan_compute = enable;
        this->gpu = enable;
        this->hasGPUInstance = true;
    }
}
zgnUNetForVIM::UNet::~UNet(){
    if(this->modelLoaded){
        this->clear();
    }
    if(this->hasGPUInstance){
        ncnn::destroy_gpu_instance();
    }
}