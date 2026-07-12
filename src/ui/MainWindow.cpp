#include "MainWindow.h"
#include "SettingsWindow.h"
#include "ThemeManager.h"
#include "common/AppConstants.h"
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMovie>
#include <QOpenGLWidget>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QStyle>
#include <QTransform>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_graphicsView(new ZoomableGraphicsView(this)),
      m_graphicsScene(new QGraphicsScene(this)),
      m_pixmapItem(nullptr),
      m_progressBar(new QProgressBar(this)),
      m_loadingLabel(new QLabel(this)),
      m_roamLabel(new QLabel(this)),
      m_fileInfoLabel(nullptr),
      m_fileSizeLabel(nullptr),
      m_fileDimensionLabel(nullptr),
      m_fileFormatLabel(nullptr),
      m_dragging(false),
      m_isFileDialogOpen(false) {
    setWindowTitle("InfiniteSight");
    resize(1200, 800);
    setMinimumSize(800, 600);
    setAcceptDrops(true);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupUi();
    createMenus();
    createTitleBar();
    createBottomBar();

    // 安装全局事件过滤器以支持无边框窗口边缘调整大小
    setMouseTracking(true);
    if (centralWidget())
        centralWidget()->setMouseTracking(true);
    if (m_titleBar)
        m_titleBar->setMouseTracking(true);
    if (m_bottomBar)
        m_bottomBar->setMouseTracking(true);

    m_bottomBarTimer = new QTimer(this);
    m_bottomBarTimer->setSingleShot(true);
    m_bottomBarTimer->setInterval(1500);
    connect(m_bottomBarTimer, &QTimer::timeout, this, &MainWindow::hideBottomBarAnimated);
    m_graphicsView->setMouseTracking(true);
    m_graphicsView->viewport()->setMouseTracking(true);
    m_graphicsView->setFocusPolicy(Qt::StrongFocus);
    m_graphicsView->viewport()->setFocusPolicy(Qt::StrongFocus);
    m_graphicsView->installEventFilter(this);
    m_graphicsView->viewport()->installEventFilter(this);
    m_titleBar->installEventFilter(this);
    m_bottomBar->installEventFilter(this);
    centralWidget()->installEventFilter(this);

    connect(m_graphicsView, &ZoomableGraphicsView::filesDropped,
            this, [this](const QStringList &paths) {
                if (!paths.isEmpty())
                    emit imageOpenRequested(paths.first());
            });

    connect(m_graphicsView, &ZoomableGraphicsView::zoomLevelChanged,
            this, [this](double scaleFactor) {
                m_currentViewState.scaleFactor = scaleFactor;
                m_currentViewState.isFitToWindow = false;
                updateBottomBarInfo();
            });

    updateMaximizeIcon();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(36);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(10, 0, 0, 0);
    titleLayout->setSpacing(0);

    m_titleIcon = new QLabel(this);
    m_titleIcon->setFixedSize(20, 20);
    titleLayout->addWidget(m_titleIcon);

    m_titleLabel = new QLabel(tr("InfiniteSight"), this);
    m_titleLabel->setObjectName("titleLabel");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    auto createTitleBtn = [this](const QString &iconName, const QString &objName = "titleBtn") -> QPushButton * {
        QPushButton *btn = new QPushButton(this);
        btn->setFixedSize(46, 36);
        btn->setIconSize(QSize(14, 14));
        btn->setObjectName(objName);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    m_menuBtn = createTitleBtn("menu");
    connect(m_menuBtn, &QPushButton::clicked, this, &MainWindow::showMenu);
    titleLayout->addWidget(m_menuBtn);

    m_pinBtn = createTitleBtn("pin", "pinBtn");
    m_pinBtn->setCheckable(true);
    connect(m_pinBtn, &QPushButton::clicked, this, [this]() {
        Qt::WindowFlags flags = windowFlags();
        bool pinned = flags & Qt::WindowStaysOnTopHint;
        setWindowFlags(pinned ? (flags & ~Qt::WindowStaysOnTopHint) : (flags | Qt::WindowStaysOnTopHint));
        refreshIcons();
        show();
    });
    titleLayout->addWidget(m_pinBtn);

    m_minBtn = createTitleBtn("minimize");
    connect(m_minBtn, &QPushButton::clicked, this, &MainWindow::onMinimize);
    titleLayout->addWidget(m_minBtn);

    m_maxBtn = createTitleBtn("maximize");
    connect(m_maxBtn, &QPushButton::clicked, this, &MainWindow::onMaximize);
    titleLayout->addWidget(m_maxBtn);

    m_closeBtn = createTitleBtn("close", "closeBtn");
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::onClose);
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(m_titleBar);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_graphicsView->setScene(m_graphicsScene);
    m_graphicsView->setAlignment(Qt::AlignCenter);
    m_graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

    QWidget *imageContainer = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setAlignment(Qt::AlignCenter);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->addWidget(m_graphicsView);

    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setVisible(false);
    imageLayout->addWidget(m_loadingLabel);

    m_progressBar->setVisible(false);
    m_progressBar->setRange(0, 100);
    imageLayout->addWidget(m_progressBar);

    m_infoDock = new QDockWidget(tr("Image Information"), this);
    m_infoDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_infoTree = new QTreeWidget(this);
    m_infoTree->setHeaderHidden(true);
    m_infoTree->setColumnWidth(0, 200);
    m_infoTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_infoTree->setFont(QFont("Segoe UI", 10));

    m_infoDock->setWidget(m_infoTree);
    addDockWidget(Qt::RightDockWidgetArea, m_infoDock);
    m_infoDock->setVisible(false);

    m_splitter->addWidget(imageContainer);
    m_splitter->addWidget(m_infoDock);
    m_splitter->setSizes({1000, 200});
    m_infoDock->setMinimumWidth(150);

    mainLayout->addWidget(m_splitter);
    setCentralWidget(mainWidget);

    qInfo() << "MainWindow initialized";
}

