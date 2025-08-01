#include "logger.h"
#include "constants.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <iostream>

Q_LOGGING_CATEGORY(logger, "logger")

Logger* Logger::s_instance = nullptr;

Logger& Logger::instance()
{
    if (!s_instance) {
        s_instance = new Logger();
    }
    return *s_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_logLevel(LogLevel::Info)
    , m_fileLoggingEnabled(true)
    , m_consoleLoggingEnabled(true)
    , m_maxLogFiles(10)
    , m_maxLogFileSize(10 * 1024 * 1024) // 10 MB
    , m_rotationTimer(new QTimer(this))
{
    // Set up log directory
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_logDirectory = QDir(appDataDir).absoluteFilePath(Constants::Paths::LOGS_DIR);
    
    // Create log directory if it doesn't exist
    QDir().mkpath(m_logDirectory);
    
    // Set up log rotation timer (check every hour)
    m_rotationTimer->setInterval(60 * 60 * 1000);
    connect(m_rotationTimer, &QTimer::timeout, this, &Logger::rotateLogFiles);
}

Logger::~Logger()
{
    shutdown();
}

void Logger::initialize()
{
    QMutexLocker locker(&m_logMutex);
    
    // Install custom message handler
    qInstallMessageHandler(&Logger::messageHandler);
    
    // Create initial log file
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString logFileName = QString("yads_%1.log").arg(timestamp);
    QString logFilePath = QDir(m_logDirectory).absoluteFilePath(logFileName);
    
    m_currentLogFile = std::make_unique<QFile>(logFilePath);
    if (m_currentLogFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = std::make_unique<QTextStream>(m_currentLogFile.get());
        m_logStream->setEncoding(QStringConverter::Utf8);
    } else {
        qWarning() << "Failed to open log file:" << logFilePath;
        m_fileLoggingEnabled = false;
    }
    
    // Start rotation timer
    m_rotationTimer->start();
    
    // Log initialization
    info("Logger initialized", "logger");
    info(QString("Log directory: %1").arg(m_logDirectory), "logger");
    info(QString("File logging: %1").arg(m_fileLoggingEnabled ? "enabled" : "disabled"), "logger");
    info(QString("Console logging: %1").arg(m_consoleLoggingEnabled ? "enabled" : "disabled"), "logger");
}

void Logger::shutdown()
{
    QMutexLocker locker(&m_logMutex);
    
    info("Logger shutting down", "logger");
    
    m_rotationTimer->stop();
    
    if (m_logStream) {
        m_logStream->flush();
        m_logStream.reset();
    }
    
    if (m_currentLogFile) {
        m_currentLogFile->close();
        m_currentLogFile.reset();
    }
    
    // Restore default message handler
    qInstallMessageHandler(nullptr);
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
    info(QString("Log level set to: %1").arg(static_cast<int>(level)), "logger");
}

void Logger::setFileLoggingEnabled(bool enabled)
{
    m_fileLoggingEnabled = enabled;
    info(QString("File logging %1").arg(enabled ? "enabled" : "disabled"), "logger");
}

void Logger::setConsoleLoggingEnabled(bool enabled)
{
    m_consoleLoggingEnabled = enabled;
    info(QString("Console logging %1").arg(enabled ? "enabled" : "disabled"), "logger");
}

void Logger::setMaxLogFiles(int maxFiles)
{
    m_maxLogFiles = maxFiles;
    info(QString("Max log files set to: %1").arg(maxFiles), "logger");
}

void Logger::setMaxLogFileSize(qint64 maxSize)
{
    m_maxLogFileSize = maxSize;
    info(QString("Max log file size set to: %1 bytes").arg(maxSize), "logger");
}

void Logger::debug(const QString& message, const QString& category)
{
    if (m_logLevel <= LogLevel::Debug) {
        QString formattedMessage = formatMessage(LogLevel::Debug, category, message);
        
        if (m_fileLoggingEnabled) {
            writeToFile(formattedMessage);
        }
        
        if (m_consoleLoggingEnabled) {
            writeToConsole(formattedMessage);
        }
        
        emit logMessage(QDateTime::currentDateTime().toString(Qt::ISODate), 
                       "DEBUG", category, message);
    }
}

void Logger::info(const QString& message, const QString& category)
{
    if (m_logLevel <= LogLevel::Info) {
        QString formattedMessage = formatMessage(LogLevel::Info, category, message);
        
        if (m_fileLoggingEnabled) {
            writeToFile(formattedMessage);
        }
        
        if (m_consoleLoggingEnabled) {
            writeToConsole(formattedMessage);
        }
        
        emit logMessage(QDateTime::currentDateTime().toString(Qt::ISODate), 
                       "INFO", category, message);
    }
}

void Logger::warning(const QString& message, const QString& category)
{
    if (m_logLevel <= LogLevel::Warning) {
        QString formattedMessage = formatMessage(LogLevel::Warning, category, message);
        
        if (m_fileLoggingEnabled) {
            writeToFile(formattedMessage);
        }
        
        if (m_consoleLoggingEnabled) {
            writeToConsole(formattedMessage);
        }
        
        emit logMessage(QDateTime::currentDateTime().toString(Qt::ISODate), 
                       "WARNING", category, message);
    }
}

void Logger::critical(const QString& message, const QString& category)
{
    if (m_logLevel <= LogLevel::Critical) {
        QString formattedMessage = formatMessage(LogLevel::Critical, category, message);
        
        if (m_fileLoggingEnabled) {
            writeToFile(formattedMessage);
        }
        
        if (m_consoleLoggingEnabled) {
            writeToConsole(formattedMessage);
        }
        
        emit logMessage(QDateTime::currentDateTime().toString(Qt::ISODate), 
                       "CRITICAL", category, message);
    }
}

void Logger::fatal(const QString& message, const QString& category)
{
    QString formattedMessage = formatMessage(LogLevel::Fatal, category, message);
    
    if (m_fileLoggingEnabled) {
        writeToFile(formattedMessage);
    }
    
    if (m_consoleLoggingEnabled) {
        writeToConsole(formattedMessage);
    }
    
    emit logMessage(QDateTime::currentDateTime().toString(Qt::ISODate), 
                   "FATAL", category, message);
}

QString Logger::getLogDirectory() const
{
    return m_logDirectory;
}

QStringList Logger::getLogFiles() const
{
    QDir logDir(m_logDirectory);
    QStringList filters;
    filters << "*.log";
    return logDir.entryList(filters, QDir::Files, QDir::Time | QDir::Reversed);
}

QString Logger::getLogContent(const QString& filename, int maxLines) const
{
    QString filePath = QDir(m_logDirectory).absoluteFilePath(filename);
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("Error: Could not open log file %1").arg(filename);
    }
    
    QStringList lines;
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    
    while (!stream.atEnd()) {
        lines.append(stream.readLine());
    }
    
    if (maxLines > 0 && lines.size() > maxLines) {
        lines = lines.mid(lines.size() - maxLines);
    }
    
    return lines.join('\n');
}

