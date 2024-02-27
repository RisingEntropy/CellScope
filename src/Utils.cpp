#include "Utils.h"
#include <QDataStream>
int64_t Utils::fashHash(QString fileName){
    int64_t hash = 0;
    QFile file;
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    std::default_random_engine e;
    e.seed(file.size());
    QDataStream stream(&file);
    if(file.size()<sizeof(int64_t)){
        stream.readRawData((char*)&hash,file.size());
        file.close();
        return hash;
    }
    int64_t tmp;
    for(int i = 0;i<1024;i++){
        int64_t pos = e();
        file.seek(pos);
        stream>>tmp;
        hash^=(tmp^e());
    }
    return hash;
}
