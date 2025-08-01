#include "robotstate.h"
#include "comms/communicationhandler.h"
#include "controllers/controllerhidhandler.h"
#include "managers/battery_manager.h"
#include "managers/practice_match_manager.h"
#include "managers/network_manager.h"

#ifdef ENABLE_FMS_SUPPORT
#include "fms/fmshandler.h"
#endif

#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QHostAddress>
#include <QProcess>

RobotState::RobotState(QObject *parent)
    : QObject(parent)
    , m_robotEnabled(false)
    , m_emergencyStop(false)
    , m_fmsConnected(false)
    , m_teamNumber(0)
    , m_robotIpAddress("")
    , m_connectionMode(TeamNumber)
    , m_robotMode(Disabled)
    , m_connectionState(Disconnected)
    , m_batteryVoltage(0.0)
    , m_pingLatency(-1)
    , m_globalShortcutsEnabled(true)
    , m_commsStatus("Disconnected")
    , m_robotCodeStatus("No Robot Code")
    , m_joystickStatus("No Controllers")
    , m_robotVoltage(0.0)
    , m_networkLatency(0.0)
    , m_packetLoss(0.0)
    , m_consoleOutput("")
    , m_communicationHandler(nullptr)
    , m_controllerHandler(nullptr)
    , m_batteryManager(nullptr)
    , m_practiceMatchManager(nullptr)
    , m_networkManager(nullptr)
#ifdef ENABLE_FMS_SUPPORT
    , m_fmsHandler(nullptr)
#endif
#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    , m_emergencyStopHotkey(nullptr)
    , m_disableRobotHotkey(nullptr)
    , m_enableRobotHotkey(nullptr)
#endif
    , m_statusUpdateTimer(nullptr)
    , m_connectionTimeoutTimer(nullptr)
{
    Logger::instance().log(Logger::Info, "RobotState", "Initializing robot state manager");
    
    // Load settings first
    loadSettings();
    
    // Initialize components
    initializeComponents();
    
    // Setup connections between components
    setupConnections();
    
    // Setup timers
    setupTimers();
    
#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    // Setup global shortcuts if enabled
    if (m_globalShortcutsEnabled) {
        setupGlobalShortcuts();
    }
#endif
    
    Logger::instance().log(Logger::Info, "RobotState", 
                          QString("Robot state manager initialized (Team: %1, IP: %2, Mode: %3, Global shortcuts: %4)")
                          .arg(m_teamNumber)
                          .arg(m_robotIpAddress)
                          .arg(m_connectionMode == TeamNumber ? "Team Number" : "IP Address")
                          .arg(m_globalShortcutsEnabled ? "enabled" : "disabled"));
}

RobotState::~RobotState()
{
    Logger::instance().log(Logger::Info, "RobotState", "Shutting down robot state manager");
    
    // Save current settings
    saveSettings();
    
#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    // Cleanup global shortcuts
    cleanupGlobalShortcuts();
#endif
    
    // Stop timers
    if (m_statusUpdateTimer) {
        m_statusUpdateTimer->stop();
    }
    if (m_connectionTimeoutTimer) {
        m_connectionTimeoutTimer->stop();
    }
    
    // Cleanup components (they will be deleted by Qt's parent-child system)
    Logger::instance().log(Logger::Info, "RobotState", "Robot state manager shutdown complete");
}

void RobotState::initializeComponents()
{
    // Initialize communication handler
    m_communicationHandler = new CommunicationHandler(this);
    
    // Initialize controller handler
    m_controllerHandler = new ControllerHIDHandler(this);
    
    // Initialize battery manager
    m_batteryManager = new BatteryManager(this);
    
    // Initialize practice match manager
    m_practiceMatchManager = new PracticeMatchManager(this);
    
    // Initialize network manager
    m_networkManager = new NetworkManager(this);
    
#ifdef ENABLE_FMS_SUPPORT
    // Initialize FMS handler if enabled
    m_fmsHandler = new FMSHandler(this);
#endif
    
    Logger::instance().log(Logger::Info, "RobotState", "All components initialized");
}

