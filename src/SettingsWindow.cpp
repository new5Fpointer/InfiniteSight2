#include "SettingsWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

SettingsWindow::SettingsWindow(SettingsManager *manager, QWidget *parent)
    : QDialog(parent), m_manager(manager) {
    setWindowTitle(tr("Application Settings"));
    setMinimumSize(400, 150);

    setupUi();
    loadCurrentSettings();
}

void SettingsWindow::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

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
}

void SettingsWindow::applyFromUi() {
    AppearanceSettings a;
    a.theme = m_themeCombo->currentIndex() == 0 ? "dark" : "light";
    m_manager->setAppearance(a);
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
    m_manager->setAppearance(AppearanceSettings());
    m_manager->save();
    loadCurrentSettings();
    emit settingsApplied();
}
