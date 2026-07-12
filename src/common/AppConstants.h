#pragma once

#include <QStringList>

namespace AppConstants {

inline QStringList supportedImageExtensions() {
    return QStringList{QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
                       QStringLiteral("bmp"), QStringLiteral("gif"), QStringLiteral("tiff"),
                       QStringLiteral("tif"), QStringLiteral("webp")};
}

inline QString imageFilterString() {
    return QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp)");
}

inline int defaultCacheSizeMB() { return 512; }
inline int defaultMaxCacheEntries() { return 100; }

} // namespace AppConstants
