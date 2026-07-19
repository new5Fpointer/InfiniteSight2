#include "AppController.h"
#include "ImageNavigator.h"
#include "LoadCoordinator.h"
#include "ViewStateManager.h"
#include "core/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/SettingsWindow.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow) {
    m_mainWindow->m_controller = this;
    m_settingsManager = new SettingsManager(this);
    m_navigator = new ImageNavigator(this);
    m_loadCoordinator = new LoadCoordinator(this);
    m_viewStateManager = new ViewStateManager(this);

    // 内部模块（跨线程信号仍需 connect）
    connect(m_navigator, &ImageNavigator::currentPathChanged,
            this, &AppController::onCurrentPathChanged);
    connect(m_loadCoordinator, &LoadCoordinator::imageLoaded,
            this, &AppController::onImageLoaded);
    connect(m_loadCoordinator, &LoadCoordinator::infoReady,
            this, &AppController::onInfoReady);
    connect(m_loadCoordinator, &LoadCoordinator::progress,
            this, &AppController::onProgress);
    connect(m_viewStateManager, &ViewStateManager::viewStateChanged,
            this, &AppController::onViewStateChanged);

    applySettings();
}

AppController::~AppController() {
}

void AppController::applySettings() {
    m_mainWindow->onSettingsApplied(m_settingsManager->general(),
                                    m_settingsManager->performance(),
                                    m_settingsManager->appearance());
}

void AppController::openImage(const QString &path) {
    if (!path.isEmpty()) {
        m_navigator->goTo(path);
    }
}

void AppController::navigateNext() {
    m_navigator->next();
}

void AppController::navigatePrevious() {
    m_navigator->previous();
}

void AppController::jumpToImage(int index) {
    m_navigator->jumpTo(index);
}

void AppController::zoomIn() {
    m_viewStateManager->zoomIn();
}

void AppController::zoomOut() {
    m_viewStateManager->zoomOut();
}

void AppController::actualSize() {
    m_viewStateManager->actualSize();
}

void AppController::fitToWindow() {
    m_viewStateManager->fitToWindow();
}

void AppController::toggleFitActualSize() {
    m_viewStateManager->toggleFitActualSize();
}

void AppController::rotateImage(int angle) {
    m_viewStateManager->rotate(angle);
}

void AppController::mirrorImage() {
    m_viewStateManager->mirror();
}

void AppController::deleteImage() {
    QString path = m_navigator->currentPath();
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.moveToTrash()) {
        qWarning() << "Failed to move image to trash:" << file.errorString();
        return;
    }
    qInfo() << "Image moved to trash:" << QFileInfo(path).fileName();

    // 刷新当前文件夹列表并加载下一张
    m_navigator->refreshAfterDeletion();
}

void AppController::setInfoPanelVisible(bool visible) {
    GeneralSettings g = m_settingsManager->general();
    g.showInfoPanel = visible;
    m_settingsManager->setGeneral(g);
    m_settingsManager->save();
}

void AppController::switchTheme(const QString &theme) {
    AppearanceSettings a = m_settingsManager->appearance();
    a.theme = theme;
    m_settingsManager->setAppearance(a);
    m_settingsManager->save();
    applySettings();
}

void AppController::openSettingsDialog() {
    SettingsWindow dialog(m_settingsManager, m_mainWindow);
    connect(&dialog, &SettingsWindow::settingsApplied, this, &AppController::applySettings);
    dialog.exec();
}

void AppController::closeWindow() {
    GeneralSettings g = m_settingsManager->general();
    g.windowGeometry = m_mainWindow->saveGeometry();
    m_settingsManager->setGeneral(g);
    m_settingsManager->save();
}

void AppController::setActualScaleFactor(double scale) {
    m_viewStateManager->setScaleFactor(scale);
}

void AppController::onCurrentPathChanged(const QString &path, const QStringList &folderImages, int currentIndex) {
    Q_UNUSED(path)
    m_currentFolderImages = folderImages;
    m_currentIndex = currentIndex;
    m_viewStateManager->resetSilent();
    loadCurrentImage();
}

void AppController::loadCurrentImage() {
    QString path = m_navigator->currentPath();
    if (path.isEmpty())
        return;

    PerformanceSettings perf = m_settingsManager->performance();
    m_loadCoordinator->load(path, perf);
}

void AppController::onImageLoaded(const ImageViewModel &viewModel) {
    ImageViewModel vm = viewModel;
    vm.viewState = m_viewStateManager->viewState();
    vm.currentIndex = m_navigator->currentIndex();
    vm.total = m_navigator->total();
    m_mainWindow->onImageLoaded(vm);
}

void AppController::onInfoReady(const ImageInfo &info) {
    m_mainWindow->onInfoReady(info);
}

void AppController::onProgress(int value) {
    m_mainWindow->onProgress(value);
}

void AppController::onViewStateChanged(const ViewState &state) {
    m_mainWindow->onViewStateChanged(state);
}
