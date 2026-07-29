#pragma once

#include "common/ImageModel.h"
#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QVariant>

#ifdef HAS_LIBVIPS
#include <vips/vips8>
#endif

class ImageLoader : public QObject {
    Q_OBJECT

public:
    explicit ImageLoader(const QString &filePath,
                         const PerformanceSettings &perfSettings,
                         const QString &jobId,
                         QObject *parent = nullptr);

    static bool isSupportedByVips(const QString &filePath);
    static qint64 fileSize(const QString &filePath);

public slots:
    void load();
    void cancel();

signals:
    void finished(const QPixmap &pixmap, const QString &filePath, const QString &jobId);
    void infoReady(const ImageInfo &info, const QString &jobId);
    void progress(int value);
    void loadResultReady(const LoadResult &result, const QString &jobId);

private:
    void loadStandard();
    void loadVipsFull();

#ifdef HAS_LIBVIPS
    static void vipsProgressCallback(VipsImage *image, VipsProgress *progress, void *userData);
#endif

    ImageInfo collectImageInfo();
    ImageInfo collectVipsImageInfo();

    QString m_filePath;
    PerformanceSettings m_perfSettings;
    QString m_jobId;
    bool m_canceled = false;
};
