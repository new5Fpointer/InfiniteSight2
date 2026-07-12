#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class ImageNavigator : public QObject {
    Q_OBJECT

public:
    explicit ImageNavigator(QObject *parent = nullptr);

    QString currentPath() const;
    QStringList folderImages() const;
    int currentIndex() const;
    int total() const;

    void goTo(const QString &imagePath);
    void next();
    void previous();
    void jumpTo(int index);
    void refreshAfterDeletion();

signals:
    void currentPathChanged(const QString &path, const QStringList &folderImages, int currentIndex);

private:
    void refreshFolderList(const QString &imagePath);
    static QStringList imageNameFilters();

    QString m_currentPath;
    QStringList m_folderImages;
    int m_currentIndex = -1;
};
