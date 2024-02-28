#include "ModelZoo.h"
#include "GlobalResources.h"
void ModelZoo::loadModelZoo(){

}

bool ModelZoo::modelZooLoaded(){
    return true;
}

QList<QString> ModelZoo::getModelList(){
    this->modelList.clear();
    this->modelList.append("zgnUNetForVIM");
    return this->modelList;
}

QSharedPointer<Inferable> ModelZoo::getModel(QString modelName){
    if(modelName == "zgnUNetForVIM"){
        return QSharedPointer<Inferable>(new zgnUNetForVIM::UNet(globalSettings.getStringValue("zgnUNetParamPath"), globalSettings.getStringValue("zgnUNetBinPath")));
    }else{
        return QSharedPointer<Inferable>(nullptr);
    }
}
