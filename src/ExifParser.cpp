#ifdef HAS_LIBVIPS
#include <vips/vips8>
#endif

// 直接使用 libexif 解析 exif-data blob
#ifdef UNTAGGED_EXIF
#include <exif-data.h>
#include <exif-ifd.h>
#include <exif-loader.h>
#include <exif-utils.h>
#else /*!UNTAGGED_EXIF*/
#include <libexif/exif-data.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>
#endif /*UNTAGGED_EXIF*/

#include "ExifParser.h"
#include <QDebug>
#include <QFileInfo>

bool ExifParser::ExifData::isEmpty() const {
    return cameraMake.isEmpty() && cameraModel.isEmpty() && lensMake.isEmpty() &&
           lensModel.isEmpty() && software.isEmpty() && exposureTime.isEmpty() &&
           fNumber.isEmpty() && iso.isEmpty() && focalLength.isEmpty() &&
           dateTimeOriginal.isEmpty() && artist.isEmpty() && copyright.isEmpty() &&
           gpsLatitude.isEmpty() && gpsLongitude.isEmpty() && width == 0 && height == 0 &&
           rawFields.isEmpty();
}

QMap<QString, QString> ExifParser::ExifData::toDisplayMap() const {
    QMap<QString, QString> result;

    if (!cameraMake.isEmpty() || !cameraModel.isEmpty()) {
        QString make = cameraMake.trimmed();
        QString model = cameraModel.trimmed();
        QString camera;
        // 避免 Make 和 Model 重复前缀（如 "Canon" + "Canon EOS R5"）
        if (make.length() > 1 && model.startsWith(make + " ", Qt::CaseInsensitive)) {
            camera = model;
        } else {
            camera = make;
            if (!model.isEmpty()) {
                if (!make.isEmpty())
                    camera += " ";
                camera += model;
            }
        }
        result["Camera"] = camera;
    }
    if (!lensMake.isEmpty() || !lensModel.isEmpty()) {
        QString lens = lensMake.trimmed();
        if (!lensModel.isEmpty()) {
            if (!lens.isEmpty())
                lens += " ";
            lens += lensModel.trimmed();
        }
        result["Lens"] = lens;
    }
    if (!software.isEmpty())
        result["Software"] = software;

    if (!exposureTime.isEmpty())
        result["Exposure Time"] = exposureTime;
    if (!fNumber.isEmpty())
        result["Aperture"] = fNumber;
    if (!iso.isEmpty())
        result["ISO"] = iso;
    if (!focalLength.isEmpty()) {
        QString fl = focalLength;
        if (!focalLength35mm.isEmpty())
            fl += " (35mm: " + focalLength35mm + ")";
        result["Focal Length"] = fl;
    } else if (!focalLength35mm.isEmpty()) {
        result["Focal Length"] = focalLength35mm;
    }
    if (!exposureProgram.isEmpty())
        result["Exposure Program"] = exposureProgram;
    if (!meteringMode.isEmpty())
        result["Metering Mode"] = meteringMode;
    if (!flash.isEmpty())
        result["Flash"] = flash;
    if (!whiteBalance.isEmpty())
        result["White Balance"] = whiteBalance;
    if (!exposureBias.isEmpty())
        result["Exposure Bias"] = exposureBias;
    if (!exposureMode.isEmpty())
        result["Exposure Mode"] = exposureMode;
    if (!sceneCaptureType.isEmpty())
        result["Scene Capture Type"] = sceneCaptureType;
    if (!dateTimeOriginal.isEmpty())
        result["Date Time Original"] = dateTimeOriginal;
    if (!dateTimeDigitized.isEmpty() && dateTimeDigitized != dateTimeOriginal)
        result["Date Time Digitized"] = dateTimeDigitized;
    if (!dateTime.isEmpty() && dateTime != dateTimeOriginal)
        result["Date Time"] = dateTime;

    if (width > 0 && height > 0)
        result["Dimensions"] = QString("%1 x %2").arg(width).arg(height);
    if (orientation > 0)
        result["Orientation"] = QString::number(orientation);
    if (!colorSpace.isEmpty())
        result["Color Space"] = colorSpace;
    if (!xResolution.isEmpty() && !yResolution.isEmpty()) {
        QString res = xResolution + " x " + yResolution;
        if (!resolutionUnit.isEmpty())
            res += " " + resolutionUnit;
        result["Resolution"] = res;
    }
    if (!bitsPerSample.isEmpty())
        result["Bits Per Sample"] = bitsPerSample;
    if (!compression.isEmpty())
        result["Compression"] = compression;
    if (!imageDescription.isEmpty())
        result["Description"] = imageDescription;
    if (!userComment.isEmpty())
        result["User Comment"] = userComment;

    if (!gpsLatitude.isEmpty() || !gpsLongitude.isEmpty()) {
        QString gps = gpsLatitudeRef + " " + gpsLatitude + ", " +
                      gpsLongitudeRef + " " + gpsLongitude;
        if (!gpsAltitude.isEmpty())
            gps += ", Alt: " + gpsAltitude + gpsAltitudeRef;
        result["GPS"] = gps.trimmed();
    } else if (!gpsTimeStamp.isEmpty() || !gpsDateStamp.isEmpty()) {
        QString gpsTime = gpsDateStamp;
        if (!gpsTimeStamp.isEmpty()) {
            if (!gpsTime.isEmpty())
                gpsTime += " ";
            gpsTime += gpsTimeStamp;
        }
        result["GPS Time"] = gpsTime;
    }

    if (!artist.isEmpty())
        result["Artist"] = artist;
    if (!copyright.isEmpty())
        result["Copyright"] = copyright;

    for (auto it = rawFields.begin(); it != rawFields.end(); ++it) {
        if (!result.contains(it.key()) && !it.value().isEmpty())
            result[it.key()] = it.value();
    }

    return result;
}

