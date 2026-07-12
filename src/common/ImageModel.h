#pragma once

#include <QByteArray>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QVariant>

// 文件/图像/EXIF 元信息
struct ImageInfo {
    QMap<QString, QString> fileInfo;
    QMap<QString, QString> imageInfo;
    QMap<QString, QVariant> exifInfo;
    QString error;
};

// 图像加载原始结果（Backend 输出）
struct LoadResult {
    QPixmap pixmap;
    ImageInfo info;
    QString error;
    bool isDownsampled = false;
    int originalWidth = 0;
    int originalHeight = 0;
};

// 视图状态（缩放/旋转/适配）
struct ViewState {
    double scaleFactor = 1.0;
    bool isFitToWindow = true;
    int rotation = 0; // 0/90/180/270
    bool mirrored = false;
};

// Frontend 展示用视图模型（由 Manager 组装）
struct ImageViewModel {
    QPixmap pixmap;
    QString filePath;
    QString fileName;
    qint64 fileSize = 0;
    int imageWidth = 0;
    int imageHeight = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    bool isDownsampled = false;
    ImageInfo info;
    ViewState viewState;
    int currentIndex = -1;
    int total = 0;
    bool isNull = true;
};

// 通用设置
struct GeneralSettings {
    QString defaultWindowState = QStringLiteral("normal");
    QByteArray windowGeometry;
    bool showInfoPanel = true;
    QStringList recentFiles;
    int maxRecentFiles = 5;
    QString language = QStringLiteral("en_us");
};

// 性能设置
struct PerformanceSettings {
    bool lazyLoading = true;
    bool quickRender = false;
    bool skipExif = false;
    int cacheSize = 100;
};

// 外观设置
struct AppearanceSettings {
    QString uiFont = QStringLiteral("Segoe UI");
    int uiFontSize = 10;
    QString theme = QStringLiteral("dark");
};
