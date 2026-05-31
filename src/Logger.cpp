#include "Logger.h"
#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>

Logger *Logger::s_instance = nullptr;

static QtMessageHandler g_originalHandler = nullptr;

static void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Logger::instance()->writeLog(type, context, msg);

    if (g_originalHandler) {
        g_originalHandler(type, context, msg);
    }
}

Logger *Logger::instance() {
    if (!s_instance) {
        s_instance = new Logger();
    }
    return s_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent), m_logLevel(LogLevel::Debug) {
}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(const QString &logDirPath, const QString &fileNamePrefix) {
    QMutexLocker locker(&m_mutex);

    m_fileNamePrefix = fileNamePrefix;

    m_logDirPath = logDirPath;

    QDir dir(m_logDirPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    m_currentLogDate = QDate::currentDate();
    QString filePath = generateLogFilePath(m_currentLogDate);
    m_logFile.setFileName(filePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_textStream.setDevice(&m_logFile);
    }

    g_originalHandler = qInstallMessageHandler(logMessageHandler);
}

void Logger::shutdown() {
    QMutexLocker locker(&m_mutex);

    qInstallMessageHandler(g_originalHandler);
    g_originalHandler = nullptr;

    if (m_textStream.device()) {
        m_textStream.flush();
    }
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

LogLevel Logger::logLevel() const {
    QMutexLocker locker(&m_mutex);
    return m_logLevel;
}

void Logger::writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QMutexLocker locker(&m_mutex);

    if (!m_logFile.isOpen()) {
        return;
    }

    LogLevel level = qtMsgTypeToLevel(type);
    if (level < m_logLevel) {
        return;
    }

    switchLogFileIfDateChanged();

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    QString levelStr = levelToString(level);

    QString fileInfo;
    if (context.file && context.line > 0) {
        fileInfo = QStringLiteral(" [%1:%2]").arg(QString::fromUtf8(context.file)).arg(context.line);
    }

    QString line = QStringLiteral("[%1] [%2]%3 %4\n")
                       .arg(timestamp, levelStr, fileInfo, msg);

    m_textStream << line;
    m_textStream.flush();

    if (type == QtFatalMsg) {
        m_logFile.flush();
    }
}

void Logger::switchLogFileIfDateChanged() {
    QDate today = QDate::currentDate();
    if (today == m_currentLogDate) {
        return;
    }

    m_textStream.flush();
    m_logFile.close();

    m_currentLogDate = today;
    QString filePath = generateLogFilePath(m_currentLogDate);
    m_logFile.setFileName(filePath);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    m_textStream.setDevice(&m_logFile);
}

QString Logger::generateLogFilePath(const QDate &date) const {
    QString dateStr = date.toString(QStringLiteral("yyyy-MM-dd"));
    return m_logDirPath + QStringLiteral("/") + m_fileNamePrefix + QStringLiteral("_") + dateStr + QStringLiteral(".log");
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::Debug:
        return QStringLiteral("DEBUG");
    case LogLevel::Info:
        return QStringLiteral("INFO");
    case LogLevel::Warning:
        return QStringLiteral("WARN");
    case LogLevel::Critical:
        return QStringLiteral("ERROR");
    case LogLevel::Fatal:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("UNKNOWN");
}

LogLevel Logger::qtMsgTypeToLevel(QtMsgType type) const {
    switch (type) {
    case QtDebugMsg:
        return LogLevel::Debug;
    case QtInfoMsg:
        return LogLevel::Info;
    case QtWarningMsg:
        return LogLevel::Warning;
    case QtCriticalMsg:
        return LogLevel::Critical;
    case QtFatalMsg:
        return LogLevel::Fatal;
    }
    return LogLevel::Debug;
}