void MainWindow::createTitleBar() {
}

void MainWindow::createBottomBar() {
    m_bottomBar = new QWidget(this);
    m_bottomBar->setObjectName("bottomBar");
    m_bottomBar->setFixedHeight(40);
    m_bottomBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    QHBoxLayout *bottomLayout = new QHBoxLayout(m_bottomBar);
    bottomLayout->setContentsMargins(12, 0, 12, 0);
    bottomLayout->setSpacing(0);

    auto createInfoBlock = [this](const QString &objName) -> QLabel * {
        QLabel *label = new QLabel(this);
        label->setObjectName(objName);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(24);
        return label;
    };

    m_infoContainer = new QWidget(this);
    m_infoContainer->setObjectName("infoContainer");
    QHBoxLayout *infoLayout = new QHBoxLayout(m_infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(6);

    m_fileSizeLabel = createInfoBlock("infoBlock");
    m_fileSizeLabel->setFixedWidth(60);
    infoLayout->addWidget(m_fileSizeLabel);

    m_fileDimensionLabel = createInfoBlock("infoBlock");
    m_fileDimensionLabel->setFixedWidth(80);
    infoLayout->addWidget(m_fileDimensionLabel);

    m_fileFormatLabel = createInfoBlock("infoBlock");
    m_fileFormatLabel->setFixedWidth(50);
    infoLayout->addWidget(m_fileFormatLabel);

    bottomLayout->addWidget(m_infoContainer);

    bottomLayout->addStretch();

    auto createBottomBtn = [this](const QString &iconName, int w = 32) -> QPushButton * {
        QPushButton *btn = new QPushButton(this);
        btn->setFixedSize(w, 28);
        if (!iconName.isEmpty()) {
            btn->setIcon(themedIcon(iconName));
            btn->setIconSize(QSize(16, 16));
        }
        btn->setObjectName("bottomBtn");
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    m_centerContainer = new QWidget(m_bottomBar);
    m_centerContainer->setObjectName("centerContainer");
    QHBoxLayout *centerLayout = new QHBoxLayout(m_centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(6);

    m_prevBtn = createBottomBtn("chevron-left");
    connect(m_prevBtn, &QPushButton::clicked, this, [this]() { navigateFolderImage(-1); });
    centerLayout->addWidget(m_prevBtn);

    QWidget *pageContainer = new QWidget(m_centerContainer);
    pageContainer->setObjectName("pageContainer");
    pageContainer->setFixedWidth(60);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageContainer);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    m_pageLabel = new QLabel("0 / 0", pageContainer);
    m_pageLabel->setObjectName("pageLabel");
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setFixedHeight(24);
    m_pageLabel->setCursor(Qt::PointingHandCursor);
    m_pageLabel->installEventFilter(this);
    pageLayout->addWidget(m_pageLabel);

    m_pageEdit = new QLineEdit(pageContainer);
    m_pageEdit->setObjectName("pageEdit");
    m_pageEdit->setFixedHeight(24);
    m_pageEdit->setAlignment(Qt::AlignCenter);
    m_pageEdit->setVisible(false);
    connect(m_pageEdit, &QLineEdit::editingFinished, this, [this]() {
        QString text = m_pageEdit->text().trimmed();
        m_pageEdit->setVisible(false);
        m_pageLabel->setVisible(true);
        if (m_bottomBarTimer)
            m_bottomBarTimer->start();
        if (text.isEmpty())
            return;
        bool ok;
        int page = text.toInt(&ok);
        if (ok && page > 0) {
            jumpToImage(page - 1);
        }
    });
    connect(m_pageEdit, &QLineEdit::returnPressed, this, [this]() {
        m_pageEdit->clearFocus();
    });
    m_pageEdit->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    pageLayout->addWidget(m_pageEdit);

    centerLayout->addWidget(pageContainer);

    m_nextBtn = createBottomBtn("chevron-right");
    connect(m_nextBtn, &QPushButton::clicked, this, [this]() { navigateFolderImage(1); });
    centerLayout->addWidget(m_nextBtn);

    centerLayout->addSpacing(16);

    m_fitBtn = createBottomBtn("fit-screen");
    m_fitBtn->setToolTip(tr("Fit to Window"));
    connect(m_fitBtn, &QPushButton::clicked, this, &MainWindow::toggleFitActualSize);
    centerLayout->addWidget(m_fitBtn);

    m_zoomCombo = createBottomBtn("", 56);
    m_zoomCombo->setText("100%");
    connect(m_zoomCombo, &QPushButton::clicked, this, &MainWindow::actualSize);
    centerLayout->addWidget(m_zoomCombo);

    m_zoomOutBtn = createBottomBtn("zoom-out");
    m_zoomOutBtn->setToolTip(tr("Zoom Out"));
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &MainWindow::zoomOut);
    centerLayout->addWidget(m_zoomOutBtn);

    m_zoomInBtn = createBottomBtn("zoom-in");
    m_zoomInBtn->setToolTip(tr("Zoom In"));
    connect(m_zoomInBtn, &QPushButton::clicked, this, &MainWindow::zoomIn);
    centerLayout->addWidget(m_zoomInBtn);

    m_rotateLeftBtn = createBottomBtn("rotate-left");
    m_rotateLeftBtn->setToolTip(tr("Rotate left") + " (Ctrl+L)");
    connect(m_rotateLeftBtn, &QPushButton::clicked, this, [this]() { rotateImage(-90); });
    centerLayout->addWidget(m_rotateLeftBtn);

    m_rotateRightBtn = createBottomBtn("rotate-right");
    m_rotateRightBtn->setToolTip(tr("Rotate right") + " (Ctrl+R)");
    connect(m_rotateRightBtn, &QPushButton::clicked, this, [this]() { rotateImage(90); });
    centerLayout->addWidget(m_rotateRightBtn);

    m_copyBtn = createBottomBtn("copy");
    m_copyBtn->setToolTip(tr("Copy Image"));
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        if (m_pixmapItem && !m_pixmapItem->pixmap().isNull()) {
            QApplication::clipboard()->setPixmap(m_pixmapItem->pixmap());
            qInfo() << "Image copied to clipboard:" << QFileInfo(m_currentViewModel.filePath).fileName();
        }
    });
    centerLayout->addWidget(m_copyBtn);

    m_deleteBtn = createBottomBtn("delete");
    m_deleteBtn->setToolTip(tr("Delete Image"));
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteImageRequested();
    });
    centerLayout->addWidget(m_deleteBtn);

    m_centerContainer->adjustSize();
    m_centerContainer->setFixedSize(m_centerContainer->sizeHint());

    m_fullscreenBtn = createBottomBtn("fullscreen");
    m_fullscreenBtn->setToolTip(tr("Fullscreen") + " (F11)");
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &MainWindow::toggleFullscreen);
    bottomLayout->addWidget(m_fullscreenBtn);

    if (auto *lay = centralWidget()->layout()) {
        lay->addWidget(m_bottomBar);
    }
    m_bottomBarInLayout = true;

    QTimer::singleShot(0, this, [this]() { updateCenterContainerPos(); });
}

