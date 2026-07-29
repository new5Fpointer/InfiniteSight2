#pragma once

#include "core/SettingsManager.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(SettingsManager *manager, QWidget *parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onAccepted();
    void onApply();
    void onRestoreDefaults();

private:
    void setupUi();
    void loadCurrentSettings();
    void applyFromUi();

    SettingsManager *m_manager;
    QComboBox *m_themeCombo;
    QCheckBox *m_showInfoPanelCheck;
    QCheckBox *m_skipExifCheck;
    QCheckBox *m_showProgressCheck;
};
