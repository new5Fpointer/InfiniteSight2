#pragma once

#include "common/ImageModel.h"
#include "core/ImageLoader.h"
#include <QObject>
#include <QPointer>
#include <QThread>

class LoadCoordinator : public QObject {
    Q_OBJECT

public:
    explicit LoadCoordinator(QObject *parent = nullptr);
    ~LoadCoordinator();

    void load(const QString &filePath, const PerformanceSettings &perfSettings);
    void cancel();

signals:
    void imageLoaded(const ImageViewModel &viewModel);
    void infoReady(const ImageInfo &info);
    void progress(int value);
    void loadFailed(const QString &filePath);

private slots:
    void onLoadResultReady(const LoadResult &result);
    void onInfoReady(const ImageInfo &info);
    void onProgress(int value);
    void onLoadError(const QString &filePath, const QString &jobId);

private:
    void stopCurrentJob();
    ImageViewModel buildViewModel(const LoadResult &result) const;

    QString m_currentJobId;
    QString m_currentPath;
    QPointer<ImageLoader> m_loader;
    QPointer<QThread> m_thread;
};