void Logger::rotateLogFiles()
{
    QMutexLocker locker(&m_logMutex);
    
    // Check if current log file is too large
    if (m_currentLogFile && m_currentLogFile->size() > m_maxLogFileSize) {
        info("Rotating log file due to size limit", "logger");
        
        // Close current file
        if (m_logStream) {
            m_logStream->flush();
            m_logStream.reset();
        }
        m_currentLogFile->close();
        
        // Create new log file
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        QString logFileName = QString("yads_%1.log").arg(timestamp);
        QString logFilePath = QDir(m_logDirectory).absoluteFilePath(logFileName);
        
        m_currentLogFile = std::make_unique<QFile>(logFilePath);
        if (m_currentLogFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_logStream = std::make_unique<QTextStream>(m_currentLogFile.get());
            m_logStream->setEncoding(QStringConverter::Utf8);
        }
    }
    
    // Remove old log files
    QStringList logFiles = getLogFiles();
    while (logFiles.size() > m_maxLogFiles) {
        QString oldestFile = logFiles.takeLast();
        QString filePath = QDir(m_logDirectory).absoluteFilePath(oldestFile);
        QFile::remove(filePath);
        info(QString("Removed old log file: %1").arg(oldestFile), "logger");
    }
}

void Logger::writeToFile(const QString& message)
{
    QMutexLocker locker(&m_logMutex);
    
    if (m_logStream) {
        *m_logStream << message << Qt::endl;
        m_logStream->flush();
    }
}

void Logger::writeToConsole(const QString& message)
{
    std::cout << message.toStdString() << std::endl;
}

QString Logger::formatMessage(LogLevel level, const QString& category, const QString& message) const
{
    QString levelStr;
    switch (level) {
        case LogLevel::Debug: levelStr = "DEBUG"; break;
        case LogLevel::Info: levelStr = "INFO"; break;
        case LogLevel::Warning: levelStr = "WARNING"; break;
        case LogLevel::Critical: levelStr = "CRITICAL"; break;
        case LogLevel::Fatal: levelStr = "FATAL"; break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    return QString("[%1] [%2] [%3] %4").arg(timestamp, levelStr, category, message);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    Logger& logger = Logger::instance();
    
    QString category = context.category ? context.category : "qt";
    
    switch (type) {
        case QtDebugMsg:
            logger.debug(message, category);
            break;
        case QtInfoMsg:
            logger.info(message, category);
            break;
        case QtWarningMsg:
            logger.warning(message, category);
            break;
        case QtCriticalMsg:
            logger.critical(message, category);
            break;
        case QtFatalMsg:
            logger.fatal(message, category);
            break;
    }
}
