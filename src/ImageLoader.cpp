#ifdef HAS_LIBVIPS
#include <vips/vips8>
#endif

#include "ImageLoader.h"
#include "SettingsManager.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QScreen>

// 策略阈值常量
static constexpr qint64 SMALL_FILE_THRESHOLD = 2LL << 20;   // 2 MB
static constexpr qint64 LARGE_FILE_THRESHOLD = 256LL << 20; // 256 MB
static constexpr qint64 HUGE_FILE_THRESHOLD = 1024LL << 20; // 1 GB
static constexpr int MAX_DISPLAY_EDGE = 8192;               // 最大显示边长
static constexpr int THUMBNAIL_EDGE = 4096;                 // 缩略图边长

ImageLoader::ImageLoader(const QString &filePath,
                         const PerformanceSettings &perfSettings,
                         const QString &jobId,
                         QObject *parent)
    : QObject(parent), m_filePath(filePath), m_perfSettings(perfSettings), m_jobId(jobId) {
    m_strategy = detectStrategy(filePath, perfSettings);
}

qint64 ImageLoader::fileSize(const QString &filePath) {
    QFileInfo fi(filePath);
    return fi.exists() ? fi.size() : 0;
}

bool ImageLoader::isSupportedByVips(const QString &filePath) {
#ifdef HAS_LIBVIPS
    QString ext = QFileInfo(filePath).suffix().toLower();
    static const QStringList vipsFormats = {
        "jpg", "jpeg", "png", "tif", "tiff", "webp", "gif", "bmp",
        "avif", "heic", "heif", "jxl", "raw", "cr2", "nef", "dng"};
    return vipsFormats.contains(ext);
#else
    return false;
#endif
}

LoadStrategy ImageLoader::detectStrategy(const QString &filePath, const PerformanceSettings &perf) {
    bool vipsAvailable = isSupportedByVips(filePath);

    if (!vipsAvailable) {
        return LoadStrategy::Standard;
    }

    return LoadStrategy::VipsFull;
}

void ImageLoader::load() {
    if (m_canceled)
        return;

    qDebug() << "Loading image:" << QFileInfo(m_filePath).fileName()
             << "strategy:" << static_cast<int>(m_strategy);

    switch (m_strategy) {
    case LoadStrategy::VipsThumbnail:
        loadVipsThumbnail();
        break;
    case LoadStrategy::VipsFull:
        loadVipsFull();
        break;
    case LoadStrategy::Standard:
    default:
        loadStandard();
        break;
    }
}

void ImageLoader::cancel() {
    m_canceled = true;
}

void ImageLoader::loadStandard() {
    if (m_canceled)
        return;

    emit progress(10);

    QPixmap pixmap;
    bool loaded = pixmap.load(m_filePath);

    if (m_canceled)
        return;

    if (!loaded || pixmap.isNull()) {
        qWarning() << "Failed to load image:" << QFileInfo(m_filePath).fileName();
        emit finished(QPixmap(), QString("Error: Failed to load image"), m_jobId);
        return;
    }

    emit progress(50);

    ImageInfo info = collectImageInfo();
    info.imageInfo["Load Strategy"] = "Standard (Qt)";

    if (m_canceled)
        return;

    emit progress(80);
    emit finished(pixmap, m_filePath, m_jobId);
    emit infoReady(info, m_jobId);
    emit progress(100);

    LoadResult result;
    result.pixmap = pixmap;
    result.info = info;
    result.originalWidth = pixmap.width();
    result.originalHeight = pixmap.height();
    emit loadResultReady(result, m_jobId);

    qDebug() << "Image loaded successfully (Standard):" << QFileInfo(m_filePath).fileName()
             << "size:" << pixmap.width() << "x" << pixmap.height();
}

