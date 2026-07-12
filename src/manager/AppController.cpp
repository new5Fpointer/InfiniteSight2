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
    m_settingsManager = new SettingsManager(this);
    m_navigator = new ImageNavigator(this);
    m_loadCoordinator = new LoadCoordinator(this);
    m_viewStateManager = new ViewStateManager(this);

    // MainWindow -> AppController
    connect(mainWindow, &MainWindow::imageOpenRequested, this, &AppController::onImageOpenRequested);
    connect(mainWindow, &MainWindow::navigateNextRequested, this, &AppController::onNavigateNextRequested);
    connect(mainWindow, &MainWindow::navigatePreviousRequested, this, &AppController::onNavigatePreviousRequested);
    connect(mainWindow, &MainWindow::jumpToImageRequested, this, &AppController::onJumpToImageRequested);
    connect(mainWindow, &MainWindow::zoomInRequested, this, &AppController::onZoomInRequested);
    connect(mainWindow, &MainWindow::zoomOutRequested, this, &AppController::onZoomOutRequested);
    connect(mainWindow, &MainWindow::actualSizeRequested, this, &AppController::onActualSizeRequested);
    connect(mainWindow, &MainWindow::fitToWindowRequested, this, &AppController::onFitToWindowRequested);
    connect(mainWindow, &MainWindow::toggleFitActualSizeRequested, this, &AppController::onToggleFitActualSizeRequested);
    connect(mainWindow, &MainWindow::rotateRequested, this, &AppController::onRotateRequested);
    connect(mainWindow, &MainWindow::mirrorRequested, this, &AppController::onMirrorRequested);
    connect(mainWindow, &MainWindow::deleteImageRequested, this, &AppController::onDeleteImageRequested);
    connect(mainWindow, &MainWindow::infoPanelVisibilityChanged, this, &AppController::onInfoPanelVisibilityChanged);
    connect(mainWindow, &MainWindow::themeChangeRequested, this, &AppController::onThemeChangeRequested);
    connect(mainWindow, &MainWindow::openSettingsRequested, this, &AppController::onOpenSettingsRequested);
    connect(mainWindow, &MainWindow::windowCloseRequested, this, &AppController::onWindowCloseRequested);

    // 内部模块
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

    // AppController -> MainWindow
    connect(this, &AppController::settingsApplied,
            mainWindow, &MainWindow::onSettingsApplied);

    // 同步 fitInView 后的实际缩放值到 ViewStateManager
    connect(mainWindow, &MainWindow::actualScaleFactorChanged,
            this, [this](double scale) {
                m_viewStateManager->setScaleFactor(scale);
            });

    applySettings();
}

AppController::~AppController() {
}

void AppController::applySettings() {
    emit settingsApplied(m_settingsManager->general(),
                         m_settingsManager->performance(),
                         m_settingsManager->appearance());
}

void AppController::onImageOpenRequested(const QString &path) {
    if (!path.isEmpty()) {
        m_navigator->goTo(path);
    }
}

void AppController::onNavigateNextRequested() {
    m_navigator->next();
}

void AppController::onNavigatePreviousRequested() {
    m_navigator->previous();
}

void AppController::onJumpToImageRequested(int index) {
    m_navigator->jumpTo(index);
}

void AppController::onZoomInRequested() {
    m_viewStateManager->zoomIn();
}

void AppController::onZoomOutRequested() {
    m_viewStateManager->zoomOut();
}

void AppController::onActualSizeRequested() {
    m_viewStateManager->actualSize();
}

void AppController::onFitToWindowRequested() {
    m_viewStateManager->fitToWindow();
}

void AppController::onToggleFitActualSizeRequested() {
    m_viewStateManager->toggleFitActualSize();
}

void AppController::onRotateRequested(int angle) {
    m_viewStateManager->rotate(angle);
}

void AppController::onMirrorRequested() {
    m_viewStateManager->mirror();
}

void AppController::onDeleteImageRequested() {
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

void AppController::onInfoPanelVisibilityChanged(bool visible) {
    GeneralSettings g = m_settingsManager->general();
    g.showInfoPanel = visible;
    m_settingsManager->setGeneral(g);
    m_settingsManager->save();
}

void AppController::onThemeChangeRequested(const QString &theme) {
    AppearanceSettings a = m_settingsManager->appearance();
    a.theme = theme;
    m_settingsManager->setAppearance(a);
    m_settingsManager->save();
    applySettings();
}

void AppController::onOpenSettingsRequested() {
    SettingsWindow dialog(m_settingsManager, m_mainWindow);
    connect(&dialog, &SettingsWindow::settingsApplied, this, &AppController::applySettings);
    dialog.exec();
}

void AppController::onWindowCloseRequested() {
    GeneralSettings g = m_settingsManager->general();
    g.windowGeometry = m_mainWindow->saveGeometry();
    m_settingsManager->setGeneral(g);
    m_settingsManager->save();
}

void AppController::onCurrentPathChanged(const QString &path, const QStringList &folderImages, int currentIndex) {
    Q_UNUSED(path)
    m_currentFolderImages = folderImages;
    m_currentIndex = currentIndex;
    m_viewStateManager->reset();
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
