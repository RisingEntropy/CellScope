#ifndef MODELZOO_H
#define MODELZOO_H

#include <QString>
#include <QList>
#include <Inferable.h>
#include <zgnUNetForVIM/UNet.h>
#include <QSharedPointer>
class ModelZoo{
public:
    void loadModelZoo();
    bool modelZooLoaded();
    QList<QString> getModelList();
    QSharedPointer<Inferable> getModel(QString modelName);
private:
    QList<QString> modelList;

};
#endif