void MainWindow::updateCenterContainerPos() {
    if (!m_bottomBar || !m_centerContainer)
        return;

    int x = (m_bottomBar->width() - m_centerContainer->width()) / 2;
    int y = (m_bottomBar->height() - m_centerContainer->height()) / 2;
    m_centerContainer->move(x, y);

    if (!m_infoContainer)
        return;

    int infoLeft = m_bottomBar->layout()->contentsMargins().left();
    int spacing = qobject_cast<QHBoxLayout *>(m_infoContainer->layout())->spacing();
    const int margin = 3;

    int sizeW = m_fileSizeLabel->width();
    int dimW = m_fileDimensionLabel->width();
    int fmtW = m_fileFormatLabel->width();

    int widthFull = sizeW + spacing + dimW + spacing + fmtW;
    int widthNoFmt = sizeW + spacing + dimW;
    int widthNoFmtDim = sizeW;

    if (infoLeft + widthFull + margin <= x) {
        m_fileSizeLabel->setVisible(true);
        m_fileDimensionLabel->setVisible(true);
        m_fileFormatLabel->setVisible(true);
    } else if (infoLeft + widthNoFmt + margin <= x) {
        m_fileSizeLabel->setVisible(true);
        m_fileDimensionLabel->setVisible(true);
        m_fileFormatLabel->setVisible(false);
    } else if (infoLeft + widthNoFmtDim + margin <= x) {
        m_fileSizeLabel->setVisible(true);
        m_fileDimensionLabel->setVisible(false);
        m_fileFormatLabel->setVisible(false);
    } else {
        m_fileSizeLabel->setVisible(false);
        m_fileDimensionLabel->setVisible(false);
        m_fileFormatLabel->setVisible(false);
    }
}

MainWindow::ResizeEdge MainWindow::getResizeEdge(const QPoint &pos) const {
    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();
    int m = kResizeMargin;

    bool left = x <= m;
    bool right = x >= w - m;
    bool top = y <= m;
    bool bottom = y >= h - m;

    if (top && left)
        return ResizeEdge::TopLeft;
    if (top && right)
        return ResizeEdge::TopRight;
    if (bottom && left)
        return ResizeEdge::BottomLeft;
    if (bottom && right)
        return ResizeEdge::BottomRight;
    if (left)
        return ResizeEdge::Left;
    if (right)
        return ResizeEdge::Right;
    if (top)
        return ResizeEdge::Top;
    if (bottom)
        return ResizeEdge::Bottom;
    return ResizeEdge::None;
}

void MainWindow::updateCursorForResize(ResizeEdge edge) {
    if (m_resizing)
        return;
    Qt::CursorShape shape = Qt::ArrowCursor;
    switch (edge) {
    case ResizeEdge::Left:
    case ResizeEdge::Right:
        shape = Qt::SizeHorCursor;
        break;
    case ResizeEdge::Top:
    case ResizeEdge::Bottom:
        shape = Qt::SizeVerCursor;
        break;
    case ResizeEdge::TopLeft:
    case ResizeEdge::BottomRight:
        shape = Qt::SizeFDiagCursor;
        break;
    case ResizeEdge::TopRight:
    case ResizeEdge::BottomLeft:
        shape = Qt::SizeBDiagCursor;
        break;
    default:
        shape = Qt::ArrowCursor;
        break;
    }
    if (QApplication::overrideCursor()) {
        if (QApplication::overrideCursor()->shape() != shape) {
            QApplication::changeOverrideCursor(QCursor(shape));
        }
    } else {
        QApplication::setOverrideCursor(QCursor(shape));
    }
}

void MainWindow::clearResizeCursor() {
    if (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_bottomBar && isFullScreen()) {
        int barHeight = m_bottomBar->height();
        m_bottomBar->setGeometry(0, height() - barHeight, width(), barHeight);
    }
    updateCenterContainerPos();
}

