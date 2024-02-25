#ifndef SCOPEFILEUTIL_H
#define SCOPEFILEUTIL_H

#include <QFile>
#include <QIODevice>
#include <QString>
#include "../scopeFile/ScopeFileHeader.h"
class ScopeFileUtil{
public:

    static bool validateFileHeader(QString);
    static bool validateFileHeader(QIODevice*);
    // This is slow because it will read the whole file
    static bool thoroughlyValidateFile(QString);
    static bool thoroughlyValidateFile(QIODevice*);
    static const ScopeFileHeader parseHeader(QString);
    static const ScopeFileHeader parseHeader(QIODevice*);
    // Yes its return type is int64_t, if any error occurred, it will return -1, otherwise, return the number of bytes written
    static int64_t writeHeader(QIODevice*, const ScopeFileHeader&);
};


#endif