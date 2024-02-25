#ifndef TILEDFILEAIINFERENCETASK_H
#define TILEDFILEAIINFERENCETASK_H
#include <QSharedPointer>
#include "../io/OpenSlideFileReader.h"
#include <Inferable.h>
#include "../scopeFile/ScopeFileWriter.h"
#include "SyncTask.h"
class TiledFileAIInferenceTask:public SyncTask{
    Q_OBJECT
public:
    TiledFileAIInferenceTask(QSharedPointer<OpenSlideFileReader> reader,QSharedPointer<ScopeFileWriter> writer, QSharedPointer<Inferable> inferable, 
                            int patchSize=512, unsigned char compreessLevel=9, double dropEdgeRatio=0.1);
    ~TiledFileAIInferenceTask();
    virtual int reportTotalWorkload() override;
    virtual void run() override;
    virtual QString getTaskName() override;
    int64_t getTotalCellArea();
private:
    inline double getLevelScaleXFromLevel0(int64_t level);
    inline double getLevelScaleYFromLevel0(int64_t level);
    inline int64_t getLevelMapping(int64_t rawLevel);
    inline int64_t getRawLevel(int64_t sortedLevel);
    inline void loadAndRerrangeLevelInfo();
    inline void cropPatch(int64_t x,int64_t y, cv::Mat& result);
    inline void cropMask(int64_t x,int64_t y, cv::Mat& mask);
    bool userCancel = false;
    QVector<std::tuple<int64_t,int64_t,int64_t> > levelInfos;//shit, no native tuple in Qt, use std::tuple instead
    int64_t totalPatches = -1;
    int64_t currentPatches = 0;
    int64_t patchSize;
    unsigned char compressLevel;
    double dropEdgeRatio = 0.1;
    int64_t totalCellArea = 0;
    QSharedPointer<Inferable> inferable;
    QSharedPointer<OpenSlideFileReader> reader;
    QSharedPointer<ScopeFileWriter> writer;
    QVector<QSharedPointer<MaskImage>> levelBuffer;
    
};

#endif