#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QFile>
#include <random>

class Utils{
public:
    static int64_t fashHash(QString fileName);
};

#endif