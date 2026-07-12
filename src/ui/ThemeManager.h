#pragma once

#include "Theme.h"

#include <QIcon>
#include <QMap>
#include <QObject>
#include <QStringList>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager &instance();

    bool setCurrentTheme(const QString &id);
    QString currentThemeId() const;
    Theme currentTheme() const;

    QStringList availableThemeIds() const;
    QList<Theme> availableThemes() const;
    Theme themeById(const QString &id) const;

    QIcon icon(const QString &name) const;

    // Convenience accessors (delegates to current theme)
    QString iconPrefix() const { return currentTheme().iconPrefix; }

signals:
    void themeChanged(const QString &id);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    QMap<QString, Theme> m_themes;
    QString m_currentId;
};