void MainWindow::createMenus() {
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setVisible(false);

    m_fileMenu = menuBar->addMenu(tr("&File"));

    m_openAction = new QAction(tr("&Open Image"), this);
    m_openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openImage);
    m_fileMenu->addAction(m_openAction);

    m_fileMenu->addSeparator();

    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    m_fileMenu->addAction(m_exitAction);

    m_viewMenu = menuBar->addMenu(tr("&View"));

    m_infoToggle = new QAction(tr("&Image Information"), this);
    m_infoToggle->setShortcut(QKeySequence("Ctrl+I"));
    m_infoToggle->setCheckable(true);
    m_infoToggle->setChecked(m_generalSettings.showInfoPanel);
    connect(m_infoToggle, &QAction::toggled, this, &MainWindow::toggleInfoPanel);
    m_viewMenu->addAction(m_infoToggle);

    QMenu *themeMenu = m_viewMenu->addMenu(tr("Theme"));
    QActionGroup *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_darkAction = new QAction(tr("Dark"), this);
    m_darkAction->setCheckable(true);
    m_darkAction->setActionGroup(themeGroup);
    connect(m_darkAction, &QAction::triggered, this, [this]() { switchTheme("dark"); });
    themeMenu->addAction(m_darkAction);

    m_lightAction = new QAction(tr("Light"), this);
    m_lightAction->setCheckable(true);
    m_lightAction->setActionGroup(themeGroup);
    connect(m_lightAction, &QAction::triggered, this, [this]() { switchTheme("light"); });
    themeMenu->addAction(m_lightAction);

    m_darkAction->setChecked(m_appearanceSettings.theme == "dark");
    m_lightAction->setChecked(m_appearanceSettings.theme == "light");

    m_settingsMenu = menuBar->addMenu(tr("&Settings"));
    m_settingsAction = new QAction(tr("Application Settings"), this);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    m_settingsMenu->addAction(m_settingsAction);

    setMenuBar(menuBar);
}

void MainWindow::openImage() {
    if (m_isFileDialogOpen)
        return;

    m_isFileDialogOpen = true;
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Image"), "",
                                                    AppConstants::imageFilterString());
    m_isFileDialogOpen = false;

    if (!filePath.isEmpty()) {
        emit imageOpenRequested(filePath);
    }
}

void MainWindow::toggleInfoPanel(bool visible) {
    m_infoDock->setVisible(visible);
    emit infoPanelVisibilityChanged(visible);
}

void MainWindow::openSettings() {
    emit openSettingsRequested();
}

void MainWindow::switchTheme(const QString &theme) {
    emit themeChangeRequested(theme);
}

void MainWindow::zoomIn() {
    emit zoomInRequested();
}

void MainWindow::zoomOut() {
    emit zoomOutRequested();
}

void MainWindow::actualSize() {
    emit actualSizeRequested();
}

void MainWindow::fitToWindow() {
    emit fitToWindowRequested();
}

void MainWindow::toggleFitActualSize() {
    emit toggleFitActualSizeRequested();
}

void MainWindow::rotateImage(int angle) {
    emit rotateRequested(angle);
}

void MainWindow::mirrorImage() {
    emit mirrorRequested();
}

void MainWindow::navigateFolderImage(int direction) {
    if (direction > 0)
        emit navigateNextRequested();
    else
        emit navigatePreviousRequested();
}

void MainWindow::jumpToImage(int index) {
    emit jumpToImageRequested(index);
}

void MainWindow::onImageLoaded(const ImageViewModel &viewModel) {
    m_currentViewModel = viewModel;

    if (viewModel.isNull || viewModel.pixmap.isNull()) {
        qWarning() << "Failed to load image:" << QFileInfo(viewModel.filePath).fileName();
        m_loadingLabel->setVisible(false);
        m_progressBar->setVisible(false);
        updateTitleBarTitle();
        updateBottomBarInfo();
        return;
    }

    resetCanvas();
    m_graphicsScene->clear();

    m_pixmapItem = new QGraphicsPixmapItem();
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    m_graphicsScene->addItem(m_pixmapItem);

    m_currentViewState = viewModel.viewState;
    applyViewState();

    m_graphicsView->horizontalScrollBar()->setValue(0);
    m_graphicsView->verticalScrollBar()->setValue(0);

    m_loadingLabel->setVisible(false);
    m_progressBar->setVisible(false);
    qInfo() << "Image loaded:" << QFileInfo(viewModel.filePath).fileName();

    updateTitleBarTitle();
    updateBottomBarInfo();
}

void MainWindow::onInfoReady(const ImageInfo &info) {
    m_infoTree->clear();

    auto addSection = [this](const QString &title, const QMap<QString, QString> &data) {
        if (data.isEmpty())
            return;
        QTreeWidgetItem *root = new QTreeWidgetItem(m_infoTree, {title});
        for (auto it = data.begin(); it != data.end(); ++it) {
            new QTreeWidgetItem(root, {QString("%1: %2").arg(it.key(), it.value())});
        }
        root->setExpanded(true);
    };

    auto addSectionVariant = [this](const QString &title, const QMap<QString, QVariant> &data) {
        if (data.isEmpty())
            return;
        QTreeWidgetItem *root = new QTreeWidgetItem(m_infoTree, {title});
        for (auto it = data.begin(); it != data.end(); ++it) {
            new QTreeWidgetItem(root, {QString("%1: %2").arg(it.key(), it.value().toString())});
        }
        root->setExpanded(true);
    };

    addSection(tr("File Information"), info.fileInfo);
    addSection(tr("Image Information"), info.imageInfo);
    addSectionVariant(tr("EXIF Information"), info.exifInfo);

    if (!info.error.isEmpty()) {
        QTreeWidgetItem *errRoot = new QTreeWidgetItem(m_infoTree, {"Error"});
        new QTreeWidgetItem(errRoot, {info.error});
        errRoot->setExpanded(true);
    }
}

