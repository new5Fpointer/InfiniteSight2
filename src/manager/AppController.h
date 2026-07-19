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

    // 用户操作入口（由 MainWindow 直接调用）
    void openImage(const QString &path);
    void navigateNext();
    void navigatePrevious();
    void jumpToImage(int index);
    void zoomIn();
    void zoomOut();
    void actualSize();
    void fitToWindow();
    void toggleFitActualSize();
    void rotateImage(int angle);
    void mirrorImage();
    void deleteImage();
    void setInfoPanelVisible(bool visible);
    void switchTheme(const QString &theme);
    void openSettingsDialog();
    void closeWindow();
    void setActualScaleFactor(double scale);

    void applySettings();

private slots:
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