void RobotState::setupConnections()
{
    // Communication handler connections
    if (m_communicationHandler) {
        connect(m_communicationHandler, &CommunicationHandler::robotConnected,
                this, &RobotState::onRobotConnected);
        connect(m_communicationHandler, &CommunicationHandler::robotDisconnected,
                this, &RobotState::onRobotDisconnected);
        connect(m_communicationHandler, &CommunicationHandler::robotEnabled,
                this, &RobotState::onRobotEnabled);
        connect(m_communicationHandler, &CommunicationHandler::robotDisabled,
                this, &RobotState::onRobotDisabled);
        connect(m_communicationHandler, &CommunicationHandler::communicationError,
                this, &RobotState::onCommunicationError);
        connect(m_communicationHandler, &CommunicationHandler::pingLatencyChanged,
                this, &RobotState::onPingLatencyChanged);
    }
    
    // Battery manager connections
    if (m_batteryManager) {
        connect(m_batteryManager, &BatteryManager::batteryVoltageChanged,
                this, &RobotState::onBatteryVoltageChanged);
    }
    
#ifdef ENABLE_FMS_SUPPORT
    // FMS handler connections
    if (m_fmsHandler) {
        connect(m_fmsHandler, &FMSHandler::fmsConnected,
                this, &RobotState::onFMSConnected);
        connect(m_fmsHandler, &FMSHandler::fmsDisconnected,
                this, &RobotState::onFMSDisconnected);
        connect(m_fmsHandler, &FMSHandler::fmsModeChanged,
                this, &RobotState::onFMSModeChanged);
    }
#endif
    
    Logger::instance().log(Logger::Info, "RobotState", "Component connections established");
}

void RobotState::setupTimers()
{
    // Status update timer (100ms intervals)
    m_statusUpdateTimer = new QTimer(this);
    m_statusUpdateTimer->setInterval(100);
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &RobotState::robotStatusChanged);
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &RobotState::updateStatusStrings);
    m_statusUpdateTimer->start();
    
    // Connection timeout timer (5 second timeout)
    m_connectionTimeoutTimer = new QTimer(this);
    m_connectionTimeoutTimer->setInterval(5000);
    m_connectionTimeoutTimer->setSingleShot(true);
    
    Logger::instance().log(Logger::Info, "RobotState", "Timers configured");
}

#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
void RobotState::setupGlobalShortcuts()
{
    Logger::instance().log(Logger::Info, "RobotState", "Setting up global shortcuts");
    
    try {
        // Emergency stop shortcut (Space)
        registerGlobalShortcut(m_emergencyStopHotkey, QKeySequence(Qt::Key_Space),
                              SLOT(onEmergencyStopShortcut()), "Emergency Stop");
        
        // Disable robot shortcut (Enter)
        registerGlobalShortcut(m_disableRobotHotkey, QKeySequence(Qt::Key_Return),
                              SLOT(onDisableRobotShortcut()), "Disable Robot");
        
        // Enable robot shortcut (Ctrl+E)
        registerGlobalShortcut(m_enableRobotHotkey, QKeySequence(Qt::CTRL | Qt::Key_E),
                              SLOT(onEnableRobotShortcut()), "Enable Robot");
        
        Logger::instance().log(Logger::Info, "RobotState", "Global shortcuts registered successfully");
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "RobotState", 
                              QString("Failed to setup global shortcuts: %1").arg(e.what()));
    }
}

void RobotState::cleanupGlobalShortcuts()
{
    Logger::instance().log(Logger::Info, "RobotState", "Cleaning up global shortcuts");
    
    if (m_emergencyStopHotkey) {
        m_emergencyStopHotkey->setRegistered(false);
        m_emergencyStopHotkey->deleteLater();
        m_emergencyStopHotkey = nullptr;
    }
    
    if (m_disableRobotHotkey) {
        m_disableRobotHotkey->setRegistered(false);
        m_disableRobotHotkey->deleteLater();
        m_disableRobotHotkey = nullptr;
    }
    
    if (m_enableRobotHotkey) {
        m_enableRobotHotkey->setRegistered(false);
        m_enableRobotHotkey->deleteLater();
        m_enableRobotHotkey = nullptr;
    }
    
    Logger::instance().log(Logger::Info, "RobotState", "Global shortcuts cleanup complete");
}

void RobotState::registerGlobalShortcut(QHotkey*& hotkey, const QKeySequence& sequence, 
                                       const char* slot, const QString& description)
{
    hotkey = new QHotkey(sequence, true, this);
    
    if (hotkey->isRegistered()) {
        connect(hotkey, SIGNAL(activated()), this, slot);
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Global shortcut registered: %1 (%2)")
                              .arg(description)
                              .arg(sequence.toString()));
    } else {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              QString("Failed to register global shortcut: %1 (%2)")
                              .arg(description)
                              .arg(sequence.toString()));
        hotkey->deleteLater();
        hotkey = nullptr;
    }
}

