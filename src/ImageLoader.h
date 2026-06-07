#pragma once

#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QVariant>
#include "SettingsManager.h"

struct ImageInfo {
    QMap<QString, QString> fileInfo;
    QMap<QString, QString> imageInfo;
    QMap<QString, QVariant> exifInfo;
    QString error;
};

struct LoadResult {
    QPixmap pixmap;
    ImageInfo info;
    QString error;
    bool isDownsampled = false;
    int originalWidth = 0;
    int originalHeight = 0;
};

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

    ImageInfo collectImageInfo();
    ImageInfo collectVipsImageInfo();

    QString m_filePath;
    PerformanceSettings m_perfSettings;
    QString m_jobId;
    bool m_canceled = false;
};
