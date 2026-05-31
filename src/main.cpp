#include <QApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"
#include "Logger.h"

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setApplicationName("InfiniteSight");
    app.setOrganizationName("InfiniteSight");

    QString logDir = QCoreApplication::applicationDirPath() + QStringLiteral("/logs");
    Logger::instance()->initialize(logDir);
    qInfo() << "Application started";

    MainWindow window;
    window.show();

    int result = app.exec();

    qInfo() << "Application exiting with code" << result;
    Logger::instance()->shutdown();

    return result;
}
