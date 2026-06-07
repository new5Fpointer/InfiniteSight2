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

enum class LoadStrategy {
    Standard,      // QPixmap::load，适用于普通图片
    VipsThumbnail, // libvips 缩略图，适用于超大图片快速预览
    VipsFull,      // libvips 全图加载，适用于超大图片完整查看
    Tile           // 分块加载，适用于巨型图片（未来扩展）
};

struct LoadResult {
    QPixmap pixmap;
    ImageInfo info;
    QString error;
    bool isDownsampled = false;  // 是否已降采样
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

    static LoadStrategy detectStrategy(const QString &filePath, const PerformanceSettings &perf);
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
    void loadVipsThumbnail();
    void loadVipsFull();

    ImageInfo collectImageInfo();
    ImageInfo collectVipsImageInfo();

    QString m_filePath;
    PerformanceSettings m_perfSettings;
    QString m_jobId;
    bool m_canceled = false;
    LoadStrategy m_strategy = LoadStrategy::Standard;
};
