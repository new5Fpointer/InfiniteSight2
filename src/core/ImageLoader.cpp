#ifdef HAS_LIBVIPS
#include <vips/vips8>
#endif

#include "ExifParser.h"
#include "ImageLoader.h"
#include "common/ImageModel.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QScreen>

#ifdef HAS_LIBVIPS
// vips 进度回调函数（g_signal_connect 需要 C 函数指针）
void ImageLoader::vipsProgressCallback(VipsImage *image, VipsProgress *progress, void *userData) {
    auto *self = static_cast<ImageLoader *>(userData);

    if (self->m_canceled) {
        vips_image_set_kill(image, TRUE);
        return;
    }

    emit self->progress(progress->percent);
}
#endif

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
        emit errorOccurred(m_filePath, m_jobId);
        emit finished(QPixmap(), QStringLiteral("Error: Failed to load image"), m_jobId);
        return;
    }

    emit progress(50);

    ImageInfo info = collectImageInfo();
    info.imageInfo[QStringLiteral("Load Strategy")] = QStringLiteral("Standard (Qt)");

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

    try {
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

        int outWidth = image.width();
        int outHeight = image.height();
        int bands = image.bands();

        qDebug() << "VipsFull loaded:" << outWidth << "x" << outHeight << "bands:" << bands;

        qDebug() << "VipsFull: format=" << vips_enum_nick(VIPS_TYPE_BAND_FORMAT, image.format())
                 << "interpretation=" << vips_enum_nick(VIPS_TYPE_INTERPRETATION, image.interpretation())
                 << "bands=" << bands;

        // 统一转换为 8 位 sRGB 或 sRGBA
        vips::VImage converted = image;

        // 第一步：在原始位深下做色彩空间转换（如有必要）
        // 这一步必须在 cast 之前，因为 colourspace 对高位深数据的处理更准确
        if (bands == 1) {
            // 灰度图 -> sRGB (3 bands)
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            bands = 3;
        } else if (bands == 2) {
            // 灰度+alpha -> 先转 sRGB，再合并 alpha
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            vips::VImage alpha = image.extract_band(1);
            converted = converted.bandjoin(alpha);
            bands = 4;
        } else if (bands == 3) {
            // 检查色彩空间
            VipsInterpretation interp = converted.interpretation();
            if (interp != VIPS_INTERPRETATION_sRGB) {
                qDebug() << "VipsFull: converting 3-band from"
                         << vips_enum_nick(VIPS_TYPE_INTERPRETATION, interp) << "to sRGB";
                converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            }
        } else if (bands == 4) {
            VipsInterpretation interp = converted.interpretation();
            if (interp == VIPS_INTERPRETATION_CMYK) {
                qDebug() << "VipsFull: converting CMYK 4-band to sRGB";
                converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
                bands = 3;
            } else if (interp != VIPS_INTERPRETATION_sRGB) {
                qDebug() << "VipsFull: converting 4-band from"
                         << vips_enum_nick(VIPS_TYPE_INTERPRETATION, interp) << "to sRGB";
                converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            }
        } else if (bands > 4) {
            converted = converted.colourspace(VIPS_INTERPRETATION_sRGB);
            bands = 3;
        }

        // 第二步：降位深到 8-bit
        if (converted.format() != VIPS_FORMAT_UCHAR) {
            qDebug() << "VipsFull: casting from"
                     << vips_enum_nick(VIPS_TYPE_BAND_FORMAT, converted.format()) << "to UCHAR";
            converted = converted.cast(VIPS_FORMAT_UCHAR);
        }

        // 在触发实际计算前，启用 vips 进度回调
        vips_image_set_progress(converted.get_image(), TRUE);
        g_signal_connect(converted.get_image(), "eval",
                         G_CALLBACK(&ImageLoader::vipsProgressCallback), this);

        // 直接写入内存原始像素数据
        // vips 的 sRGB 内存数据是 RGB 顺序，QImage 的 Format_RGB888 也是 RGB 顺序，无需交换
        // write_to_memory 会触发 vips pipeline 的实际计算，进度回调在此期间触发
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

        QPixmap pixmap = QPixmap::fromImage(qimg);

        ImageInfo info = collectVipsImageInfo();
        info.imageInfo[QStringLiteral("Load Strategy")] = QStringLiteral("VipsFull");
        info.imageInfo[QStringLiteral("Dimensions")] = QStringLiteral("%1 x %2").arg(outWidth).arg(outHeight);

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

    info.fileInfo[QStringLiteral("File Name")] = fi.fileName();
    info.fileInfo[QStringLiteral("Path")] = fi.absoluteFilePath();
    info.fileInfo[QStringLiteral("Size")] = QStringLiteral("%1 KB").arg(fi.size() / 1024.0, 0, 'f', 2);
    info.fileInfo[QStringLiteral("Modified")] = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    if (m_perfSettings.skipExif) {
        return info;
    }

    QImageReader reader(m_filePath);
    if (reader.canRead()) {
        info.imageInfo[QStringLiteral("Format")] = reader.format().toUpper();
        info.imageInfo[QStringLiteral("Dimensions")] = QStringLiteral("%1 x %2 pixels")
                                           .arg(reader.size().width())
                                           .arg(reader.size().height());
    }

    // 解析EXIF数据
    ExifParser::ExifData exif = ExifParser::parse(m_filePath);
    auto displayMap = exif.toDisplayMap();
    for (auto it = displayMap.begin(); it != displayMap.end(); ++it) {
        info.exifInfo[it.key()] = it.value();
    }

    return info;
}