void RobotState::onEmergencyStopShortcut()
{
    Logger::instance().log(Logger::Warning, "RobotState", "Emergency stop triggered via global shortcut");
    emit globalShortcutTriggered("Emergency Stop");
    emergencyStopRobot();
}

void RobotState::onDisableRobotShortcut()
{
    Logger::instance().log(Logger::Info, "RobotState", "Robot disable triggered via global shortcut");
    emit globalShortcutTriggered("Disable Robot");
    disableRobot();
}

void RobotState::onEnableRobotShortcut()
{
    if (!m_emergencyStop) {
        Logger::instance().log(Logger::Info, "RobotState", "Robot enable triggered via global shortcut");
        emit globalShortcutTriggered("Enable Robot");
        enableRobot();
    } else {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              "Robot enable shortcut ignored - emergency stop active");
    }
}
#endif

QString RobotState::robotMode() const
{
    switch (m_robotMode) {
        case Disabled: return "Disabled";
        case Autonomous: return "Autonomous";
        case Teleop: return "Teleop";
        case Test: return "Test";
        default: return "Unknown";
    }
}

void RobotState::setTeamNumber(int teamNumber)
{
    if (m_teamNumber != teamNumber) {
        QMutexLocker locker(&m_stateMutex);
        m_teamNumber = teamNumber;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Team number changed to: %1").arg(teamNumber));
        
        // Update communication handler with new team number
        if (m_communicationHandler) {
            m_communicationHandler->setTeamNumber(teamNumber);
        }
        
        emit teamNumberChanged(teamNumber);
        saveSettings();
    }
}

void RobotState::setRobotIpAddress(const QString& ipAddress)
{
    if (m_robotIpAddress != ipAddress) {
        QMutexLocker locker(&m_stateMutex);
        m_robotIpAddress = ipAddress;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Robot IP address changed to: %1").arg(ipAddress));
        
        // Update communication handler with new IP address
        if (m_communicationHandler) {
            m_communicationHandler->setRobotIpAddress(ipAddress);
        }
        
        emit robotIpAddressChanged(ipAddress);
        saveSettings();
    }
}

void RobotState::setConnectionMode(int mode)
{
    ConnectionMode newMode = static_cast<ConnectionMode>(mode);
    if (m_connectionMode != newMode) {
        QMutexLocker locker(&m_stateMutex);
        m_connectionMode = newMode;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Connection mode changed to: %1")
                              .arg(newMode == TeamNumber ? "Team Number" : "IP Address"));
        
        // Update communication handler with new connection mode
        if (m_communicationHandler) {
            m_communicationHandler->setConnectionMode(newMode == TeamNumber ? 
                CommunicationHandler::TeamNumber : CommunicationHandler::IpAddress);
        }
        
        emit connectionModeChanged(mode);
        saveSettings();
    }
}

void RobotState::setGlobalShortcutsEnabled(bool enabled)
{
    if (m_globalShortcutsEnabled != enabled) {
        m_globalShortcutsEnabled = enabled;
        
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Global shortcuts %1").arg(enabled ? "enabled" : "disabled"));
        
#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
        if (enabled) {
            setupGlobalShortcuts();
        } else {
            cleanupGlobalShortcuts();
        }
#endif
        
        emit globalShortcutsEnabledChanged(enabled);
        saveSettings();
    }
}

void RobotState::enableRobot()
{
    if (m_emergencyStop) {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              "Cannot enable robot - emergency stop active");
        return;
    }
    
    if (!isRobotConnected()) {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              "Cannot enable robot - not connected");
        return;
    }
    
    Logger::instance().log(Logger::Info, "RobotState", "Enabling robot");
    
    if (m_communicationHandler) {
        m_communicationHandler->enableRobot();
    }
    
    logStateChange("Robot enabled");
}

void RobotState::disableRobot()
{
    Logger::instance().log(Logger::Info, "RobotState", "Disabling robot");
    
    if (m_communicationHandler) {
        m_communicationHandler->disableRobot();
    }
    
    logStateChange("Robot disabled");
}

void RobotState::emergencyStopRobot()
{
    QMutexLocker locker(&m_stateMutex);
    m_emergencyStop = true;
    m_lastEmergencyStopTime = QDateTime::currentDateTime();
    locker.unlock();
    
    Logger::instance().log(Logger::Error, "RobotState", "EMERGENCY STOP ACTIVATED");
    
    if (m_communicationHandler) {
        m_communicationHandler->emergencyStop();
    }
    
    emit emergencyStopChanged(true);
    emit emergencyStopTriggered("Manual");
    logStateChange("Emergency stop activated");
}

