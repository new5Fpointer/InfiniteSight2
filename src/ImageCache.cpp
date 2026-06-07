#include "ImageCache.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

ImageCache::ImageCache()
    : m_cache(100) { // 最多 100 个条目，cost 由我们自己计算
    m_cache.setMaxCost(m_maxCostMB * 1024 * 1024); // 转换为字节
}

ImageCache::~ImageCache() {
    clear();
}

ImageCache &ImageCache::instance() {
    static ImageCache cache;
    return cache;
}

void ImageCache::setMaxCost(int maxCostMB) {
    QMutexLocker locker(&m_mutex);
    m_maxCostMB = maxCostMB;
    m_cache.setMaxCost(maxCostMB * 1024 * 1024);
}

bool ImageCache::contains(const QString &filePath) const {
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(filePath);
}

QPixmap ImageCache::getPixmap(const QString &filePath) const {
    QMutexLocker locker(&m_mutex);
    CacheEntry *entry = m_cache.object(filePath);
    if (entry) {
        entry->timestamp = QDateTime::currentMSecsSinceEpoch();
        return entry->pixmap;
    }
    return QPixmap();
}

CacheEntry ImageCache::getEntry(const QString &filePath) const {
    QMutexLocker locker(&m_mutex);
    CacheEntry *entry = m_cache.object(filePath);
    if (entry) {
        entry->timestamp = QDateTime::currentMSecsSinceEpoch();
        return *entry;
    }
    return CacheEntry();
}

void ImageCache::insert(const QString &filePath, const QPixmap &pixmap, int originalW, int originalH, bool downsampled) {
    if (pixmap.isNull()) return;

    QMutexLocker locker(&m_mutex);

    // 计算 cost：pixmap 内存占用（近似）
    int cost = pixmap.width() * pixmap.height() * pixmap.depth() / 8;

    CacheEntry *entry = new CacheEntry();
    entry->pixmap = pixmap;
    entry->filePath = filePath;
    entry->fileSize = QFileInfo(filePath).size();
    entry->originalWidth = originalW;
    entry->originalHeight = originalH;
    entry->isDownsampled = downsampled;
    entry->timestamp = QDateTime::currentMSecsSinceEpoch();

    m_cache.insert(filePath, entry, cost);
}

void ImageCache::remove(const QString &filePath) {
    QMutexLocker locker(&m_mutex);
    m_cache.remove(filePath);
}

void ImageCache::clear() {
    QMutexLocker locker(&m_mutex);
    m_cache.clear();
}

int ImageCache::totalCost() const {
    QMutexLocker locker(&m_mutex);
    return m_cache.totalCost();
}
