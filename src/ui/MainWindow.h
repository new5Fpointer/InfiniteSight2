#pragma once

#include "common/ImageModel.h"
#include "ui/ZoomableGraphicsView.h"
#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QTreeWidget>

class AppController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onImageLoaded(const ImageViewModel &viewModel);
    void onInfoReady(const ImageInfo &info);
    void onProgress(int value);
    void onLoadFailed(const QString &filePath);
    void onViewStateChanged(const ViewState &state);
    void onSettingsApplied(const GeneralSettings &g, const PerformanceSettings &p, const AppearanceSettings &a);

private slots:
    void openImage();
    void toggleInfoPanel(bool visible);

    void onMinimize();
    void onMaximize();
    void onClose();
    void updateMaximizeIcon();
    void showMenu();
    void toggleFullscreen();
    void hideBottomBarAnimated();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    enum class ResizeEdge { None,
                            Left,
                            Right,
                            Top,
                            Bottom,
                            TopLeft,
                            TopRight,
                            BottomLeft,
                            BottomRight };
    static constexpr int kResizeMargin = 8;

    void setupUi();
    void createMenus();
    void createTitleBar();
    void createBottomBar();
    void resetCanvas();
    void applyStyleSheet();
    void applyViewState();
    QIcon themedIcon(const QString &name);
    void refreshIcons();
    void updateTitleBarTitle();
    void updateBottomBarInfo();
    void updateCenterContainerPos();
    void updateLoadFailedPos();
    void onThemeChanged();
    ResizeEdge getResizeEdge(const QPoint &pos) const;
    void updateCursorForResize(ResizeEdge edge);
    void clearResizeCursor();

    QWidget *m_titleBar;
    QLabel *m_titleIcon;
    QLabel *m_titleLabel;
    QPushButton *m_menuBtn;
    QPushButton *m_pinBtn;
    QPushButton *m_minBtn;
    QPushButton *m_maxBtn;
    QPushButton *m_closeBtn;
    QPoint m_dragPos;
    bool m_dragging;
    bool m_dragFromMaximized = false;
    bool m_resizing = false;
    ResizeEdge m_resizeEdge = ResizeEdge::None;
    QTimer *m_bottomBarTimer;

    QWidget *m_bottomBar;
    QWidget *m_infoContainer;
    QWidget *m_centerContainer;
    QLabel *m_fileInfoLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_fileDimensionLabel;
    QLabel *m_fileFormatLabel;
    QPushButton *m_prevBtn;
    QLabel *m_pageLabel;
    QLineEdit *m_pageEdit;
    QPushButton *m_nextBtn;
    QPushButton *m_fitBtn;
    QPushButton *m_zoomCombo;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_zoomInBtn;
    QPushButton *m_rotateLeftBtn;
    QPushButton *m_rotateRightBtn;
    QPushButton *m_copyBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_fullscreenBtn;
    bool m_bottomBarInLayout = false;

    QSplitter *m_splitter;
    ZoomableGraphicsView *m_graphicsView;
    QGraphicsScene *m_graphicsScene;
    QGraphicsPixmapItem *m_pixmapItem;

    QDockWidget *m_infoDock;
    QTreeWidget *m_infoTree;
    QProgressBar *m_progressBar;
    QLabel *m_loadingLabel;
    QLabel *m_loadFailedLabel;
    QLabel *m_roamLabel;

    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    QMenu *m_settingsMenu;
    QAction *m_openAction;
    QAction *m_exitAction;
    QAction *m_infoToggle;
    QAction *m_settingsAction;
    QAction *m_darkAction;
    QAction *m_lightAction;

    ImageViewModel m_currentViewModel;
    ViewState m_currentViewState;

    GeneralSettings m_generalSettings;
    PerformanceSettings m_performanceSettings;
    AppearanceSettings m_appearanceSettings;

    bool m_isFileDialogOpen = false;

    friend class AppController;
    AppController *m_controller = nullptr;
};
