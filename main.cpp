#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>

#include "backend/core/logger.h"
#include "backend/core/constants.h"
#include "backend/robotstate.h"
#include "backend/managers/battery_manager.h"
#include "backend/managers/practice_match_manager.h"
#include "backend/managers/network_manager.h"
#include "backend/controllers/controllerhidhandler.h"

#ifdef QHOTKEY_AVAILABLE
#include <QHotkey>
#endif

Q_LOGGING_CATEGORY(main, "main")

class GlobalShortcutManager : public QObject
{
    Q_OBJECT
    
public:
    explicit GlobalShortcutManager(QObject* parent = nullptr) : QObject(parent)
    {
#ifdef QHOTKEY_AVAILABLE
        setupGlobalShortcuts();
#else
        LOG_WARNING("Global shortcuts not available - QHotkey not compiled");
#endif
    }
    
private slots:
    void onEmergencyStop()
    {
        LOG_INFO("Emergency stop triggered via global shortcut");
        emit emergencyStopRequested();
    }
    
    void onToggleEnable()
    {
        LOG_INFO("Toggle enable triggered via global shortcut");
        emit toggleEnableRequested();
    }
    
    void onModeAutonomous()
    {
        LOG_INFO("Autonomous mode triggered via global shortcut");
        emit modeChangeRequested(Constants::RobotMode::Autonomous);
    }
    
    void onModeTeleop()
    {
        LOG_INFO("Teleop mode triggered via global shortcut");
        emit modeChangeRequested(Constants::RobotMode::Teleop);
    }
    
    void onModeTest()
    {
        LOG_INFO("Test mode triggered via global shortcut");
        emit modeChangeRequested(Constants::RobotMode::Test);
    }
    
signals:
    void emergencyStopRequested();
    void toggleEnableRequested();
    void modeChangeRequested(Constants::RobotMode mode);
    
private:
    void setupGlobalShortcuts()
    {
#ifdef QHOTKEY_AVAILABLE
        // Emergency stop shortcuts
        auto* emergencyStopHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::EMERGENCY_STOP), true, this);
        auto* emergencyStopAltHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::EMERGENCY_STOP_ALT), true, this);
        
        // Toggle enable shortcut
        auto* toggleEnableHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::TOGGLE_ENABLE), true, this);
        
        // Mode shortcuts
        auto* modeAutoHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_AUTONOMOUS), true, this);
        auto* modeTeleopHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_TELEOP), true, this);
        auto* modeTestHotkey = new QHotkey(QKeySequence(Constants::Shortcuts::MODE_TEST), true, this);
        
        // Connect signals
        connect(emergencyStopHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onEmergencyStop);
        connect(emergencyStopAltHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onEmergencyStop);
        connect(toggleEnableHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onToggleEnable);
        connect(modeAutoHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeAutonomous);
        connect(modeTeleopHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeTeleop);
        connect(modeTestHotkey, &QHotkey::activated, this, &GlobalShortcutManager::onModeTest);
        
        // Verify registration
        if (emergencyStopHotkey->isRegistered()) {
            LOG_INFO("Emergency stop global shortcut registered: %1", Constants::Shortcuts::EMERGENCY_STOP);
        } else {
            LOG_WARNING("Failed to register emergency stop shortcut: %1", Constants::Shortcuts::EMERGENCY_STOP);
        }
        
        if (toggleEnableHotkey->isRegistered()) {
            LOG_INFO("Toggle enable global shortcut registered: %1", Constants::Shortcuts::TOGGLE_ENABLE);
        } else {
            LOG_WARNING("Failed to register toggle enable shortcut: %1", Constants::Shortcuts::TOGGLE_ENABLE);
        }
        
        LOG_INFO("Global shortcuts initialized");
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
    logger->setLogLevel(Constants::LogLevel::Info);
    logger->setFileLogging(true);
    logger->setConsoleLogging(true);
    
    LOG_INFO("=== %1 v%2 Starting ===", Constants::APP_NAME, Constants::APP_VERSION);
    LOG_INFO("Qt Version: %1", QT_VERSION_STR);
    LOG_INFO("Build Date: %1 %2", __DATE__, __TIME__);
    
#ifdef QHOTKEY_AVAILABLE
    LOG_INFO("QHotkey support: Available");
#else
    LOG_INFO("QHotkey support: Not available");
#endif
    
    // Create core managers
    RobotState* robotState = new RobotState(&app);
    BatteryManager* batteryManager = new BatteryManager(&app);
    PracticeMatchManager* practiceMatchManager = new PracticeMatchManager(&app);
    NetworkManager* networkManager = new NetworkManager(&app);
    ControllerHIDHandler* controllerHandler = new ControllerHIDHandler(&app);
    
    // Create global shortcut manager
    GlobalShortcutManager* shortcutManager = new GlobalShortcutManager(&app);
    
    // Connect global shortcuts to robot state
    QObject::connect(shortcutManager, &GlobalShortcutManager::emergencyStopRequested,
                     robotState, &RobotState::emergencyStop);
    QObject::connect(shortcutManager, &GlobalShortcutManager::toggleEnableRequested,
                     robotState, &RobotState::toggleEnabled);
    QObject::connect(shortcutManager, QOverload<Constants::RobotMode>::of(&GlobalShortcutManager::modeChangeRequested),
                     robotState, &RobotState::setMode);
    
    // Connect managers
    QObject::connect(robotState, &RobotState::batteryVoltageChanged,
                     batteryManager, &BatteryManager::updateBatteryVoltage);
    QObject::connect(practiceMatchManager, &PracticeMatchManager::modeChangeRequested,
                     robotState, &RobotState::setMode);
    QObject::connect(practiceMatchManager, &PracticeMatchManager::enableRequested,
                     robotState, &RobotState::setEnabled);
    
    // Setup QML engine
    QQmlApplicationEngine engine;
    
    // Register types with QML
    qmlRegisterUncreatableType<Constants>("YetAnotherDriverStation", 1, 0, "Constants", "Constants is a namespace");
    qmlRegisterType<RobotState>("YetAnotherDriverStation", 1, 0, "RobotState");
    qmlRegisterType<BatteryManager>("YetAnotherDriverStation", 1, 0, "BatteryManager");
    qmlRegisterType<PracticeMatchManager>("YetAnotherDriverStation", 1, 0, "PracticeMatchManager");
    qmlRegisterType<NetworkManager>("YetAnotherDriverStation", 1, 0, "NetworkManager");
    qmlRegisterType<ControllerHIDHandler>("YetAnotherDriverStation", 1, 0, "ControllerHIDHandler");
    
    // Expose instances to QML
    engine.rootContext()->setContextProperty("robotState", robotState);
    engine.rootContext()->setContextProperty("batteryManager", batteryManager);
    engine.rootContext()->setContextProperty("practiceMatchManager", practiceMatchManager);
    engine.rootContext()->setContextProperty("networkManager", networkManager);
    engine.rootContext()->setContextProperty("controllerHandler", controllerHandler);
    engine.rootContext()->setContextProperty("logger", logger);
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            LOG_CRITICAL("Failed to load main QML file");
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    if (engine.rootObjects().isEmpty()) {
        LOG_CRITICAL("No root objects found in QML engine");
        return -1;
    }
    
    LOG_INFO("Application initialized successfully");
    LOG_INFO("Main window loaded, starting event loop");
    
    // Start the application
    int result = app.exec();
    
    LOG_INFO("Application shutting down with exit code: %1", result);
    LOG_INFO("=== %1 Shutdown Complete ===", Constants::APP_NAME);
    
    return result;
}

#include "main.moc"
