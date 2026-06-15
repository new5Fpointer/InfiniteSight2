#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

class ExifParser {
public:
    struct ExifData {
        // 设备信息
        QString cameraMake;
        QString cameraModel;
        QString lensMake;
        QString lensModel;
        QString software;
        QString artist;
        QString copyright;

        // 拍摄参数
        QString exposureTime;      // 曝光时间
        QString fNumber;           // 光圈值
        QString iso;               // ISO
        QString focalLength;       // 焦距
        QString focalLength35mm;   // 等效35mm焦距
        QString exposureProgram;   // 曝光程序
        QString meteringMode;      // 测光模式
        QString flash;             // 闪光灯
        QString whiteBalance;      // 白平衡
        QString exposureBias;      // 曝光补偿
        QString exposureMode;      // 曝光模式
        QString sceneCaptureType;  // 场景拍摄类型
        QString dateTimeOriginal;  // 原始拍摄时间
        QString dateTimeDigitized; // 数字化时间
        QString dateTime;          // 修改时间

        // 图像信息
        int width = 0;
        int height = 0;
        int orientation = 0;
        QString colorSpace;
        QString xResolution;
        QString yResolution;
        QString resolutionUnit;
        QString bitsPerSample;
        QString compression;
        QString photometricInterpretation;
        QString imageDescription;
        QString userComment;

        // GPS信息
        QString gpsLatitude;
        QString gpsLongitude;
        QString gpsAltitude;
        QString gpsLatitudeRef;
        QString gpsLongitudeRef;
        QString gpsAltitudeRef;
        QString gpsTimeStamp;
        QString gpsDateStamp;

        // 原始字段（未被分类的）
        QMap<QString, QString> rawFields;

        bool isEmpty() const;

        QMap<QString, QString> toDisplayMap() const;
    };

    static ExifData parse(const QString &filePath);

private:
    static QString formatRational(const QString &rational);
    static QString formatExposureTime(const QString &value);
    static QString formatFNumber(const QString &value);
    static QString formatFocalLength(const QString &value);
    static QString formatExposureBias(const QString &value);
    static QString lookupEnum(const QString &value, const QMap<int, QString> &mapping);
    static QString cleanString(const QString &str);
    static bool isKnownField(const QString &fieldName);
    static QString getFriendlyFieldName(const QString &vipsFieldName);
};
