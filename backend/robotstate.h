#ifndef ROBOTSTATE_H
#define ROBOTSTATE_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QMutex>
#include <QNetworkInterface>
#include <QHostAddress>

#ifdef ENABLE_GLOBAL_SHORTCUTS
#ifdef QHOTKEY_AVAILABLE
#include <qhotkey.h>
#include <QPointer>
#endif
#endif

#include "core/logger.h"
#include "core/constants.h"

class CommunicationHandler;
class ControllerHIDHandler;
class BatteryManager;
class PracticeMatchManager;
class NetworkManager;

#ifdef ENABLE_FMS_SUPPORT
class FMSHandler;
#endif

/**
 * @brief Central robot state management class
 * 
 * This class manages the overall state of the robot connection,
 * handles global shortcuts, and coordinates between different subsystems.
 */
class RobotState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool robotEnabled READ isRobotEnabled NOTIFY robotEnabledChanged)
    Q_PROPERTY(bool robotConnected READ isRobotConnected NOTIFY robotConnectedChanged)
    Q_PROPERTY(bool emergencyStop READ isEmergencyStop NOTIFY emergencyStopChanged)
    Q_PROPERTY(bool fmsConnected READ isFMSConnected NOTIFY fmsConnectedChanged)
    Q_PROPERTY(int teamNumber READ teamNumber WRITE setTeamNumber NOTIFY teamNumberChanged)
    Q_PROPERTY(QString robotIpAddress READ robotIpAddress WRITE setRobotIpAddress NOTIFY robotIpAddressChanged)
    Q_PROPERTY(int connectionMode READ connectionMode WRITE setConnectionMode NOTIFY connectionModeChanged)
    Q_PROPERTY(QString robotMode READ robotMode NOTIFY robotModeChanged)
    Q_PROPERTY(double batteryVoltage READ batteryVoltage NOTIFY batteryVoltageChanged)
    Q_PROPERTY(int pingLatency READ pingLatency NOTIFY pingLatencyChanged)
    Q_PROPERTY(bool globalShortcutsEnabled READ globalShortcutsEnabled WRITE setGlobalShortcutsEnabled NOTIFY globalShortcutsEnabledChanged)
    Q_PROPERTY(QString commsStatus READ commsStatus NOTIFY commsStatusChanged)
    Q_PROPERTY(QString robotCodeStatus READ robotCodeStatus NOTIFY robotCodeStatusChanged)
    Q_PROPERTY(QString joystickStatus READ joystickStatus NOTIFY joystickStatusChanged)
    Q_PROPERTY(double robotVoltage READ robotVoltage NOTIFY robotVoltageChanged)
    Q_PROPERTY(double networkLatency READ networkLatency NOTIFY networkLatencyChanged)
    Q_PROPERTY(double packetLoss READ packetLoss NOTIFY packetLossChanged)
    Q_PROPERTY(QString consoleOutput READ consoleOutput NOTIFY consoleOutputChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

public:
    enum RobotMode {
        Disabled = 0,
        Autonomous = 1,
        Teleop = 2,
        Test = 3
    };
    Q_ENUM(RobotMode)

    enum ConnectionState {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        ConnectionLost = 3
    };
    Q_ENUM(ConnectionState)

    enum ConnectionMode {
        TeamNumber = 0,
        IpAddress = 1
    };
    Q_ENUM(ConnectionMode)

    explicit RobotState(QObject *parent = nullptr);
    ~RobotState();

    // Getters
    bool isRobotEnabled() const { return m_robotEnabled; }
    bool isRobotConnected() const { return m_connectionState == Connected; }
    bool isEmergencyStop() const { return m_emergencyStop; }
    bool isFMSConnected() const { return m_fmsConnected; }
    int teamNumber() const { return m_teamNumber; }
    QString robotIpAddress() const { return m_robotIpAddress; }
    int connectionMode() const { return static_cast<int>(m_connectionMode); }
    QString robotMode() const;
    double batteryVoltage() const { return m_batteryVoltage; }
    int pingLatency() const { return m_pingLatency; }
    ConnectionState connectionState() const { return m_connectionState; }
    bool globalShortcutsEnabled() const { return m_globalShortcutsEnabled; }
    QString commsStatus() const { return m_commsStatus; }
    QString robotCodeStatus() const { return m_robotCodeStatus; }
    QString joystickStatus() const { return m_joystickStatus; }
    double robotVoltage() const { return m_robotVoltage; }
    double networkLatency() const { return m_networkLatency; }
    double packetLoss() const { return m_packetLoss; }
    QString consoleOutput() const { return m_consoleOutput; }
    bool enabled() const { return m_robotEnabled; }

    // Setters
    void setTeamNumber(int teamNumber);
    void setRobotIpAddress(const QString& ipAddress);
    void setConnectionMode(int mode);
    void setGlobalShortcutsEnabled(bool enabled);

    // Component getters
    CommunicationHandler* communicationHandler() const { return m_communicationHandler; }
    ControllerHIDHandler* controllerHandler() const { return m_controllerHandler; }
    BatteryManager* batteryManager() const { return m_batteryManager; }
    PracticeMatchManager* practiceMatchManager() const { return m_practiceMatchManager; }
    NetworkManager* networkManager() const { return m_networkManager; }

#ifdef ENABLE_FMS_SUPPORT
    FMSHandler* fmsHandler() const { return m_fmsHandler; }
#endif

public slots:
    // Robot control
    void enableRobot();
    void disableRobot();
    void emergencyStopRobot();
    void clearEmergencyStop();
    
    // Connection management
    void connectToRobot();
    void disconnectFromRobot();
    void restartCommunication();

    // System control
    void restartApplication();
    void shutdownApplication();

signals:
    // State change signals
    void robotEnabledChanged(bool enabled);
    void robotConnectedChanged(bool connected);
    void emergencyStopChanged(bool emergencyStop);
    void fmsConnectedChanged(bool connected);
    void teamNumberChanged(int teamNumber);
    void robotIpAddressChanged(const QString& ipAddress);
    void connectionModeChanged(int mode);
    void robotModeChanged(const QString& mode);
    void batteryVoltageChanged(double voltage);
    void pingLatencyChanged(int latency);
    void connectionStateChanged(ConnectionState state);
    void globalShortcutsEnabledChanged(bool enabled);
    void commsStatusChanged(const QString& status);
    void robotCodeStatusChanged(const QString& status);
    void joystickStatusChanged(const QString& status);
    void robotVoltageChanged(double voltage);
    void networkLatencyChanged(double latency);
    void packetLossChanged(double loss);
    void consoleOutputChanged(const QString& output);
    void enabledChanged(bool enabled);

    // Event signals
    void robotStatusChanged();
    void communicationError(const QString& error);
    void emergencyStopTriggered(const QString& source);
    void globalShortcutTriggered(const QString& shortcut);

private slots:
    // Internal state management
    void onRobotConnected();
    void onRobotDisconnected();
    void onRobotEnabled();
    void onRobotDisabled();
    void onBatteryVoltageChanged(double voltage);
    void onPingLatencyChanged(int latency);
    void onCommunicationError(const QString& error);
    void updateStatusStrings();

#ifdef ENABLE_FMS_SUPPORT
    void onFMSConnected();
    void onFMSDisconnected();
    void onFMSModeChanged(int mode);
#endif

#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    // Global shortcut handlers
    void onEmergencyStopShortcut();
    void onDisableRobotShortcut();
    void onEnableRobotShortcut();
#endif

private:
    // Core state
    bool m_robotEnabled;
    bool m_emergencyStop;
    bool m_fmsConnected;
    int m_teamNumber;
    QString m_robotIpAddress;
    ConnectionMode m_connectionMode;
    RobotMode m_robotMode;
    ConnectionState m_connectionState;
    double m_batteryVoltage;
    int m_pingLatency;
    bool m_globalShortcutsEnabled;
    
    // Status strings
    QString m_commsStatus;
    QString m_robotCodeStatus;
    QString m_joystickStatus;
    double m_robotVoltage;
    double m_networkLatency;
    double m_packetLoss;
    QString m_consoleOutput;

    // Timestamps
    QDateTime m_lastPacketTime;
    QDateTime m_connectionStartTime;
    QDateTime m_lastEmergencyStopTime;

    // Thread safety
    mutable QMutex m_stateMutex;

    // Components
    CommunicationHandler* m_communicationHandler;
    ControllerHIDHandler* m_controllerHandler;
    BatteryManager* m_batteryManager;
    PracticeMatchManager* m_practiceMatchManager;
    NetworkManager* m_networkManager;

#ifdef ENABLE_FMS_SUPPORT
    FMSHandler* m_fmsHandler;
#endif

#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    // Global shortcuts
    QPointer<QHotkey> m_emergencyStopHotkey;
    QPointer<QHotkey> m_disableRobotHotkey;
    QPointer<QHotkey> m_enableRobotHotkey;
#endif

    // Timers
    QTimer* m_statusUpdateTimer;
    QTimer* m_connectionTimeoutTimer;

    // Helper methods
    void initializeComponents();
    void setupConnections();
    void setupTimers();
    void updateConnectionState(ConnectionState newState);
    void logStateChange(const QString& change);
    void saveSettings();
    void loadSettings();

#if defined(ENABLE_GLOBAL_SHORTCUTS) && defined(QHOTKEY_AVAILABLE)
    void setupGlobalShortcuts();
    void cleanupGlobalShortcuts();
    void registerGlobalShortcut(QHotkey*& hotkey, const QKeySequence& sequence, 
                               const char* slot, const QString& description);
#endif
};

#endif // ROBOTSTATE_H