QString ExifParser::formatRational(const QString &rational) {
    if (rational.isEmpty())
        return rational;
    int slashPos = rational.indexOf('/');
    if (slashPos > 0) {
        bool numOk = false, denOk = false;
        double num = rational.left(slashPos).trimmed().toDouble(&numOk);
        double den = rational.mid(slashPos + 1).trimmed().toDouble(&denOk);
        if (numOk && denOk && den != 0) {
            double result = num / den;
            if (result == static_cast<int>(result) && result >= -10000 && result <= 10000)
                return QString::number(static_cast<int>(result));
            return QString::number(result, 'f', 2);
        }
    }
    return rational;
}

QString ExifParser::formatExposureTime(const QString &value) {
    if (value.isEmpty())
        return value;
    int slashPos = value.indexOf('/');
    if (slashPos > 0) {
        bool numOk = false, denOk = false;
        double num = value.left(slashPos).trimmed().toDouble(&numOk);
        double den = value.mid(slashPos + 1).trimmed().toDouble(&denOk);
        if (numOk && denOk && den != 0 && num != 0) {
            double result = num / den;
            if (result < 1) {
                int denominator = static_cast<int>(den / num + 0.5);
                if (denominator > 0)
                    return QString("1/%1 sec").arg(denominator);
            } else {
                return QString("%1 sec").arg(result, 0, 'f', 1);
            }
        }
    }
    return value;
}

QString ExifParser::formatFNumber(const QString &value) {
    if (value.isEmpty())
        return value;
    // libexif 可能已经返回 "f/2.8" 这样的格式
    if (value.startsWith("f/", Qt::CaseInsensitive))
        return value;
    QString formatted = formatRational(value);
    bool ok = false;
    double fNum = formatted.toDouble(&ok);
    if (ok && fNum > 0)
        return QString("f/%1").arg(fNum, 0, 'f', 1);
    return "f/" + formatted;
}

