#pragma once

#include <QColor>
#include <QString>

struct Theme {
    QString id;          // e.g. "dark", "light"
    QString displayName; // e.g. tr("Dark")
    QString iconPrefix;  // e.g. "dark" -> :/icons/dark/xxx.svg

    // Window / layout
    QColor windowBackground;
    QColor titleBarBackground;
    QColor bottomBarBackground;
    QColor bottomBarBackgroundFullscreen; // with alpha baked in
    QColor viewBackground;
    QColor infoBlockBackground;

    // Text
    QColor titleBarText;
    QColor menuText;
    QColor menuHoverText;
    QColor infoBlockText;

    // Interactive
    QColor accent;
    QColor border;
    QColor selected;
    QColor buttonHover;
    QColor closeHover;
    QColor pageLabelBorder;

    // Menu
    QColor menuBackground;
    QColor menuBorder;
    QColor menuHoverBackground;
    QColor menuDisabled;
    QColor menuSeparator;

    // Scrollbar
    QColor scrollBackground;
    QColor scrollHandle;
    QColor scrollHandleHover;

    // Progress
    QColor progressBackground;

    bool isValid() const { return !id.isEmpty(); }

    static Theme dark();
    static Theme light();
};
