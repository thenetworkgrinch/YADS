#pragma once

#include <QObject>
#include <QLoggingCategory>
#include <QMutex>
#include <QTextStream>
#include <QFile>
#include <QTimer>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(logger)

class Logger : public QObject
{
    Q_OBJECT
    
public:
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Critical = 3,
        Fatal = 4
    };
    Q_ENUM(LogLevel)
    
    static Logger& instance();
    
    void initialize();
    void shutdown();
    
    void setLogLevel(LogLevel level);
    void setFileLoggingEnabled(bool enabled);
    void setConsoleLoggingEnabled(bool enabled);
    void setMaxLogFiles(int maxFiles);
    void setMaxLogFileSize(qint64 maxSize);
    
    void debug(const QString& message, const QString& category = "general");
    void info(const QString& message, const QString& category = "general");
    void warning(const QString& message, const QString& category = "general");
    void critical(const QString& message, const QString& category = "general");
    void fatal(const QString& message, const QString& category = "general");
    
    QString getLogDirectory() const;
    QStringList getLogFiles() const;
    QString getLogContent(const QString& filename, int maxLines = -1) const;
    
signals:
    void logMessage(const QString& timestamp, const QString& level, 
                   const QString& category, const QString& message);

private slots:
    void rotateLogFiles();

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger() override;
    
    void writeToFile(const QString& message);
    void writeToConsole(const QString& message);
    void setupLogRotation();
    QString formatMessage(LogLevel level, const QString& category, const QString& message) const;
    
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
    
    LogLevel m_logLevel;
    bool m_fileLoggingEnabled;
    bool m_consoleLoggingEnabled;
    int m_maxLogFiles;
    qint64 m_maxLogFileSize;
    
    QString m_logDirectory;
    std::unique_ptr<QFile> m_currentLogFile;
    std::unique_ptr<QTextStream> m_logStream;
    QMutex m_logMutex;
    QTimer* m_rotationTimer;
    
    static Logger* s_instance;
};
