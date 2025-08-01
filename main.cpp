#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>

#include "backend/core/logger.h"
#include "backend/core/constants.h"
#include "backend/robotstate.h"

#ifdef QHOTKEY_AVAILABLE
#include <QHotkey>
#endif

class GlobalShortcutManager : public QObject
{
    Q_OBJECT

public:
    explicit GlobalShortcutManager(QObject* parent = nullptr)
        : QObject(parent)
    {
#ifdef QHOTKEY_AVAILABLE
        setupShortcuts();
#else
        LOG_WARNING("GlobalShortcuts", "QHotkey not available, global shortcuts disabled");
#endif
    }

private slots:
    void onEmergencyStop()
    {
        LOG_INFO("GlobalShortcuts", "Emergency stop triggered");
        emit emergencyStopTriggered();
    }

    void onEnableDisable()
    {
        LOG_INFO("GlobalShortcuts", "Enable/Disable triggered");
        emit enableDisableTriggered();
    }

    void onModeAutonomous()
    {
        LOG_INFO("GlobalShortcuts", "Autonomous mode triggered");
        emit modeChanged(static_cast<int>(Constants::RobotMode::Autonomous));
    }

    void onModeTeleop()
    {
        LOG_INFO("GlobalShortcuts", "Teleop mode triggered");
        emit modeChanged(static_cast<int>(Constants::RobotMode::Teleop));
    }

    void onModeTest()
    {
        LOG_INFO("GlobalShortcuts", "Test mode triggered");
        emit modeChanged(static_cast<int>(Constants::RobotMode::Test));
    }

signals:
    void emergencyStopTriggered();
    void enableDisableTriggered();
    void modeChanged(int mode);

private:
    void setupShortcuts()
    {
#ifdef QHOTKEY_AVAILABLE
        // Emergency stop
        auto* emergencyStopHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::EMERGENCY_STOP), true, this);
        connect(emergencyStopHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onEmergencyStop);

        // Enable/Disable
        auto* enableDisableHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::ENABLE_DISABLE), true, this);
        connect(enableDisableHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onEnableDisable);

        // Mode shortcuts
        auto* autoHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_AUTONOMOUS), true, this);
        connect(autoHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeAutonomous);

        auto* teleopHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_TELEOP), true, this);
        connect(teleopHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeTeleop);

        auto* testHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_TEST), true, this);
        connect(testHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeTest);

        LOG_INFO("GlobalShortcuts", "Global shortcuts initialized successfully");
#endif
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName(Constants::APP_NAME);
    app.setApplicationVersion(Constants::APP_VERSION);
    app.setOrganizationName(Constants::APP_ORGANIZATION);
    app.setOrganizationDomain(Constants::APP_DOMAIN);
    
    // Initialize logger
    Logger* logger = Logger::instance();
    logger->setLogLevel(Logger::LogLevel::Debug);
    logger->setLogToFile(true);
    logger->setLogToConsole(true);
    
    LOG_INFO("Application", QString("Starting %1 v%2").arg(Constants::APP_NAME, Constants::APP_VERSION));
    
    // Create application directories
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(QDir(appDataDir).absoluteFilePath(Constants::Paths::CONFIG_DIR));
    QDir().mkpath(QDir(appDataDir).absoluteFilePath(Constants::Paths::DASHBOARD_DIR));
    QDir().mkpath(QDir(appDataDir).absoluteFilePath(Constants::Paths::CACHE_DIR));
    
    // Initialize robot state
    RobotState robotState;
    
    // Initialize global shortcuts
    GlobalShortcutManager shortcutManager;
    QObject::connect(&shortcutManager, &GlobalShortcutManager::emergencyStopTriggered,
                     &robotState, &RobotState::emergencyStop);
    QObject::connect(&shortcutManager, &GlobalShortcutManager::enableDisableTriggered,
                     &robotState, &RobotState::toggleEnabled);
    
    // Set up QML engine
    QQmlApplicationEngine engine;
    
    // Register types
    qmlRegisterType<RobotState>("YetAnotherDriverStation", 1, 0, "RobotState");
    qmlRegisterUncreatableMetaObject(Constants::staticMetaObject, "YetAnotherDriverStation", 1, 0, "Constants", "Access to enums & constants only");
    
    // Expose objects to QML
    engine.rootContext()->setContextProperty("robotState", &robotState);
    engine.rootContext()->setContextProperty("logger", logger);
    engine.rootContext()->setContextProperty("shortcutManager", &shortcutManager);
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            LOG_CRITICAL("Application", "Failed to load main QML file");
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        LOG_CRITICAL("Application", "No root objects found in QML engine");
        return -1;
    }
    
    LOG_INFO("Application", "Application started successfully");
    
    int result = app.exec();
    
    LOG_INFO("Application", QString("Application exiting with code %1").arg(result));
    return result;
}

#include "main.moc"
