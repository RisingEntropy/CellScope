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
    RenderThread(const RenderThread&)=delete;
    RenderThread& operator=(const RenderThread&)=delete;

    void requestRegion(int64_t level, QRectF FOVImage);

    void installTiledImage(QString path);
    void uninstallTiledImage();
    void installScopeFile(QString path);
    void uninstallScopeFile();
    void countCellNums();
    void countCellSize();
    void renderMask(bool render);

private:
    bool working = false;
    int tileSize = 256;
    bool renderMaskFlag = false;
    QThread thread;
    QSharedPointer<ScopeFileReader> scopeFileReader;
    QSharedPointer<OpenSlideFileReader> tiledFileReader;
    bool tiledFileReaderInstalled;
    bool scopeFileReaderInstalled;
    QVector<QMap<std::tuple<int64_t, int64_t, int64_t>, QSharedPointer<QGraphicsPixmapItem> > > cache;
private slots:

    void _requestRegion(int64_t level, QRectF FOVImage);
    void _installTiledImage(QString path);
    void _uninstallTiledImage();
    void _installScopeFile(QString path);
    void _uninstallScopeFile();
    void _countCellNums();
    void _countCellSize();
    void _renderMask(bool);

signals:
    void addTile(QGraphicsPixmapItem *item);
    void removeTile(QGraphicsPixmapItem *item);

    void updateCurrentViewCellSize(int64_t size);
    void updateCurrentViewCellNums(int64_t count);
    void updateScopeFileMetaData(ScopeFileMetaData);
    
    void internalRequestRegion(int64_t level, QRectF FOVImage);



    void internalInstallTiledImage(QString path);
    void internalUninstallTiledImage();
    void internalInstallScopeFile(QString path);
    void internalUninstallScopeFile();
    void internalCountCellNums();
    void internalCountCellSize();
    void internalRenderMask(bool);
};

#endif