QString ExifParser::formatFocalLength(const QString &value) {
    if (value.isEmpty())
        return value;
    // libexif 可能已经返回 "85.0 mm" 这样的格式
    if (value.contains("mm")) {
        QString v = value.trimmed();
        if (!v.endsWith("mm"))
            v += "mm";
        return v;
    }
    QString formatted = formatRational(value);
    bool ok = false;
    double fl = formatted.toDouble(&ok);
    if (ok && fl > 0)
        return QString("%1mm").arg(fl, 0, 'f', 1);
    return formatted + "mm";
}

QString ExifParser::formatExposureBias(const QString &value) {
    if (value.isEmpty())
        return value;
    // libexif 可能已经返回 "0.00 EV" 这样的格式
    if (value.contains("EV"))
        return value.trimmed();
    QString formatted = formatRational(value);
    bool ok = false;
    double bias = formatted.toDouble(&ok);
    if (ok) {
        if (bias == 0)
            return "0 EV";
        else if (bias > 0)
            return QString("+%1 EV").arg(bias, 0, 'f', 1);
        else
            return QString("%1 EV").arg(bias, 0, 'f', 1);
    }
    return formatted + " EV";
}

QString ExifParser::lookupEnum(const QString &value, const QMap<int, QString> &mapping) {
    bool ok = false;
    int intValue = value.toInt(&ok);
    if (ok && mapping.contains(intValue))
        return mapping[intValue];
    return value;
}

QString ExifParser::cleanString(const QString &str) {
    QString cleaned = str.trimmed();
    cleaned.remove(QChar('\0'));
    return cleaned;
}

static QString exifEntryToString(ExifEntry *entry) {
    if (!entry)
        return QString();
    int size = entry->size;
    int max_size = VIPS_MIN(size, 10000) * 3 + 32;
    char *text = (char *)malloc(max_size + 1);
    exif_entry_get_value(entry, text, max_size);
    if (entry->format == EXIF_FORMAT_ASCII)
        text[size] = '\0';
    QString result = QString::fromUtf8(text);
    free(text);
    return result;
}

static QString friendlyFieldName(int ifd, const char *tagName) {
    static const QMap<QString, QString> fieldNameMap = {
        {"XResolution", "X Resolution"}, {"YResolution", "Y Resolution"}, {"ResolutionUnit", "Resolution Unit"}, {"BitsPerSample", "Bits Per Sample"}, {"Compression", "Compression"}, {"PhotometricInterpretation", "Photometric Interpretation"}, {"ImageDescription", "Image Description"}, {"ExifVersion", "Exif Version"}, {"ComponentsConfiguration", "Components Configuration"}, {"CompressedBitsPerPixel", "Compressed Bits Per Pixel"}, {"ShutterSpeedValue", "Shutter Speed Value"}, {"ApertureValue", "Aperture Value"}, {"BrightnessValue", "Brightness Value"}, {"MaxApertureValue", "Max Aperture Value"}, {"SubjectDistance", "Subject Distance"}, {"LightSource", "Light Source"}, {"SubjectArea", "Subject Area"}, {"UserComment", "User Comment"}, {"SubSecTime", "Sub Second Time"}, {"SubSecTimeOriginal", "Sub Second Time Original"}, {"SubSecTimeDigitized", "Sub Second Time Digitized"}, {"FlashpixVersion", "FlashPix Version"}, {"PixelXDimension", "Pixel X Dimension"}, {"PixelYDimension", "Pixel Y Dimension"}, {"GainControl", "Gain Control"}, {"Contrast", "Contrast"}, {"Saturation", "Saturation"}, {"Sharpness", "Sharpness"}, {"SubjectDistanceRange", "Subject Distance Range"}, {"DigitalZoomRatio", "Digital Zoom Ratio"}};
    QString key = QString::fromUtf8(tagName);
    if (fieldNameMap.contains(key))
        return fieldNameMap[key];
    return key;
}

