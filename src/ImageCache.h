#pragma once

#include <QCache>
#include <QMutex>
#include <QPixmap>
#include <QString>

struct CacheEntry {
    QPixmap pixmap;
    QString filePath;
    qint64 fileSize = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    bool isDownsampled = false;
    qint64 timestamp = 0;
};

class ImageCache {
public:
    static ImageCache &instance();

    void setMaxCost(int maxCostMB);
    bool contains(const QString &filePath) const;
    QPixmap getPixmap(const QString &filePath) const;
    CacheEntry getEntry(const QString &filePath) const;
    void insert(const QString &filePath, const QPixmap &pixmap, int originalW = 0, int originalH = 0, bool downsampled = false);
    void remove(const QString &filePath);
    void clear();
    int totalCost() const;

private:
    ImageCache();
    ~ImageCache();
    ImageCache(const ImageCache &) = delete;
    ImageCache &operator=(const ImageCache &) = delete;

    mutable QMutex m_mutex;
    QCache<QString, CacheEntry> m_cache;
    int m_maxCostMB = 512; // 默认 512MB
};