ImageInfo ImageLoader::collectVipsImageInfo() {
    ImageInfo info;
    QFileInfo fi(m_filePath);

    info.fileInfo[QStringLiteral("File Name")] = fi.fileName();
    info.fileInfo[QStringLiteral("Path")] = fi.absoluteFilePath();

    QString sizeStr;
    qint64 size = fi.size();
    if (size < 1024) {
        sizeStr = QStringLiteral("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        sizeStr = QStringLiteral("%1 KB").arg(size / 1024.0, 0, 'f', 2);
    } else if (size < 1024LL * 1024 * 1024) {
        sizeStr = QStringLiteral("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        sizeStr = QStringLiteral("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    info.fileInfo[QStringLiteral("Size")] = sizeStr;
    info.fileInfo[QStringLiteral("Modified")] = fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    if (m_perfSettings.skipExif) {
        return info;
    }

#ifdef HAS_LIBVIPS
    try {
        std::string utf8Path = m_filePath.toUtf8().toStdString();
        // 使用 thumbnail 获取信息，避免加载全图
        vips::VImage image = vips::VImage::thumbnail(utf8Path.c_str(), 64);
        info.imageInfo[QStringLiteral("Format")] = QString::fromUtf8(vips_foreign_find_load(utf8Path.c_str()));
        info.imageInfo[QStringLiteral("Dimensions")] = QStringLiteral("%1 x %2 pixels")
                                           .arg(image.width())
                                           .arg(image.height());
        info.imageInfo[QStringLiteral("Bands")] = QString::number(image.bands());
        info.imageInfo[QStringLiteral("Interpretation")] = QString::fromUtf8(vips_enum_nick(VIPS_TYPE_INTERPRETATION, image.interpretation()));
    } catch (...) {
        // 回退到 QImageReader
        QImageReader reader(m_filePath);
        if (reader.canRead()) {
            info.imageInfo[QStringLiteral("Format")] = reader.format().toUpper();
            info.imageInfo[QStringLiteral("Dimensions")] = QStringLiteral("%1 x %2 pixels")
                                               .arg(reader.size().width())
                                               .arg(reader.size().height());
        }
    }
#else
    QImageReader reader(m_filePath);
    if (reader.canRead()) {
        info.imageInfo[QStringLiteral("Format")] = reader.format().toUpper();
        info.imageInfo[QStringLiteral("Dimensions")] = QStringLiteral("%1 x %2 pixels")
                                           .arg(reader.size().width())
                                           .arg(reader.size().height());
    }
#endif

    // 解析EXIF数据
    ExifParser::ExifData exif = ExifParser::parse(m_filePath);
    auto displayMap = exif.toDisplayMap();
    for (auto it = displayMap.begin(); it != displayMap.end(); ++it) {
        info.exifInfo[it.key()] = it.value();
    }

    return info;
}