void MainWindow::onProgress(int value) {
    m_progressBar->setValue(value);
}

void MainWindow::onViewStateChanged(const ViewState &state) {
    m_currentViewState = state;
    applyViewState();
}

void MainWindow::applyViewState() {
    if (!m_pixmapItem || m_currentViewModel.pixmap.isNull())
        return;

    QTransform transform;
    if (m_currentViewState.mirrored)
        transform.scale(-1, 1);
    if (m_currentViewState.rotation != 0)
        transform.rotate(m_currentViewState.rotation);

    if (transform.isIdentity()) {
        m_pixmapItem->setPixmap(m_currentViewModel.pixmap);
    } else {
        m_pixmapItem->setPixmap(m_currentViewModel.pixmap.transformed(transform, Qt::SmoothTransformation));
    }

    m_graphicsView->resetTransform();
    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());

    if (m_currentViewState.isFitToWindow) {
        m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
        double actualScale = m_graphicsView->transform().m11();
        m_currentViewState.scaleFactor = actualScale;
        emit actualScaleFactorChanged(actualScale);
    } else {
        m_graphicsView->scale(m_currentViewState.scaleFactor, m_currentViewState.scaleFactor);
    }

    updateBottomBarInfo();
    refreshIcons();
}

void MainWindow::onSettingsApplied(const GeneralSettings &g, const PerformanceSettings &p, const AppearanceSettings &a) {
    m_generalSettings = g;
    m_performanceSettings = p;
    m_appearanceSettings = a;

    if (!g.windowGeometry.isEmpty()) {
        restoreGeometry(g.windowGeometry);
    } else if (g.defaultWindowState == "maximized") {
        showMaximized();
    } else if (g.defaultWindowState == "fullscreen") {
        showFullScreen();
    }

    m_infoDock->setVisible(g.showInfoPanel);
    m_infoToggle->setChecked(g.showInfoPanel);

    QFont appFont(a.uiFont, a.uiFontSize);
    QApplication::setFont(appFont);
    menuBar()->setFont(appFont);

    ThemeManager::instance().setCurrentTheme(a.theme);
    onThemeChanged();

    m_darkAction->setChecked(a.theme == "dark");
    m_lightAction->setChecked(a.theme == "light");
}

void MainWindow::applyStyleSheet() {
    const AppearanceSettings a = m_appearanceSettings;
    const Theme t = ThemeManager::instance().currentTheme();
    const auto c = [](const QColor &color) { return color.name(QColor::HexArgb); };

    QString style = QString(
                        "QMainWindow, QDockWidget, QTreeWidget, QScrollArea, QWidget {"
                        "  background-color: %1; color: %2; font-family: '%3'; font-size: %4pt; }"
                        "QMenuBar { background-color: %5; color: %6; border-bottom: 1px solid %7; }"
                        "QMenuBar::item:selected { background-color: %8; }"
                        "QMenu { background-color: %22; border: 1px solid %23; padding: 6px 0px; }"
                        "QMenu::item { padding: 8px 32px 8px 16px; margin: 0px 4px; border-radius: 4px; color: %6; min-width: 160px; }"
                        "QMenu::item:selected { background-color: %24; color: %25; }"
                        "QMenu::item:checked { color: %25; }"
                        "QMenu::item:checked:selected { background-color: %24; }"
                        "QMenu::item:disabled { color: %27; background-color: transparent; }"
                        "QMenu::separator { height: 1px; background-color: %26; margin: 6px 12px; }"
                        "QMenu::icon { padding-left: 12px; }"
                        "QTreeWidget::item:selected { background-color: %9; color: #FFFFFF; }"
                        "QProgressBar { border: 1px solid %7; border-radius: 3px; text-align: center;"
                        "  background-color: %10; color: %2; }"
                        "QProgressBar::chunk { background-color: %9; }"
                        "QScrollBar:vertical { border: none; background: %11; width: 12px; margin: 0px; }"
                        "QScrollBar::handle:vertical { background: %12; border-radius: 6px; min-height: 30px; }"
                        "QScrollBar::handle:vertical:hover { background: %13; }"
                        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; height: 0px; }"
                        "QScrollBar:horizontal { border: none; background: %11; height: 12px; margin: 0px; }"
                        "QScrollBar::handle:horizontal { background: %12; border-radius: 6px; min-width: 30px; }"
                        "QScrollBar::handle:horizontal:hover { background: %13; }"
                        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { border: none; background: none; width: 0px; }"
                        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,"
                        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
                        "#titleBar { background-color: %14; border-bottom: 1px solid %7; }"
                        "#titleLabel { color: %15; font-size: 13px; padding-left: 6px; }"
                        "#titleBtn { background-color: transparent; border: none; border-radius: 0px; }"
                        "#titleBtn:hover { background-color: %16; }"
                        "#pinBtn { background-color: transparent; border: none; border-radius: 0px; }"
                        "#pinBtn:checked { background-color: %28; }"
                        "#closeBtn { background-color: transparent; border: none; border-radius: 0px; }"
                        "#closeBtn:hover { background-color: %17; }"
                        "#bottomBar { background-color: %18; border-top: 1px solid %7; }"
                        "#bottomBar[fullscreen=\"true\"] { background-color: %20; border-top: 1px solid %7; }"
                        "#centerContainer { background-color: transparent; }"
                        "#fileInfoLabel { color: %15; font-size: 12px; padding: 0 8px; }"
                        "#pageContainer { background-color: transparent; }"
                        "#pageLabel { color: %15; font-size: 12px; background-color: transparent; border: 1px solid %21; border-radius: 4px; padding: 2px 4px; }"
                        "#pageEdit { color: %15; font-size: 12px; background-color: transparent; border: 1px solid %21; border-radius: 4px; padding: 2px 4px; }"
                        "#bottomBtn { background-color: transparent; border: none; border-radius: 4px; }"
                        "#bottomBtn:hover { background-color: %16; }"
                        "#bottomBtn[fullscreen=\"true\"]:hover { background-color: %29; }"
                        "#infoBlock { background-color: %18; color: %15; font-size: 11px; border-radius: 4px; padding: 2px 8px; }"
                        "#infoBlock[fullscreen=\"true\"] { background-color: %30; color: %15; }"
                        "QGraphicsView { background-color: %19; border: none; }")
                        .arg(c(t.windowBackground), c(t.titleBarText), a.uiFont, QString::number(a.uiFontSize))
                        .arg(c(t.titleBarBackground), c(t.menuText), c(t.border), c(t.selected), c(t.accent), c(t.progressBackground), c(t.scrollBackground), c(t.scrollHandle), c(t.scrollHandleHover))
                        .arg(c(t.titleBarBackground), c(t.titleBarText), c(t.buttonHover), c(t.closeHover), c(t.bottomBarBackground), c(t.viewBackground))
                        .arg(c(t.bottomBarBackgroundFullscreen))
                        .arg(c(t.pageLabelBorder))
                        .arg(c(t.menuBackground), c(t.menuBorder), c(t.menuHoverBackground), c(t.menuHoverText), c(t.menuSeparator), c(t.menuDisabled))
                        .arg(c(t.selected))
                        .arg(c(t.bottomBtnHoverFullscreen))
                        .arg(c(t.infoBlockBackgroundFullscreen));

    setStyleSheet(style);

    if (m_bottomBar) {
        m_bottomBar->setProperty("fullscreen", isFullScreen());
        m_bottomBar->style()->unpolish(m_bottomBar);
        m_bottomBar->style()->polish(m_bottomBar);
    }
}