void RobotState::clearEmergencyStop()
{
    if (m_emergencyStop) {
        QMutexLocker locker(&m_stateMutex);
        m_emergencyStop = false;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "RobotState", "Emergency stop cleared");
        
        emit emergencyStopChanged(false);
        logStateChange("Emergency stop cleared");
    }
}

void RobotState::connectToRobot()
{
    if (m_connectionMode == TeamNumber && m_teamNumber == 0) {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              "Cannot connect - team number not set");
        return;
    }
    
    if (m_connectionMode == IpAddress && m_robotIpAddress.isEmpty()) {
        Logger::instance().log(Logger::Warning, "RobotState", 
                              "Cannot connect - IP address not set");
        return;
    }
    
    QString target = (m_connectionMode == TeamNumber) ? 
        QString("Team %1").arg(m_teamNumber) : m_robotIpAddress;
    
    Logger::instance().log(Logger::Info, "RobotState", 
                          QString("Connecting to robot (%1)").arg(target));
    
    updateConnectionState(Connecting);
    
    if (m_communicationHandler) {
        m_communicationHandler->connectToRobot();
    }
    
    // Start connection timeout
    if (m_connectionTimeoutTimer) {
        m_connectionTimeoutTimer->start();
    }
}

void RobotState::disconnectFromRobot()
{
    Logger::instance().log(Logger::Info, "RobotState", "Disconnecting from robot");
    
    if (m_communicationHandler) {
        m_communicationHandler->disconnectFromRobot();
    }
    
    updateConnectionState(Disconnected);
}

void RobotState::restartCommunication()
{
    Logger::instance().log(Logger::Info, "RobotState", "Restarting communication");
    
    disconnectFromRobot();
    
    // Wait a moment before reconnecting
    QTimer::singleShot(1000, this, &RobotState::connectToRobot);
}

void RobotState::updateStatusStrings()
{
    // Update communications status
    QString newCommsStatus;
    if (isRobotConnected()) {
        newCommsStatus = "Connected";
    } else if (m_connectionState == Connecting) {
        newCommsStatus = "Connecting...";
    } else {
        newCommsStatus = "Disconnected";
    }
    
    if (m_commsStatus != newCommsStatus) {
        m_commsStatus = newCommsStatus;
        emit commsStatusChanged(m_commsStatus);
    }
    
    // Update robot code status
    QString newRobotCodeStatus = isRobotConnected() ? "Robot Code" : "No Robot Code";
    if (m_robotCodeStatus != newRobotCodeStatus) {
        m_robotCodeStatus = newRobotCodeStatus;
        emit robotCodeStatusChanged(m_robotCodeStatus);
    }
    
    // Update joystick status
    QString newJoystickStatus = "No Controllers"; // This would be updated by controller handler
    if (m_joystickStatus != newJoystickStatus) {
        m_joystickStatus = newJoystickStatus;
        emit joystickStatusChanged(m_joystickStatus);
    }
    
    // Update voltage (same as battery voltage for now)
    if (m_robotVoltage != m_batteryVoltage) {
        m_robotVoltage = m_batteryVoltage;
        emit robotVoltageChanged(m_robotVoltage);
    }
    
    // Update network latency
    double newLatency = static_cast<double>(m_pingLatency);
    if (m_networkLatency != newLatency) {
        m_networkLatency = newLatency;
        emit networkLatencyChanged(m_networkLatency);
    }
    
    // Update packet loss (would be calculated from communication handler)
    double newPacketLoss = 0.0; // Placeholder
    if (m_packetLoss != newPacketLoss) {
        m_packetLoss = newPacketLoss;
        emit packetLossChanged(m_packetLoss);
    }
}

void RobotState::onRobotConnected()
{
    QMutexLocker locker(&m_stateMutex);
    m_connectionStartTime = QDateTime::currentDateTime();
    locker.unlock();
    
    updateConnectionState(Connected);
    
    Logger::instance().log(Logger::Info, "RobotState", "Robot connected successfully");
    
    // Stop connection timeout timer
    if (m_connectionTimeoutTimer) {
        m_connectionTimeoutTimer->stop();
    }
    
    emit robotConnectedChanged(true);
}

void RobotState::onRobotDisconnected()
{
    updateConnectionState(Disconnected);
    
    Logger::instance().log(Logger::Warning, "RobotState", "Robot disconnected");
    
    // Clear robot enabled state
    if (m_robotEnabled) {
        QMutexLocker locker(&m_stateMutex);
        m_robotEnabled = false;
        locker.unlock();
        emit robotEnabledChanged(false);
        emit enabledChanged(false);
    }
    
    emit robotConnectedChanged(false);
}

