#ifndef META_FILE_MANAGER_H
#define META_FILE_MANAGER_H
#include <QString>
#include <QMap>
#include <QFile>
class MetaFileManager{
public:
    enum MetaFileType{
        META_FILE_TYPE_TEMPORARY,
        META_FILE_TYPE_PERMANENT
    };
    MetaFileManager(QString metaInfoPath);
    ~MetaFileManager();
    QSharedPointer<QFile> allocateFile(QString name, MetaFileType type, QString group="");
    void lockFile(QString name, QString group);
    void unlockFile(QString name, QString group);
    bool hasFile(QString name, QString group);
    bool isFileLocked(QString name, QString group);
    bool removeFile(QString name, QString group);
    void dumpInfo();
    bool valid();
    void reload();
private:
    bool validManager = false;
    QString metaInfoPath;
    QMap<QString, QSharedPointer<QFile>> files;
    QMap<QString, bool> locks;
    QMap<QString, QSet<QString>> fileGroups;
};
#endif