void ImageLoader::loadVipsThumbnail() {
#ifdef HAS_LIBVIPS
    if (m_canceled)
        return;
    emit progress(10);

    try {
        // Windows 下 vips 需要正斜杠路径
        QString normalizedPath = m_filePath;
        normalizedPath.replace('\\', '/');

        // 检查文件是否存在
        if (!QFileInfo(m_filePath).exists()) {
            qWarning() << "VipsThumbnail: File does not exist:" << m_filePath;
            loadStandard();
            return;
        }

        QByteArray pathBytes = normalizedPath.toUtf8();
        const char *path = pathBytes.constData();

        qDebug() << "VipsThumbnail loading:" << normalizedPath;
        qDebug() << "VipsThumbnail path bytes length:" << pathBytes.length();

        // 使用 C API 直接调用，避免 C++ 包装器的问题
        VipsImage *vipsImage = nullptr;
        if (vips_thumbnail(path, &vipsImage, THUMBNAIL_EDGE, NULL) != 0) {
            const char *err = vips_error_buffer();
            qWarning() << "VipsThumbnail C API failed:" << err;
            vips_error_clear();
            loadStandard();
            return;
        }

        vips::VImage image(vipsImage);

        if (m_canceled)
            return;
        emit progress(50);

        int origWidth = image.width();
        int origHeight = image.height();

        // 写入 PNG 内存缓冲区，然后构造 QImage
        size_t bufSize = 0;
        void *buf = nullptr;
        image.write_to_buffer(".png", &buf, &bufSize);

        QImage qimg;
        if (buf && bufSize > 0) {
            qimg = QImage::fromData(static_cast<const uchar *>(buf), static_cast<int>(bufSize));
            g_free(buf);
        }

        if (qimg.isNull()) {
            qWarning() << "Vips thumbnail: QImage from buffer failed, falling back";
            loadStandard();
            return;
        }

        if (m_canceled)
            return;
        emit progress(80);

        QPixmap pixmap = QPixmap::fromImage(qimg);

        ImageInfo info = collectVipsImageInfo();
        info.imageInfo["Load Strategy"] = "VipsThumbnail (Downsampled)";
        info.imageInfo["Original Dimensions"] = QString("%1 x %2").arg(origWidth).arg(origHeight);
        info.imageInfo["Display Dimensions"] = QString("%1 x %2").arg(qimg.width()).arg(qimg.height());

        emit finished(pixmap, m_filePath, m_jobId);
        emit infoReady(info, m_jobId);
        emit progress(100);

        LoadResult result;
        result.pixmap = pixmap;
        result.info = info;
        result.isDownsampled = true;
        result.originalWidth = origWidth;
        result.originalHeight = origHeight;
        emit loadResultReady(result, m_jobId);

        qDebug() << "Image loaded successfully (VipsThumbnail):" << QFileInfo(m_filePath).fileName()
                 << "original:" << origWidth << "x" << origHeight
                 << "display:" << qimg.width() << "x" << qimg.height();

    } catch (const vips::VError &e) {
        qWarning() << "Vips thumbnail load failed:" << e.what() << "- falling back to standard";
        loadStandard();
    }
#else
    loadStandard();
#endif
}

void ImageLoader::loadVipsFull() {
#ifdef HAS_LIBVIPS
    if (m_canceled)
        return;
    emit progress(10);

    try {
        // Windows 下 vips 需要正斜杠路径
        QString normalizedPath = m_filePath;
        normalizedPath.replace('\\', '/');

        // 检查文件是否存在
        if (!QFileInfo(m_filePath).exists()) {
            qWarning() << "VipsFull: File does not exist:" << m_filePath;
            loadStandard();
            return;
        }

        QByteArray pathBytes = normalizedPath.toUtf8();
        const char *path = pathBytes.constData();

        qDebug() << "VipsFull loading:" << normalizedPath;

        // 使用 C API 加载全图，不缩放
        VipsImage *vipsImage = vips_image_new_from_file(path, NULL);
        if (!vipsImage) {
            const char *err = vips_error_buffer();
            qWarning() << "VipsFull C API failed:" << err;
            vips_error_clear();
            loadStandard();
            return;
        }

        vips::VImage image(vipsImage);

        if (m_canceled)
            return;
        emit progress(50);

        int outWidth = image.width();
        int outHeight = image.height();

        qDebug() << "VipsFull loaded:" << outWidth << "x" << outHeight;

        // 写入 PNG 内存缓冲区，然后构造 QImage
        size_t bufSize = 0;
        void *buf = nullptr;
        image.write_to_buffer(".png", &buf, &bufSize);

        if (!buf || bufSize == 0) {
            qWarning() << "VipsFull write_to_buffer failed";
            loadStandard();
            return;
        }

        // vips::VImage 会自动释放 vipsImage，不需要手动 g_object_unref

        QImage qimg;
        if (buf && bufSize > 0) {
            qimg = QImage::fromData(static_cast<const uchar *>(buf), static_cast<int>(bufSize));
            g_free(buf);
        }

        if (qimg.isNull()) {
            qWarning() << "Vips full: QImage from buffer failed, falling back";
            loadStandard();
            return;
        }

        if (m_canceled)
            return;
        emit progress(80);

        QPixmap pixmap = QPixmap::fromImage(qimg);

        ImageInfo info = collectVipsImageInfo();
        info.imageInfo["Load Strategy"] = "VipsFull";
        info.imageInfo["Dimensions"] = QString("%1 x %2").arg(outWidth).arg(outHeight);

        emit finished(pixmap, m_filePath, m_jobId);
        emit infoReady(info, m_jobId);
        emit progress(100);

        LoadResult result;
        result.pixmap = pixmap;
        result.info = info;
        result.isDownsampled = false;
        result.originalWidth = outWidth;
        result.originalHeight = outHeight;
        emit loadResultReady(result, m_jobId);

        qDebug() << "Image loaded successfully (VipsFull):" << QFileInfo(m_filePath).fileName()
                 << "size:" << outWidth << "x" << outHeight;

    } catch (const vips::VError &e) {
        qWarning() << "Vips full load failed:" << e.what() << "- falling back to standard";
        loadStandard();
    }
#else
    loadStandard();
#endif
}

