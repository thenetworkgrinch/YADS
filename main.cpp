#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>

#include "backend/core/logger.h"
#include "backend/core/constants.h"
#include "backend/managers/application_manager.h"
#include "backend/robotstate.h"

#ifdef ENABLE_GLOBAL_SHORTCUTS
#include <QHotkey>
#endif

Q_LOGGING_CATEGORY(main, "main")

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName(Constants::APPLICATION_NAME);
    app.setApplicationVersion(Constants::APPLICATION_VERSION);
    app.setOrganizationName(Constants::ORGANIZATION_NAME);
    app.setOrganizationDomain(Constants::ORGANIZATION_DOMAIN);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app-icon.png"));
    
    // Initialize logging system
    Logger::instance().initialize();
    qCInfo(main) << "Starting" << Constants::APPLICATION_NAME << "version" << Constants::APPLICATION_VERSION;
    
    // Create application manager
    ApplicationManager appManager;
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Register QML types
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "ApplicationManager", &appManager);
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "RobotState", appManager.robotState());
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "NetworkManager", appManager.networkManager());
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "BatteryManager", appManager.batteryManager());
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "ControllerManager", appManager.controllerManager());
    qmlRegisterSingletonInstance("YetAnotherDriverStation", 1, 0, "PracticeMatchManager", appManager.practiceMatchManager());
    
    // Set up global shortcuts
#ifdef ENABLE_GLOBAL_SHORTCUTS
    QHotkey *toggleEnableShortcut = new QHotkey(QKeySequence("Space"), true, &app);
    QObject::connect(toggleEnableShortcut, &QHotkey::activated, [&appManager]() {
        qCInfo(main) << "Global shortcut: Toggle enable/disable";
        appManager.robotState()->toggleEnabled();
    });
    
    QHotkey *enableCurrentModeShortcut = new QHotkey(QKeySequence("Return"), true, &app);
    QObject::connect(enableCurrentModeShortcut, &QHotkey::activated, [&appManager]() {
        qCInfo(main) << "Global shortcut: Enable/disable current mode";
        if (appManager.robotState()->isEnabled()) {
            appManager.robotState()->setEnabled(false);
        } else {
            appManager.robotState()->setEnabled(true);
        }
    });
    
    if (!toggleEnableShortcut->isRegistered()) {
        qCWarning(main) << "Failed to register Space key global shortcut";
    }
    
    if (!enableCurrentModeShortcut->isRegistered()) {
        qCWarning(main) << "Failed to register Enter key global shortcut";
    }
#endif
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    qCInfo(main) << "Application started successfully";
    
    int result = app.exec();
    
    qCInfo(main) << "Application shutting down with exit code:" << result;
    return result;
}
