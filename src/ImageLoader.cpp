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

ImageLoader::ImageLoader(const QString &filePath,
                         const PerformanceSettings &perfSettings,
                         const QString &jobId,
                         QObject *parent)
    : QObject(parent), m_filePath(filePath), m_perfSettings(perfSettings), m_jobId(jobId) {
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

void ImageLoader::load() {
    if (m_canceled)
        return;

    qDebug() << "Loading image:" << QFileInfo(m_filePath).fileName();

    if (isSupportedByVips(m_filePath)) {
        loadVipsFull();
    } else {
        loadStandard();
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
        int bands = image.bands();

        qDebug() << "VipsFull loaded:" << outWidth << "x" << outHeight << "bands:" << bands;

        // 统一转换为 8 位 sRGB 或 sRGBA
        vips::VImage converted = image;
        if (image.format() != VIPS_FORMAT_UCHAR) {
            converted = converted.cast(VIPS_FORMAT_UCHAR);
        }

        if (bands == 1) {
            // 灰度图 -> RGB (3 bands)
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            bands = 3;
        } else if (bands == 2) {
            // 灰度+alpha -> RGBA (4 bands)
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            // 提取 alpha 通道并合并
            vips::VImage alpha = image.extract_band(1);
            converted = converted.bandjoin(alpha);
            bands = 4;
        } else if (bands == 3) {
            // RGB -> 保持
        } else if (bands == 4) {
            // RGBA -> 保持
        } else if (bands > 4) {
            // CMYK 等 -> 转 sRGB
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            bands = 3;
        }

        // 直接写入内存原始像素数据
        // vips 的 sRGB 内存数据是 RGB 顺序，QImage 的 Format_RGB888 也是 RGB 顺序，无需交换
        size_t memSize = 0;
        void *memBuf = vips_image_write_to_memory(converted.get_image(), &memSize);
        if (!memBuf || memSize == 0) {
            qWarning() << "VipsFull write_to_memory failed";
            loadStandard();
            return;
        }

        // 根据 bands 选择 QImage 格式
        int finalBands = converted.bands();
        QImage::Format qFormat;
        int bytesPerPixel;
        if (finalBands == 3) {
            qFormat = QImage::Format_RGB888;
            bytesPerPixel = 3;
        } else if (finalBands == 4) {
            qFormat = QImage::Format_RGBA8888;
            bytesPerPixel = 4;
        } else {
            qWarning() << "VipsFull: unsupported bands:" << finalBands;
            g_free(memBuf);
            loadStandard();
            return;
        }

        // vips 内存数据没有行对齐，QImage 需要指定正确的 bytesPerLine
        int vipsBytesPerLine = outWidth * bytesPerPixel;
        QImage qimg(static_cast<const uchar *>(memBuf), outWidth, outHeight, vipsBytesPerLine, qFormat);
        // 深拷贝（QImage 使用共享数据，需要确保内存释放后数据仍然有效）
        qimg = qimg.copy();
        g_free(memBuf);

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
