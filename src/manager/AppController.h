#pragma once

#include "common/ImageModel.h"
#include <QObject>

class MainWindow;
class SettingsManager;
class ImageNavigator;
class LoadCoordinator;
class ViewStateManager;

class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);
    ~AppController();

    void applySettings();

signals:
    void settingsApplied(const GeneralSettings &g, const PerformanceSettings &p, const AppearanceSettings &a);

private slots:
    // 来自 MainWindow 的用户意图
    void onImageOpenRequested(const QString &path);
    void onNavigateNextRequested();
    void onNavigatePreviousRequested();
    void onJumpToImageRequested(int index);
    void onZoomInRequested();
    void onZoomOutRequested();
    void onActualSizeRequested();
    void onFitToWindowRequested();
    void onToggleFitActualSizeRequested();
    void onRotateRequested(int angle);
    void onMirrorRequested();
    void onDeleteImageRequested();
    void onInfoPanelVisibilityChanged(bool visible);
    void onThemeChangeRequested(const QString &theme);
    void onOpenSettingsRequested();
    void onWindowCloseRequested();

    // 来自内部模块
    void onCurrentPathChanged(const QString &path, const QStringList &folderImages, int currentIndex);
    void onImageLoaded(const ImageViewModel &viewModel);
    void onInfoReady(const ImageInfo &info);
    void onProgress(int value);
    void onViewStateChanged(const ViewState &state);

private:
    void loadCurrentImage();

    MainWindow *m_mainWindow;
    SettingsManager *m_settingsManager;
    ImageNavigator *m_navigator;
    LoadCoordinator *m_loadCoordinator;
    ViewStateManager *m_viewStateManager;

    QStringList m_currentFolderImages;
    int m_currentIndex = -1;
};
