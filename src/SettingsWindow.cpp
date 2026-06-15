#include "SettingsWindow.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(SettingsManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager) {
    setWindowTitle(tr("Application Settings"));
    setMinimumSize(400, 150);

    setupUi();
    loadCurrentSettings();
}

void SettingsWindow::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 通用设置组
    QGroupBox *generalGroup = new QGroupBox(tr("General"), this);
    QFormLayout *generalLayout = new QFormLayout(generalGroup);

    m_showInfoPanelCheck = new QCheckBox(generalGroup);
    m_showInfoPanelCheck->setText(tr("Show info panel"));
    generalLayout->addRow("", m_showInfoPanelCheck);

    layout->addWidget(generalGroup);

    // 性能设置组
    QGroupBox *performanceGroup = new QGroupBox(tr("Performance"), this);
    QFormLayout *perfLayout = new QFormLayout(performanceGroup);

    m_skipExifCheck = new QCheckBox(performanceGroup);
    m_skipExifCheck->setText(tr("Skip EXIF parsing"));
    perfLayout->addRow("", m_skipExifCheck);

    layout->addWidget(performanceGroup);

    // 外观设置组
    QGroupBox *appearanceGroup = new QGroupBox(tr("Appearance"), this);
    QFormLayout *formLayout = new QFormLayout(appearanceGroup);

    m_themeCombo = new QComboBox(appearanceGroup);
    m_themeCombo->addItems({tr("Dark"), tr("Light")});
    formLayout->addRow(tr("Theme:"), m_themeCombo);

    layout->addWidget(appearanceGroup);
    layout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
            QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
        this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsWindow::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsWindow::onApply);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsWindow::onRestoreDefaults);

    layout->addWidget(buttonBox);
}

void SettingsWindow::loadCurrentSettings() {
    AppearanceSettings a = m_manager->appearance();
    m_themeCombo->setCurrentIndex(a.theme == "dark" ? 0 : 1);

    GeneralSettings g = m_manager->general();
    m_showInfoPanelCheck->setChecked(g.showInfoPanel);

    PerformanceSettings p = m_manager->performance();
    m_skipExifCheck->setChecked(p.skipExif);
}

void SettingsWindow::applyFromUi() {
    AppearanceSettings a;
    a.theme = m_themeCombo->currentIndex() == 0 ? "dark" : "light";
    m_manager->setAppearance(a);

    GeneralSettings g = m_manager->general();
    g.showInfoPanel = m_showInfoPanelCheck->isChecked();
    m_manager->setGeneral(g);

    PerformanceSettings p = m_manager->performance();
    p.skipExif = m_skipExifCheck->isChecked();
    m_manager->setPerformance(p);

    m_manager->save();
}

void SettingsWindow::onAccepted() {
    applyFromUi();
    accept();
    emit settingsApplied();
}

void SettingsWindow::onApply() {
    applyFromUi();
    emit settingsApplied();
}

void SettingsWindow::onRestoreDefaults() {
    m_manager->setGeneral(GeneralSettings());
    m_manager->setPerformance(PerformanceSettings());
    m_manager->setAppearance(AppearanceSettings());
    m_manager->save();
    loadCurrentSettings();
    emit settingsApplied();
}
