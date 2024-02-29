#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H
#include <QThread>
#include <QPixmap>
#include <QImage>
#include <QSharedPointer>
#include "../scopeFile/ScopeFileReader.h"
#include "../scopeFile/ScopeFileHeader.h"
#include "../scopeFile/ScopeFileMetaData.h"
#include "../io/OpenSlideFileReader.h"
#include <QMap>
#include <QGraphicsPixmapItem>
#include <utility>
class RenderThread:public QObject{
    Q_OBJECT
public:
    RenderThread(int tileSize = 256);
    ~RenderThread() noexcept;
    void stop();
    RenderThread(const RenderThread&)=delete;
    RenderThread& operator=(const RenderThread&)=delete;

    void requestRegion(int64_t level, QRectF FOVImage);

    void installTiledImage(QString path);
    void installTiledImage(QSharedPointer<OpenSlideFileReader> reader);

    void uninstallTiledImage();
    void installScopeFile(QString path);
    void installScopeFile(QSharedPointer<ScopeFileReader> reader);
    void uninstallScopeFile();
    void countCellNums();
    void countCellSize();
    void renderMask(bool render);

private:
    bool working = false;
    int tileSize = 256;
    double maskWeight = 0.5;
    bool renderMaskFlag = false;
    QThread thread;
    QSharedPointer<ScopeFileReader> scopeFileReader = nullptr;
    QSharedPointer<OpenSlideFileReader> tiledFileReader = nullptr;
    bool tiledFileReaderInstalled;
    bool scopeFileReaderInstalled;
    QVector<QMap<std::tuple<int64_t, int64_t, int64_t>, QSharedPointer<QGraphicsPixmapItem> > > cache;
    QVector<QMap<std::tuple<int64_t, int64_t, int64_t>, int64_t> > sizeCache;
    QVector<QMap<std::tuple<int64_t, int64_t, int64_t>, int64_t > > countCache;
private slots:

    void _requestRegion(int64_t level, QRectF FOVImage);
    void _installTiledImage(QString path);
    void _installTiledImagePointer(QSharedPointer<OpenSlideFileReader> reader);
    void _uninstallTiledImage();
    void _installScopeFile(QString path);
    void _installScopeFilePointer(QSharedPointer<ScopeFileReader> reader);
    void _uninstallScopeFile();
    void _countCellNums();
    void _countCellSize();
    void _renderMask(bool);

signals:
    void addTile(QSharedPointer<QGraphicsPixmapItem> item);
    void removeTile(QSharedPointer<QGraphicsPixmapItem> item);

    void updateScopeFileMetaData(ScopeFileMetaData metaData);
    void updateCurrentViewCellSize(int64_t size);
    void updateCurrentViewCellNums(int64_t count);


    void internalRequestRegion(int64_t level, QRectF FOVImage);
    void internalInstallTiledImage(QString path);
    void internalInstallTiledImagePointer(QSharedPointer<OpenSlideFileReader> reader);
    void internalUninstallTiledImage();
    void internalInstallScopeFile(QString path);
    void internalInstallScopeFilePointer(QSharedPointer<ScopeFileReader> reader);
    void internalUninstallScopeFile();
    void internalCountCellNums();
    void internalCountCellSize();
    void internalRenderMask(bool);
};

#endif