QIcon MainWindow::themedIcon(const QString &name) {
    return ThemeManager::instance().icon(name);
}

void MainWindow::refreshIcons() {
    if (m_titleIcon) {
        m_titleIcon->setPixmap(themedIcon("folder-open").pixmap(16, 16));
    }
    if (m_menuBtn)
        m_menuBtn->setIcon(themedIcon("menu"));
    if (m_pinBtn) {
        bool pinned = windowFlags() & Qt::WindowStaysOnTopHint;
        m_pinBtn->setIcon(themedIcon(pinned ? "pin-off" : "pin"));
        m_pinBtn->setChecked(pinned);
    }
    if (m_minBtn)
        m_minBtn->setIcon(themedIcon("minimize"));
    if (m_maxBtn)
        updateMaximizeIcon();
    if (m_closeBtn)
        m_closeBtn->setIcon(themedIcon("close"));

    if (m_prevBtn)
        m_prevBtn->setIcon(themedIcon("chevron-left"));
    if (m_nextBtn)
        m_nextBtn->setIcon(themedIcon("chevron-right"));
    if (m_fitBtn) {
        if (m_currentViewState.isFitToWindow) {
            m_fitBtn->setIcon(themedIcon("actual-size"));
            m_fitBtn->setToolTip(tr("Actual Size"));
        } else {
            m_fitBtn->setIcon(themedIcon("fit-screen"));
            m_fitBtn->setToolTip(tr("Fit to Window"));
        }
    }
    if (m_zoomOutBtn)
        m_zoomOutBtn->setIcon(themedIcon("zoom-out"));
    if (m_zoomInBtn)
        m_zoomInBtn->setIcon(themedIcon("zoom-in"));
    if (m_rotateLeftBtn)
        m_rotateLeftBtn->setIcon(themedIcon("rotate-left"));
    if (m_rotateRightBtn)
        m_rotateRightBtn->setIcon(themedIcon("rotate-right"));
    if (m_copyBtn)
        m_copyBtn->setIcon(themedIcon("copy"));
    if (m_deleteBtn)
        m_deleteBtn->setIcon(themedIcon("delete"));
    if (m_fullscreenBtn) {
        const QString iconName = m_fullscreenBtn->toolTip().startsWith(tr("Exit")) ? "fullscreen-exit" : "fullscreen";
        m_fullscreenBtn->setIcon(themedIcon(iconName));
    }
}

static QString elideFileName(const QString &fileName, int maxLength) {
    if (fileName.length() <= maxLength) {
        return fileName;
    }
    const QString ellipsis = QStringLiteral("...");
    int keep = maxLength - ellipsis.length();
    if (keep <= 0) {
        return ellipsis;
    }
    int left = keep / 2;
    int right = keep - left;
    QString result = fileName.left(left) + ellipsis + fileName.right(right);
    qInfo() << "Title truncated:" << fileName << "->" << result;
    return result;
}

void MainWindow::updateTitleBarTitle() {
    if (m_currentViewModel.filePath.isEmpty()) {
        m_titleLabel->setText(tr("InfiniteSight"));
    } else {
        QString fileName = QFileInfo(m_currentViewModel.filePath).fileName();
        m_titleLabel->setText(elideFileName(fileName, 50));
    }
}

