#pragma once

#include "common/ImageModel.h"
#include <QObject>
#include <QSettings>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);

    GeneralSettings general() const;
    PerformanceSettings performance() const;
    AppearanceSettings appearance() const;

    void setGeneral(const GeneralSettings &settings);
    void setPerformance(const PerformanceSettings &settings);
    void setAppearance(const AppearanceSettings &settings);

    void addRecentFile(const QString &filePath);
    void clearRecentFiles();

    void save();
    void load();

signals:
    void settingsChanged();

private:
    QSettings m_settings;
    GeneralSettings m_general;
    PerformanceSettings m_performance;
    AppearanceSettings m_appearance;
};