ExifParser::ExifData ExifParser::parse(const QString &filePath) {
    ExifData result;
#ifdef HAS_LIBVIPS
    try {
        QString normalizedPath = filePath;
        normalizedPath.replace('\\', '/');
        QByteArray pathBytes = normalizedPath.toUtf8();
        const char *path = pathBytes.constData();

        VipsImage *vipsImage = vips_image_new_from_file(path, NULL);
        if (!vipsImage) {
            qWarning() << "ExifParser: vips_image_new_from_file failed:" << filePath;
            return result;
        }

        result.width = vipsImage->Xsize;
        result.height = vipsImage->Ysize;

        const void *exifBlob = nullptr;
        size_t exifLen = 0;
        if (vips_image_get_blob(vipsImage, "exif-data", &exifBlob, &exifLen) != 0 || !exifBlob || exifLen == 0) {
            qWarning() << "ExifParser: No exif-data found in" << filePath;
            g_object_unref(vipsImage);
            return result;
        }

        ::ExifData *libexifData = exif_data_new();
        if (!libexifData) {
            qWarning() << "ExifParser: Failed to create ExifData";
            g_object_unref(vipsImage);
            return result;
        }
        exif_data_load_data(libexifData, static_cast<const unsigned char *>(exifBlob), exifLen);

        for (int i = 0; i < EXIF_IFD_COUNT; i++) {
            ExifContent *content = libexifData->ifd[i];
            if (!content)
                continue;
            for (unsigned int j = 0; j < content->count; j++) {
                ExifEntry *entry = content->entries[j];
                if (!entry)
                    continue;
                const char *tagName = exif_tag_get_name_in_ifd(entry->tag, static_cast<ExifIfd>(i));
                if (!tagName)
                    continue;

                QString fieldName = QString("exif-ifd%1-%2").arg(i).arg(tagName);
                QString value = cleanString(exifEntryToString(entry));
                if (value.isEmpty())
                    continue;

                if (fieldName == "exif-ifd0-Make")
                    result.cameraMake = cleanString(value);
                else if (fieldName == "exif-ifd0-Model")
                    result.cameraModel = cleanString(value);
                else if (fieldName == "exif-ifd0-Software")
                    result.software = value;
                else if (fieldName == "exif-ifd0-Artist")
                    result.artist = value;
                else if (fieldName == "exif-ifd0-Copyright")
                    result.copyright = value;
                else if (fieldName == "exif-ifd0-Orientation") {
                    if (value == "Top-left" || value == "1")
                        result.orientation = 1;
                    else if (value == "Top-right" || value == "2")
                        result.orientation = 2;
                    else if (value == "Bottom-right" || value == "3")
                        result.orientation = 3;
                    else if (value == "Bottom-left" || value == "4")
                        result.orientation = 4;
                    else if (value == "Left-top" || value == "5")
                        result.orientation = 5;
                    else if (value == "Right-top" || value == "6")
                        result.orientation = 6;
                    else if (value == "Right-bottom" || value == "7")
                        result.orientation = 7;
                    else if (value == "Left-bottom" || value == "8")
                        result.orientation = 8;
                    else
                        result.orientation = value.toInt();
                } else if (fieldName == "exif-ifd0-XResolution")
                    result.xResolution = formatRational(value);
                else if (fieldName == "exif-ifd0-YResolution")
                    result.yResolution = formatRational(value);
                else if (fieldName == "exif-ifd0-ResolutionUnit")
                    result.resolutionUnit = value;
                else if (fieldName == "exif-ifd0-BitsPerSample")
                    result.bitsPerSample = value;
                else if (fieldName == "exif-ifd0-Compression")
                    result.compression = value;
                else if (fieldName == "exif-ifd0-PhotometricInterpretation")
                    result.photometricInterpretation = value;
                else if (fieldName == "exif-ifd0-ImageDescription")
                    result.imageDescription = value;
                else if (fieldName == "exif-ifd0-DateTime")
                    result.dateTime = value;
                else if (fieldName == "exif-ifd1-Compression") {
                } else if (fieldName == "exif-ifd2-ExposureTime")
                    result.exposureTime = formatExposureTime(value);
                else if (fieldName == "exif-ifd2-FNumber")
                    result.fNumber = formatFNumber(value);
                else if (fieldName == "exif-ifd2-ISO" || fieldName == "exif-ifd2-ISOSpeedRatings")
                    result.iso = value;
                else if (fieldName == "exif-ifd2-DateTimeOriginal")
                    result.dateTimeOriginal = value;
                else if (fieldName == "exif-ifd2-DateTimeDigitized")
                    result.dateTimeDigitized = value;
                else if (fieldName == "exif-ifd2-BrightnessValue")
                    result.rawFields[friendlyFieldName(i, tagName)] = formatRational(value);
                else if (fieldName == "exif-ifd2-ExposureBiasValue")
                    result.exposureBias = formatExposureBias(value);
                else if (fieldName == "exif-ifd2-MaxApertureValue")
                    result.rawFields[friendlyFieldName(i, tagName)] = formatRational(value);
                else if (fieldName == "exif-ifd2-SubjectDistance")
                    result.rawFields[friendlyFieldName(i, tagName)] = formatRational(value) + " m";
                else if (fieldName == "exif-ifd2-MeteringMode") {
                    static const QMap<int, QString> meteringMap = {
                        {0, "Unknown"}, {1, "Average"}, {2, "CenterWeightedAverage"}, {3, "Spot"}, {4, "MultiSpot"}, {5, "Pattern"}, {6, "Partial"}, {255, "Other"}};
                    result.meteringMode = lookupEnum(value, meteringMap);
                } else if (fieldName == "exif-ifd2-LightSource") {
                    static const QMap<int, QString> lightMap = {
                        {0, "Unknown"}, {1, "Daylight"}, {2, "Fluorescent"}, {3, "Tungsten"}, {4, "Flash"}, {9, "Fine weather"}, {10, "Cloudy weather"}, {11, "Shade"}, {12, "Daylight fluorescent"}, {13, "Day white fluorescent"}, {14, "Cool white fluorescent"}, {15, "White fluorescent"}, {17, "Standard light A"}, {18, "Standard light B"}, {19, "Standard light C"}, {20, "D55"}, {21, "D65"}, {22, "D75"}, {23, "D50"}, {24, "ISO studio tungsten"}, {255, "Other"}};
                    result.rawFields[friendlyFieldName(i, tagName)] = lookupEnum(value, lightMap);
                } else if (fieldName == "exif-ifd2-Flash") {
                    bool ok = false;
                    int flashValue = value.toInt(&ok);
                    if (ok) {
                        result.flash = (flashValue & 0x01) ? "Fired" : "No Flash";
                        if (flashValue & 0x08)
                            result.flash += ", Auto";
                        if (flashValue & 0x40)
                            result.flash += ", Red-eye reduction";
                    } else
                        result.flash = value;
                } else if (fieldName == "exif-ifd2-FocalLength")
                    result.focalLength = formatFocalLength(value);
                else if (fieldName == "exif-ifd2-UserComment")
                    result.userComment = value;
                else if (fieldName == "exif-ifd2-SubSecTime" || fieldName == "exif-ifd2-SubSecTimeOriginal" || fieldName == "exif-ifd2-SubSecTimeDigitized")
                    result.rawFields[friendlyFieldName(i, tagName)] = value;
                else if (fieldName == "exif-ifd2-ColorSpace")
                    result.colorSpace = value;
                else if (fieldName == "exif-ifd2-FocalLengthIn35mmFilm") {
                    result.focalLength35mm = value.contains("mm") ? value.trimmed() : (value + "mm");
                } else if (fieldName == "exif-ifd2-SceneCaptureType") {
                    static const QMap<int, QString> sceneMap = {{0, "Standard"}, {1, "Landscape"}, {2, "Portrait"}, {3, "Night scene"}};
                    result.sceneCaptureType = lookupEnum(value, sceneMap);
                } else if (fieldName == "exif-ifd2-GainControl" || fieldName == "exif-ifd2-Contrast" || fieldName == "exif-ifd2-Saturation" || fieldName == "exif-ifd2-Sharpness" || fieldName == "exif-ifd2-SubjectDistanceRange")
                    result.rawFields[friendlyFieldName(i, tagName)] = value;
                else if (fieldName == "exif-ifd2-ExposureProgram") {
                    static const QMap<int, QString> programMap = {{0, "Not defined"}, {1, "Manual"}, {2, "Normal program"}, {3, "Aperture priority"}, {4, "Shutter priority"}, {5, "Creative program"}, {6, "Action program"}, {7, "Portrait mode"}, {8, "Landscape mode"}};
                    result.exposureProgram = lookupEnum(value, programMap);
                } else if (fieldName == "exif-ifd2-ExposureMode") {
                    static const QMap<int, QString> modeMap = {{0, "Auto"}, {1, "Manual"}, {2, "Auto bracket"}};
                    result.exposureMode = lookupEnum(value, modeMap);
                } else if (fieldName == "exif-ifd2-WhiteBalance") {
                    static const QMap<int, QString> wbMap = {{0, "Auto"}, {1, "Manual"}};
                    result.whiteBalance = lookupEnum(value, wbMap);
                } else if (fieldName == "exif-ifd2-DigitalZoomRatio")
                    result.rawFields[friendlyFieldName(i, tagName)] = formatRational(value) + "x";
                else if (fieldName == "exif-ifd2-LensMake")
                    result.lensMake = cleanString(value);
                else if (fieldName == "exif-ifd2-LensModel")
                    result.lensModel = cleanString(value);
                else if (fieldName == "exif-ifd2-LensSpecification")
                    result.rawFields[friendlyFieldName(i, tagName)] = value;
                else if (fieldName == "exif-ifd3-LensMake" && result.lensMake.isEmpty())
                    result.lensMake = cleanString(value);
                else if (fieldName == "exif-ifd3-LensModel" && result.lensModel.isEmpty())
                    result.lensModel = cleanString(value);
                else if (fieldName == "exif-ifd2-GPSLatitude" || fieldName == "exif-ifd3-GPSLatitude")
                    result.gpsLatitude = value;
                else if (fieldName == "exif-ifd2-GPSLatitudeRef" || fieldName == "exif-ifd3-GPSLatitudeRef")
                    result.gpsLatitudeRef = value;
                else if (fieldName == "exif-ifd2-GPSLongitude" || fieldName == "exif-ifd3-GPSLongitude")
                    result.gpsLongitude = value;
                else if (fieldName == "exif-ifd2-GPSLongitudeRef" || fieldName == "exif-ifd3-GPSLongitudeRef")
                    result.gpsLongitudeRef = value;
                else if (fieldName == "exif-ifd2-GPSAltitude" || fieldName == "exif-ifd3-GPSAltitude")
                    result.gpsAltitude = formatRational(value);
                else if (fieldName == "exif-ifd2-GPSAltitudeRef" || fieldName == "exif-ifd3-GPSAltitudeRef")
                    result.gpsAltitudeRef = (value == "0" || value == "Sea level") ? " m" : " m (below sea level)";
                else if (fieldName == "exif-ifd2-GPSTimeStamp" || fieldName == "exif-ifd3-GPSTimeStamp")
                    result.gpsTimeStamp = value;
                else if (fieldName == "exif-ifd2-GPSDateStamp" || fieldName == "exif-ifd3-GPSDateStamp")
                    result.gpsDateStamp = value;
                else if (fieldName.startsWith("exif-") || fieldName.startsWith("xmp-"))
                    result.rawFields[friendlyFieldName(i, tagName)] = value;
            }
        }

        exif_data_free(libexifData);
        g_object_unref(vipsImage);
    } catch (...) {
        qWarning() << "ExifParser: Failed to parse EXIF data for" << filePath;
    }
#else
    Q_UNUSED(filePath);
#endif
    return result;
}
