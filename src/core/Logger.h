#pragma once

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTextStream>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Critical,
    Fatal
};

class Logger : public QObject {
    Q_OBJECT

public:
    static Logger *instance();

    void initialize(const QString &logDirPath,
                    const QString &fileNamePrefix = QStringLiteral("InfiniteSight"));
    void shutdown();

    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;

    void writeLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    Q_DISABLE_COPY(Logger)

    void switchLogFileIfDateChanged();
    QString generateLogFilePath(const QDate &date) const;
    QString levelToString(LogLevel level) const;
    LogLevel qtMsgTypeToLevel(QtMsgType type) const;

    static Logger *s_instance;

    QString m_logDirPath;
    QString m_fileNamePrefix;
    LogLevel m_logLevel;

    QDate m_currentLogDate;
    QFile m_logFile;
    QTextStream m_textStream;
    mutable QMutex m_mutex;
};