void MainWindow::updateBottomBarInfo() {
    if (m_currentViewModel.filePath.isEmpty()) {
        if (m_fileInfoLabel)
            m_fileInfoLabel->setText("");
        if (m_fileSizeLabel)
            m_fileSizeLabel->setText("");
        if (m_fileDimensionLabel)
            m_fileDimensionLabel->setText("");
        if (m_fileFormatLabel)
            m_fileFormatLabel->setText("");
        m_pageLabel->setText("0/0");
        m_zoomCombo->setText("100%");
        return;
    }

    QFileInfo fi(m_currentViewModel.filePath);
    QString ext = fi.suffix().toUpper();
    QString sizeStr;
    qint64 fileSize = m_currentViewModel.fileSize;
    if (fileSize < 1024) {
        sizeStr = QString("%1B").arg(fileSize);
    } else if (fileSize < 1024 * 1024) {
        sizeStr = QString("%1K").arg(fileSize / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1M").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 2);
    }

    if (m_fileSizeLabel)
        m_fileSizeLabel->setText(sizeStr);
    if (m_fileDimensionLabel)
        m_fileDimensionLabel->setText(QString("%1x%2").arg(m_currentViewModel.imageWidth).arg(m_currentViewModel.imageHeight));
    if (m_fileFormatLabel)
        m_fileFormatLabel->setText(ext);

    int curr = m_currentViewModel.currentIndex >= 0 ? m_currentViewModel.currentIndex + 1 : 1;
    int total = m_currentViewModel.total > 0 ? m_currentViewModel.total : 1;
    m_pageLabel->setText(QString("%1/%2").arg(curr).arg(total));

    int zoomPercent = qRound(m_currentViewState.scaleFactor * 100);
    m_zoomCombo->setText(QString("%1%").arg(zoomPercent));
}

void MainWindow::onMinimize() {
    showMinimized();
}

void MainWindow::onMaximize() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeIcon();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::onClose() {
    close();
}

void MainWindow::updateMaximizeIcon() {
    if (m_maxBtn) {
        if (isMaximized()) {
            m_maxBtn->setIcon(themedIcon("restore"));
        } else {
            m_maxBtn->setIcon(themedIcon("maximize"));
        }
    }
}

void MainWindow::toggleFullscreen() {
    if (isFullScreen()) {
        m_bottomBarTimer->stop();
        if (m_bottomBar) {
            m_bottomBar->setVisible(true);
            if (!m_bottomBarInLayout) {
                if (auto *lay = centralWidget()->layout()) {
                    lay->addWidget(m_bottomBar);
                    m_bottomBarInLayout = true;
                }
            }
        }
        showNormal();
        if (m_titleBar)
            m_titleBar->setVisible(true);
        if (m_infoContainer)
            m_infoContainer->setVisible(true);
        if (m_fullscreenBtn) {
            m_fullscreenBtn->setIcon(themedIcon("fullscreen"));
            m_fullscreenBtn->setToolTip(tr("Fullscreen") + " (F11)");
        }
        applyStyleSheet();
    } else {
        if (m_bottomBar) {
            if (m_bottomBarInLayout) {
                if (auto *lay = centralWidget()->layout()) {
                    lay->removeWidget(m_bottomBar);
                    m_bottomBarInLayout = false;
                }
            }
            m_bottomBar->setParent(centralWidget());
            m_bottomBar->setVisible(false);
            m_bottomBar->raise();
        }
        showFullScreen();
        if (m_titleBar)
            m_titleBar->setVisible(false);
        if (m_infoContainer)
            m_infoContainer->setVisible(false);
        if (m_fullscreenBtn) {
            m_fullscreenBtn->setIcon(themedIcon("fullscreen-exit"));
            m_fullscreenBtn->setToolTip(tr("Exit Fullscreen") + " (F11)");
        }
        applyStyleSheet();
    }
}

void MainWindow::hideBottomBarAnimated() {
    if (m_bottomBar && isFullScreen())
        m_bottomBar->setVisible(false);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if ((obj == m_graphicsView || obj == m_graphicsView->viewport()) && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Left) {
            navigateFolderImage(-1);
            keyEvent->accept();
            return true;
        } else if (keyEvent->key() == Qt::Key_Right) {
            navigateFolderImage(1);
            keyEvent->accept();
            return true;
        }
    }

    if (obj == m_pageLabel && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && m_currentViewModel.total > 0) {
            m_pageLabel->setVisible(false);
            m_pageEdit->setText(QString::number(m_currentViewModel.currentIndex + 1));
            m_pageEdit->setVisible(true);
            m_pageEdit->setFocus();
            m_pageEdit->selectAll();
            if (m_bottomBarTimer)
                m_bottomBarTimer->stop();
            return true;
        }
    }

    // 无边框窗口边缘调整大小（最大化/全屏时禁用）
    if (!isFullScreen() && !isMaximized()) {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            QPoint globalPos = me->globalPosition().toPoint();
            QPoint localPos = mapFromGlobal(globalPos);

            if (m_resizing && (me->buttons() & Qt::LeftButton)) {
                QPoint delta = globalPos - m_dragPos;
                m_dragPos = globalPos;

                QRect geo = geometry();
                QSize minSz = minimumSize();
                int newW = geo.width();
                int newH = geo.height();
                int newX = geo.x();
                int newY = geo.y();

                switch (m_resizeEdge) {
                case ResizeEdge::Right:
                    newW = qMax(minSz.width(), geo.width() + delta.x());
                    break;
                case ResizeEdge::Left:
                    newW = qMax(minSz.width(), geo.width() - delta.x());
                    if (newW > minSz.width())
                        newX = geo.x() + delta.x();
                    break;
                case ResizeEdge::Bottom:
                    newH = qMax(minSz.height(), geo.height() + delta.y());
                    break;
                case ResizeEdge::Top:
                    newH = qMax(minSz.height(), geo.height() - delta.y());
                    if (newH > minSz.height())
                        newY = geo.y() + delta.y();
                    break;
                case ResizeEdge::TopLeft:
                    newW = qMax(minSz.width(), geo.width() - delta.x());
                    newH = qMax(minSz.height(), geo.height() - delta.y());
                    if (newW > minSz.width())
                        newX = geo.x() + delta.x();
                    if (newH > minSz.height())
                        newY = geo.y() + delta.y();
                    break;
                case ResizeEdge::TopRight:
                    newW = qMax(minSz.width(), geo.width() + delta.x());
                    newH = qMax(minSz.height(), geo.height() - delta.y());
                    if (newH > minSz.height())
                        newY = geo.y() + delta.y();
                    break;
                case ResizeEdge::BottomLeft:
                    newW = qMax(minSz.width(), geo.width() - delta.x());
                    newH = qMax(minSz.height(), geo.height() + delta.y());
                    if (newW > minSz.width())
                        newX = geo.x() + delta.x();
                    break;
                case ResizeEdge::BottomRight:
                    newW = qMax(minSz.width(), geo.width() + delta.x());
                    newH = qMax(minSz.height(), geo.height() + delta.y());
                    break;
                default:
                    break;
                }
                setGeometry(newX, newY, newW, newH);
                return true;
            }

            if (!m_dragging) {
                ResizeEdge edge = getResizeEdge(localPos);
                if (edge != ResizeEdge::None) {
                    updateCursorForResize(edge);
                } else {
                    clearResizeCursor();
                }
            }
        } else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                QPoint globalPos = me->globalPosition().toPoint();
                QPoint localPos = mapFromGlobal(globalPos);
                ResizeEdge edge = getResizeEdge(localPos);
                if (edge != ResizeEdge::None) {
                    m_resizing = true;
                    m_resizeEdge = edge;
                    m_dragPos = globalPos;
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_resizing) {
                m_resizing = false;
                m_resizeEdge = ResizeEdge::None;
                clearResizeCursor();
                return true;
            }
        } else if (event->type() == QEvent::Leave) {
            if (!m_resizing && !m_dragging) {
                clearResizeCursor();
            }
        }
    }

    if (event->type() == QEvent::MouseMove && isFullScreen() && m_bottomBar) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        QPoint localPos = mapFromGlobal(me->globalPosition().toPoint());
        int barHeight = m_bottomBar->height();
        int triggerZone = barHeight + 20;
        if (localPos.y() >= height() - triggerZone) {
            if (!m_bottomBar->isVisible()) {
                m_bottomBar->setVisible(true);
                m_bottomBar->raise();
            }
            m_bottomBarTimer->stop();
        } else {
            if (m_bottomBar->isVisible() && !(m_pageEdit && m_pageEdit->isVisible())) {
                m_bottomBarTimer->start();
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
            if (isMaximized()) {
                showNormal();
                updateMaximizeIcon();
            }
            m_dragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        if (isMaximized()) {
            showNormal();
            updateMaximizeIcon();
        }
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
        return;
    }
    if (isFullScreen() && m_bottomBar) {
        int barHeight = m_bottomBar->height();
        int triggerZone = barHeight + 20;
        if (event->pos().y() >= height() - triggerZone) {
            if (!m_bottomBar->isVisible()) {
                m_bottomBar->setVisible(true);
                m_bottomBar->raise();
            }
            m_bottomBarTimer->stop();
        } else {
            if (m_bottomBar->isVisible() && !(m_pageEdit && m_pageEdit->isVisible())) {
                m_bottomBarTimer->start();
            }
        }
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        if (m_resizing) {
            m_resizing = false;
            m_resizeEdge = ResizeEdge::None;
            clearResizeCursor();
        }
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
        onMaximize();
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::onThemeChanged() {
    applyStyleSheet();
    refreshIcons();
}

void MainWindow::resetCanvas() {
    m_graphicsView->resetTransform();
    m_graphicsView->horizontalScrollBar()->setValue(0);
    m_graphicsView->verticalScrollBar()->setValue(0);
    m_graphicsScene->clearSelection();
    m_graphicsView->centerOn(0, 0);
    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        QString ext = QFileInfo(path).suffix().toLower();
        if (AppConstants::supportedImageExtensions().contains(ext)) {
            emit imageOpenRequested(path);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit windowCloseRequested();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        navigateFolderImage(-1);
    } else if (event->key() == Qt::Key_Right) {
        navigateFolderImage(1);
    } else if (event->key() == Qt::Key_F11) {
        toggleFullscreen();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::showMenu() {
    QMenu menu(this);

    menu.addAction(tr("Open Image"), QKeySequence("Ctrl+O"), this, &MainWindow::openImage);
    menu.addSeparator();

    QMenu *themeMenu = menu.addMenu(tr("Change Theme"));
    QAction *darkAction = themeMenu->addAction(tr("Professional Dark"));
    darkAction->setCheckable(true);
    darkAction->setChecked(m_appearanceSettings.theme == "dark");
    connect(darkAction, &QAction::triggered, this, [this]() { switchTheme("dark"); });

    QAction *lightAction = themeMenu->addAction(tr("Classic White"));
    lightAction->setCheckable(true);
    lightAction->setChecked(m_appearanceSettings.theme == "light");
    connect(lightAction, &QAction::triggered, this, [this]() { switchTheme("light"); });

    menu.addAction(tr("Settings"), QKeySequence("F10"), this, &MainWindow::openSettings);
    menu.addSeparator();
    menu.addAction(tr("Exit"), QKeySequence("Ctrl+Q"), this, &QWidget::close);

    QPoint pos = m_menuBtn->mapToGlobal(QPoint(0, m_menuBtn->height()));
    menu.exec(pos);
}
