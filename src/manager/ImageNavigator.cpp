#include "ImageNavigator.h"
#include "common/AppConstants.h"

#include <QDir>
#include <QFileInfo>

ImageNavigator::ImageNavigator(QObject *parent)
    : QObject(parent) {
}

QString ImageNavigator::currentPath() const {
    return m_currentPath;
}

QStringList ImageNavigator::folderImages() const {
    return m_folderImages;
}

int ImageNavigator::currentIndex() const {
    return m_currentIndex;
}

int ImageNavigator::total() const {
    return m_folderImages.size();
}

void ImageNavigator::goTo(const QString &imagePath) {
    if (imagePath.isEmpty())
        return;

    m_currentPath = imagePath;
    refreshFolderList(imagePath);

    m_currentIndex = m_folderImages.indexOf(imagePath);
    if (m_currentIndex < 0)
        m_currentIndex = 0;

    emit currentPathChanged(m_currentPath, m_folderImages, m_currentIndex);
}

void ImageNavigator::next() {
    if (m_folderImages.isEmpty())
        return;

    int newIndex = (m_currentIndex + 1) % m_folderImages.size();
    jumpTo(newIndex);
}

void ImageNavigator::previous() {
    if (m_folderImages.isEmpty())
        return;

    int newIndex = m_currentIndex - 1;
    if (newIndex < 0)
        newIndex = m_folderImages.size() - 1;
    jumpTo(newIndex);
}

void ImageNavigator::jumpTo(int index) {
    if (m_folderImages.isEmpty() || index < 0 || index >= m_folderImages.size())
        return;
    if (index == m_currentIndex)
        return;

    m_currentIndex = index;
    m_currentPath = m_folderImages[m_currentIndex];
    emit currentPathChanged(m_currentPath, m_folderImages, m_currentIndex);
}

void ImageNavigator::refreshFolderList(const QString &imagePath) {
    QFileInfo fi(imagePath);
    QDir dir(fi.absolutePath());

    if (!dir.exists()) {
        m_folderImages.clear();
        return;
    }

    dir.setNameFilters(imageNameFilters());
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::IgnoreCase);

    QStringList relativeFiles = dir.entryList();
    m_folderImages.clear();
    for (const QString &f : relativeFiles) {
        m_folderImages.append(dir.absoluteFilePath(f));
    }
}

QStringList ImageNavigator::imageNameFilters() {
    QStringList filters;
    for (const QString &ext : AppConstants::supportedImageExtensions()) {
        filters.append(QStringLiteral("*.%1").arg(ext));
    }
    return filters;
}

void ImageNavigator::refreshAfterDeletion() {
    if (m_currentPath.isEmpty())
        return;

    int oldIndex = m_currentIndex;
    refreshFolderList(m_currentPath);

    if (m_folderImages.isEmpty()) {
        m_currentPath.clear();
        m_currentIndex = -1;
        emit currentPathChanged(m_currentPath, m_folderImages, m_currentIndex);
        return;
    }

    int newIndex = oldIndex;
    if (newIndex >= m_folderImages.size())
        newIndex = m_folderImages.size() - 1;
    if (newIndex < 0)
        newIndex = 0;

    m_currentIndex = newIndex;
    m_currentPath = m_folderImages[m_currentIndex];
    emit currentPathChanged(m_currentPath, m_folderImages, m_currentIndex);
}
