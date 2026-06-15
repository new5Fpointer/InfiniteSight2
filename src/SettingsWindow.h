#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include "SettingsManager.h"

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
};
