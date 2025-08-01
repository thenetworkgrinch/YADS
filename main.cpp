#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <memory>

#include "backend/robotstate.h"
#include "backend/robot/comms/communicationhandler.h"
#include "backend/controllers/controllerhidhandler.h"
#include "backend/core/logger.h"
#include "backend/core/constants.h"

using namespace FRCDriverStation;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName(Constants::Application::APPLICATION_NAME);
    app.setApplicationVersion(Constants::Application::APPLICATION_VERSION);
    app.setOrganizationName(Constants::Application::ORGANIZATION_NAME);
    app.setOrganizationDomain(Constants::Application::APPLICATION_DOMAIN);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/app-icon.png"));
    
    // Configure Qt logging
    QLoggingCategory::setFilterRules("qt.network.ssl.debug=false");
    
    // Create data directory
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    
    // Initialize logger
    auto logger = std::make_shared<Logger>();
    if (!logger->initialize(QDir(dataDir).absoluteFilePath("logs"))) {
        qCritical() << "Failed to initialize logger";
        return 1;
    }
    
    logger->info("Application", "FRC Driver Station starting", 
                QString("Version: %1").arg(Constants::Application::APPLICATION_VERSION));
    
    // Create core components
    auto robotState = std::make_unique<RobotState>(logger);
    auto controllerHandler = std::make_unique<ControllerHIDHandler>(logger);
    auto communicationHandler = std::make_unique<CommunicationHandler>(
        robotState.get(), controllerHandler.get(), logger);
    
    // Start controller polling
    controllerHandler->startPolling();
    
    // Create QML engine
    QQmlApplicationEngine engine;
    
    // Register types for QML
    qmlRegisterType<RobotState>("FRCDriverStation", 1, 0, "RobotState");
    qmlRegisterType<ControllerHIDHandler>("FRCDriverStation", 1, 0, "ControllerHIDHandler");
    qmlRegisterType<BatteryManager>("FRCDriverStation", 1, 0, "BatteryManager");
    qmlRegisterType<PracticeMatchManager>("FRCDriverStation", 1, 0, "PracticeMatchManager");
    qmlRegisterType<NetworkManager>("FRCDriverStation", 1, 0, "NetworkManager");
    
#ifdef ENABLE_FMS_SUPPORT
    qmlRegisterType<FMSHandler>("FRCDriverStation", 1, 0, "FMSHandler");
#endif
    
    // Expose objects to QML
    engine.rootContext()->setContextProperty("robotState", robotState.get());
    engine.rootContext()->setContextProperty("controllerHandler", controllerHandler.get());
    engine.rootContext()->setContextProperty("batteryManager", robotState->batteryManager());
    engine.rootContext()->setContextProperty("practiceMatchManager", robotState->practiceMatchManager());
    engine.rootContext()->setContextProperty("networkManager", robotState->networkManager());
    
#ifdef ENABLE_FMS_SUPPORT
    engine.rootContext()->setContextProperty("fmsHandler", robotState->fmsHandler());
#endif
    
    // Set feature flags for QML
    engine.rootContext()->setContextProperty("enableFMSSupport", 
#ifdef ENABLE_FMS_SUPPORT
        true
#else
        false
#endif
    );
    
    engine.rootContext()->setContextProperty("enableGlassIntegration",
#ifdef ENABLE_GLASS_INTEGRATION
        true
#else
        false
#endif
    );
    
    engine.rootContext()->setContextProperty("enableDashboardManagement",
#ifdef ENABLE_DASHBOARD_MANAGEMENT
        true
#else
        false
#endif
    );
    
    engine.rootContext()->setContextProperty("enablePracticeMatch",
#ifdef ENABLE_PRACTICE_MATCH
        true
#else
        false
#endif
    );
    
    // Load main QML file
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    logger->info("Application", "FRC Driver Station started successfully");
    
    // Run application
    int result = app.exec();
    
    logger->info("Application", "FRC Driver Station shutting down");
    
    // Cleanup
    controllerHandler->stopPolling();
    
    return result;
}