void RobotState::onRobotEnabled()
{
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = true;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "RobotState", "Robot enabled");
    emit robotEnabledChanged(true);
    emit enabledChanged(true);
}

void RobotState::onRobotDisabled()
{
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = false;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "RobotState", "Robot disabled");
    emit robotEnabledChanged(false);
    emit enabledChanged(false);
}

void RobotState::onBatteryVoltageChanged(double voltage)
{
    QMutexLocker locker(&m_stateMutex);
    m_batteryVoltage = voltage;
    locker.unlock();
    
    emit batteryVoltageChanged(voltage);
}

void RobotState::onPingLatencyChanged(int latency)
{
    QMutexLocker locker(&m_stateMutex);
    m_pingLatency = latency;
    locker.unlock();
    
    emit pingLatencyChanged(latency);
}

void RobotState::onCommunicationError(const QString& error)
{
    Logger::instance().log(Logger::Error, "RobotState", 
                          QString("Communication error: %1").arg(error));
    
    emit communicationError(error);
    
    // If we were connected, update state to connection lost
    if (m_connectionState == Connected) {
        updateConnectionState(ConnectionLost);
    }
}

#ifdef ENABLE_FMS_SUPPORT
void RobotState::onFMSConnected()
{
    QMutexLocker locker(&m_stateMutex);
    m_fmsConnected = true;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "RobotState", "FMS connected");
    emit fmsConnectedChanged(true);
}

void RobotState::onFMSDisconnected()
{
    QMutexLocker locker(&m_stateMutex);
    m_fmsConnected = false;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "RobotState", "FMS disconnected");
    emit fmsConnectedChanged(false);
}

void RobotState::onFMSModeChanged(int mode)
{
    QMutexLocker locker(&m_stateMutex);
    m_robotMode = static_cast<RobotMode>(mode);
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "RobotState", 
                          QString("Robot mode changed to: %1").arg(robotMode()));
    emit robotModeChanged(robotMode());
}
#endif

void RobotState::updateConnectionState(ConnectionState newState)
{
    if (m_connectionState != newState) {
        QMutexLocker locker(&m_stateMutex);
        ConnectionState oldState = m_connectionState;
        m_connectionState = newState;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "RobotState", 
                              QString("Connection state changed: %1 -> %2")
                              .arg(static_cast<int>(oldState))
                              .arg(static_cast<int>(newState)));
        
        emit connectionStateChanged(newState);
    }
}

void RobotState::logStateChange(const QString& change)
{
    QString target = (m_connectionMode == TeamNumber) ? 
        QString("Team: %1").arg(m_teamNumber) : QString("IP: %1").arg(m_robotIpAddress);
    
    Logger::instance().log(Logger::Info, "RobotState", 
                          QString("State change: %1 (%2, Connected: %3, Enabled: %4, E-Stop: %5)")
                          .arg(change)
                          .arg(target)
                          .arg(isRobotConnected() ? "Yes" : "No")
                          .arg(m_robotEnabled ? "Yes" : "No")
                          .arg(m_emergencyStop ? "Yes" : "No"));
}

void RobotState::saveSettings()
{
    QSettings settings;
    settings.beginGroup("RobotState");
    settings.setValue("teamNumber", m_teamNumber);
    settings.setValue("robotIpAddress", m_robotIpAddress);
    settings.setValue("connectionMode", static_cast<int>(m_connectionMode));
    settings.setValue("globalShortcutsEnabled", m_globalShortcutsEnabled);
    settings.endGroup();
}

void RobotState::loadSettings()
{
    QSettings settings;
    settings.beginGroup("RobotState");
    m_teamNumber = settings.value("teamNumber", 0).toInt();
    m_robotIpAddress = settings.value("robotIpAddress", "").toString();
    m_connectionMode = static_cast<ConnectionMode>(settings.value("connectionMode", 0).toInt());
    m_globalShortcutsEnabled = settings.value("globalShortcutsEnabled", true).toBool();
    settings.endGroup();
}

void RobotState::restartApplication()
{
    Logger::instance().log(Logger::Info, "RobotState", "Restarting application");
    
    // Save current state
    saveSettings();
    
    // Restart the application
    QApplication::quit();
    QProcess::startDetached(QApplication::applicationFilePath(), QApplication::arguments());
}

void RobotState::shutdownApplication()
{
    Logger::instance().log(Logger::Info, "RobotState", "Shutting down application");
    
    // Save current state
    saveSettings();
    
    // Quit the application
    QApplication::quit();
}
