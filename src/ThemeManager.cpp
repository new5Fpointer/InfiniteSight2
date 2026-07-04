#include "ThemeManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QStyleHints>

Theme Theme::dark() {
    Theme t;
    t.id = QStringLiteral("dark");
    t.displayName = QObject::tr("Dark");
    t.iconPrefix = QStringLiteral("dark");

    t.windowBackground = QColor(QStringLiteral("#2D2D30"));
    t.titleBarBackground = QColor(QStringLiteral("#2D2D30"));
    t.bottomBarBackground = QColor(QStringLiteral("#2D2D30"));
    t.bottomBarBackgroundFullscreen = QColor(QStringLiteral("#E62D2D30"));
    t.viewBackground = QColor(QStringLiteral("#1E1E1E"));
    t.infoBlockBackground = QColor(QStringLiteral("#2D2D30"));

    t.titleBarText = QColor(QStringLiteral("#E0E0E0"));
    t.menuText = QColor(QStringLiteral("#CCCCCC"));
    t.menuHoverText = QColor(QStringLiteral("#FFFFFF"));
    t.infoBlockText = QColor(QStringLiteral("#E0E0E0"));

    t.accent = QColor(QStringLiteral("#007ACC"));
    t.border = QColor(QStringLiteral("#1E1E1E"));
    t.selected = QColor(QStringLiteral("#3F3F46"));
    t.buttonHover = QColor(QStringLiteral("#3F3F46"));
    t.closeHover = QColor(QStringLiteral("#E81123"));
    t.pageLabelBorder = QColor(QStringLiteral("#555555"));

    t.menuBackground = QColor(QStringLiteral("#2D2D30"));
    t.menuBorder = QColor(QStringLiteral("#3F3F46"));
    t.menuHoverBackground = QColor(QStringLiteral("#3F3F46"));
    t.menuDisabled = QColor(QStringLiteral("#666666"));
    t.menuSeparator = QColor(QStringLiteral("#3F3F46"));

    t.scrollBackground = QColor(QStringLiteral("#404040"));
    t.scrollHandle = QColor(QStringLiteral("#606060"));
    t.scrollHandleHover = QColor(QStringLiteral("#808080"));

    t.progressBackground = QColor(QStringLiteral("#1E1E1E"));

    return t;
}

Theme Theme::light() {
    Theme t;
    t.id = QStringLiteral("light");
    t.displayName = QObject::tr("Light");
    t.iconPrefix = QStringLiteral("light");

    t.windowBackground = QColor(QStringLiteral("#FFFFFF"));
    t.titleBarBackground = QColor(QStringLiteral("#F3F3F3"));
    t.bottomBarBackground = QColor(QStringLiteral("#FFFFFF"));
    t.bottomBarBackgroundFullscreen = QColor(QStringLiteral("#E6FFFFFF"));
    t.viewBackground = QColor(QStringLiteral("#FFFFFF"));
    t.infoBlockBackground = QColor(QStringLiteral("#F3F3F3"));

    t.titleBarText = QColor(QStringLiteral("#000000"));
    t.menuText = QColor(QStringLiteral("#000000"));
    t.menuHoverText = QColor(QStringLiteral("#000000"));
    t.infoBlockText = QColor(QStringLiteral("#000000"));

    t.accent = QColor(QStringLiteral("#007ACC"));
    t.border = QColor(QStringLiteral("#CCCCCC"));
    t.selected = QColor(QStringLiteral("#E0E0E0"));
    t.buttonHover = QColor(QStringLiteral("#E5E5E5"));
    t.closeHover = QColor(QStringLiteral("#E81123"));
    t.pageLabelBorder = QColor(QStringLiteral("#CCCCCC"));

    t.menuBackground = QColor(QStringLiteral("#FFFFFF"));
    t.menuBorder = QColor(QStringLiteral("#E0E0E0"));
    t.menuHoverBackground = QColor(QStringLiteral("#E0E0E0"));
    t.menuDisabled = QColor(QStringLiteral("#999999"));
    t.menuSeparator = QColor(QStringLiteral("#E0E0E0"));

    t.scrollBackground = QColor(QStringLiteral("#F0F0F0"));
    t.scrollHandle = QColor(QStringLiteral("#C0C0C0"));
    t.scrollHandleHover = QColor(QStringLiteral("#A0A0A0"));

    t.progressBackground = QColor(QStringLiteral("#FFFFFF"));

    return t;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent) {
    Theme d = Theme::dark();
    Theme l = Theme::light();
    m_themes.insert(d.id, d);
    m_themes.insert(l.id, l);
    m_currentId = d.id;
}

ThemeManager &ThemeManager::instance() {
    static ThemeManager mgr;
    return mgr;
}

bool ThemeManager::setCurrentTheme(const QString &id) {
    if (!m_themes.contains(id))
        return false;
    if (m_currentId == id)
        return true;
    m_currentId = id;
    emit themeChanged(id);
    return true;
}

QString ThemeManager::currentThemeId() const {
    return m_currentId;
}

Theme ThemeManager::currentTheme() const {
    return themeById(m_currentId);
}

QStringList ThemeManager::availableThemeIds() const {
    return QStringList{m_themes.keys()};
}

QList<Theme> ThemeManager::availableThemes() const {
    return QList<Theme>{m_themes.values()};
}

Theme ThemeManager::themeById(const QString &id) const {
    auto it = m_themes.find(id);
    if (it != m_themes.end())
        return it.value();
    return Theme();
}

QIcon ThemeManager::icon(const QString &name) const {
    const QStringList prefixes{currentTheme().iconPrefix, QStringLiteral("dark"), QStringLiteral("light")};
    for (const QString &prefix : prefixes) {
        QString path = QStringLiteral(":/icons/%1/%2.svg").arg(prefix, name);
        if (QFile::exists(path))
            return QIcon(path);
    }
    return QIcon();
}
