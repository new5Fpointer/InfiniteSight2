#include "LoadCoordinator.h"
#include "common/AppConstants.h"
#include "core/ImageCache.h"

#include <QDebug>
#include <QFileInfo>
#include <QUuid>

LoadCoordinator::LoadCoordinator(QObject *parent)
    : QObject(parent) {
}

LoadCoordinator::~LoadCoordinator() {
    stopCurrentJob();
}

void LoadCoordinator::load(const QString &filePath, const PerformanceSettings &perfSettings) {
    if (filePath.isEmpty())
        return;

    stopCurrentJob();

    m_currentJobId = QUuid::createUuid().toString();
    m_currentPath = filePath;

    bool cacheHit = ImageCache::instance().contains(filePath);
    if (cacheHit) {
        qDebug() << "Cache hit for:" << QFileInfo(filePath).fileName();
        CacheEntry entry = ImageCache::instance().getEntry(filePath);

        LoadResult cachedResult;
        cachedResult.pixmap = entry.pixmap;
        cachedResult.originalWidth = entry.originalWidth;
        cachedResult.originalHeight = entry.originalHeight;
        cachedResult.isDownsampled = entry.isDownsampled;

        ImageViewModel viewModel = buildViewModel(cachedResult);
        emit imageLoaded(viewModel);
        // 仍需异步收集信息
    }

    // 创建后台加载任务（缓存命中时仅用于收集信息）
    m_loader = new ImageLoader(filePath, perfSettings, m_currentJobId);
    m_thread = new QThread(this);
    m_loader->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_loader, &ImageLoader::load, Qt::QueuedConnection);
    connect(m_loader, &ImageLoader::infoReady, this, &LoadCoordinator::onInfoReady, Qt::QueuedConnection);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    if (!cacheHit) {
        connect(m_loader, &ImageLoader::loadResultReady, this, &LoadCoordinator::onLoadResultReady, Qt::QueuedConnection);
        connect(m_loader, &ImageLoader::progress, this, &LoadCoordinator::onProgress, Qt::QueuedConnection);
    }

    m_thread->start();
}

void LoadCoordinator::cancel() {
    stopCurrentJob();
}

void LoadCoordinator::stopCurrentJob() {
    if (m_loader) {
        m_loader->disconnect();
        m_loader->cancel();
    }

    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        if (!m_thread->wait(2000)) {
            m_thread->terminate();
            m_thread->wait();
        }
    }

    if (m_loader) {
        m_loader->deleteLater();
        m_loader = nullptr;
    }
    if (m_thread) {
        m_thread->deleteLater();
        m_thread = nullptr;
    }
}

void LoadCoordinator::onLoadResultReady(const LoadResult &result) {
    if (!result.pixmap.isNull()) {
        ImageCache::instance().insert(result.info.fileInfo.value(QStringLiteral("Path"), m_currentPath),
                                      result.pixmap,
                                      result.originalWidth,
                                      result.originalHeight,
                                      result.isDownsampled);
    }

    ImageViewModel viewModel = buildViewModel(result);
    emit imageLoaded(viewModel);

    // 清理
    if (m_thread) {
        m_thread->quit();
    }
}

void LoadCoordinator::onInfoReady(const ImageInfo &info) {
    emit infoReady(info);

    // 清理
    if (m_thread && !m_thread->isRunning()) {
        m_thread->quit();
    }
}

void LoadCoordinator::onProgress(int value) {
    emit progress(value);
}

ImageViewModel LoadCoordinator::buildViewModel(const LoadResult &result) const {
    ImageViewModel vm;
    vm.pixmap = result.pixmap;
    vm.filePath = m_currentPath;
    vm.fileName = QFileInfo(m_currentPath).fileName();
    vm.fileSize = QFileInfo(m_currentPath).size();
    vm.imageWidth = result.pixmap.width();
    vm.imageHeight = result.pixmap.height();
    vm.originalWidth = result.originalWidth;
    vm.originalHeight = result.originalHeight;
    vm.isDownsampled = result.isDownsampled;
    vm.info = result.info;
    vm.isNull = result.pixmap.isNull();
    return vm;
}
