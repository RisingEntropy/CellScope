#ifndef AIINFERTASK_H
#define AIINFERTASK_H
#include "Task.h"
#include "../io/OpenSlideFileReader.h"
#include <Inferable.h>
#include "../scopeFile/ScopeFileWriter.h"
#include <QVector>
#include <QSharedPointer>
#include <utility>
class AIInferTask:public Task{
    Q_OBJECT
public:
    AIInferTask(QSharedPointer<OpenSlideFileReader> reader,QSharedPointer<ScopeFileWriter> writer, QSharedPointer<Inferable> inferable, int patchSize = 512, unsigned char compressLevel = 4);
    ~AIInferTask();
    int reportTotoalWorkload() override;
    void work() override;
    bool done() override;
    void pause() override;
    bool pausable() override;
    void forceStop() override;
    QString taskName() override;
    QString getFailReason();
    int64_t getTotalCellArea();
protected:
    void updateProgress() override;
private:
    inline double getLevelScaleXFromLevel0(int64_t level);
    inline double getLevelScaleYFromLevel0(int64_t level);
    inline int64_t getLevelMapping(int64_t rawLevel);
    inline int64_t getRawLevel(int64_t sortedLevel);
    inline void loadAndRerrangeLevelInfo();
    QVector<std::tuple<int64_t,int64_t,int64_t> > levelInfos;//shit, no native tuple in Qt, use std::tuple instead
    int64_t totalPatches = -1;
    int64_t currentPatches = 0;
    int64_t patchSize;
    unsigned char compressLevel;
    int64_t totalCellArea = 0;
    QString failReason;
    QSharedPointer<Inferable> inferable;
    QSharedPointer<OpenSlideFileReader> reader;
    QSharedPointer<ScopeFileWriter> writer;
    QVector<QSharedPointer<MaskImage>> levelBuffer;
};

#endif