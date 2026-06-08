#ifdef HAS_LIBVIPS
#include <vips/vips8>
#endif

#include "Logger.h"
#include "MainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
#ifdef HAS_LIBVIPS
    // 初始化 libvips，传递 argv[0] 作为程序名
    if (VIPS_INIT(argv[0])) {
        qCritical() << "Failed to initialize libvips";
        return 1;
    }
    qInfo() << "libvips initialized successfully";

    // 调整 vips 操作缓存，减少内存占用
    vips_cache_set_max_mem(500 * 1024 * 1024);  // 500MB
    vips_cache_set_max(100);                     // 最多缓存 100 个操作
    vips_cache_set_max_files(10);               // 最多打开 10 个文件

    // 如果传入了测试图片路径，进行测试加载
    if (argc > 1) {
        QString testPath = QString::fromLocal8Bit(argv[1]);
        qInfo() << "Testing vips image load:" << testPath;

        try {
            vips::VImage image = vips::VImage::thumbnail(testPath.toUtf8().constData(), 1024);
            qInfo() << "Vips test successful - Image size:" << image.width() << "x" << image.height();
        } catch (const vips::VError &e) {
            qCritical() << "Vips test failed:" << e.what();
        }
    }
#endif

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

#ifdef HAS_LIBVIPS
    vips_shutdown();
#endif

    return result;
}