ImageInfo ImageLoader::collectImageInfo() {
    ImageInfo info;
    QFileInfo fi(m_filePath);

    info.fileInfo["File Name"] = fi.fileName();
    info.fileInfo["Path"] = fi.absoluteFilePath();
    info.fileInfo["Size"] = QString("%1 KB").arg(fi.size() / 1024.0, 0, 'f', 2);
    info.fileInfo["Modified"] = fi.lastModified().toString("yyyy-MM-dd hh:mm:ss");

    if (m_perfSettings.skipExif) {
        return info;
    }

    QImageReader reader(m_filePath);
    if (reader.canRead()) {
        info.imageInfo["Format"] = reader.format().toUpper();
        info.imageInfo["Dimensions"] = QString("%1 x %2 pixels")
                                           .arg(reader.size().width())
                                           .arg(reader.size().height());
        info.imageInfo["DPI"] = QString("N/A");
    }

    return info;
}

ImageInfo ImageLoader::collectVipsImageInfo() {
    ImageInfo info;
    QFileInfo fi(m_filePath);

    info.fileInfo["File Name"] = fi.fileName();
    info.fileInfo["Path"] = fi.absoluteFilePath();

    QString sizeStr;
    qint64 size = fi.size();
    if (size < 1024) {
        sizeStr = QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else if (size < 1024LL * 1024 * 1024) {
        sizeStr = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        sizeStr = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    info.fileInfo["Size"] = sizeStr;
    info.fileInfo["Modified"] = fi.lastModified().toString("yyyy-MM-dd hh:mm:ss");

    if (m_perfSettings.skipExif) {
        return info;
    }

#ifdef HAS_LIBVIPS
    try {
        std::string utf8Path = m_filePath.toUtf8().toStdString();
        // 使用 thumbnail 获取信息，避免加载全图
        vips::VImage image = vips::VImage::thumbnail(utf8Path.c_str(), 64);
        info.imageInfo["Format"] = QString::fromUtf8(vips_foreign_find_load(utf8Path.c_str()));
        info.imageInfo["Dimensions"] = QString("%1 x %2 pixels")
                                           .arg(image.width())
                                           .arg(image.height());
        info.imageInfo["Bands"] = QString::number(image.bands());
        info.imageInfo["Interpretation"] = QString::fromUtf8(vips_enum_nick(VIPS_TYPE_INTERPRETATION, image.interpretation()));
    } catch (...) {
        // 回退到 QImageReader
        QImageReader reader(m_filePath);
        if (reader.canRead()) {
            info.imageInfo["Format"] = reader.format().toUpper();
            info.imageInfo["Dimensions"] = QString("%1 x %2 pixels")
                                               .arg(reader.size().width())
                                               .arg(reader.size().height());
        }
    }
#else
    QImageReader reader(m_filePath);
    if (reader.canRead()) {
        info.imageInfo["Format"] = reader.format().toUpper();
        info.imageInfo["Dimensions"] = QString("%1 x %2 pixels")
                                           .arg(reader.size().width())
                                           .arg(reader.size().height());
    }
#endif

    return info;
}
