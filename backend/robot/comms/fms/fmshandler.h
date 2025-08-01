#ifndef ROBOT_FMS_HANDLER_H
#define ROBOT_FMS_HANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include "backend/core/constants.h"

class RobotFMSHandler : public QObject
{
    Q_OBJECT

public:
    explicit RobotFMSHandler(QObject *parent = nullptr);
    ~RobotFMSHandler();

    enum RobotMode {
        Disabled = 0,
        Autonomous = 1,
        Teleop = 2,
        Test = 3
    };
    Q_ENUM(RobotMode)

    bool isConnected() const { return m_connected; }
    RobotMode currentMode() const { return m_currentMode; }
    bool isEnabled() const { return m_enabled; }
    bool isEmergencyStop() const { return m_emergencyStop; }
    int batteryVoltage() const { return m_batteryVoltage; }

public slots:
    void connectToRobot();
    void disconnectFromRobot();
    void setTeamNumber(int teamNumber);
    void setRobotMode(RobotMode mode);
    void setEnabled(bool enabled);
    void setEmergencyStop(bool emergencyStop);

signals:
    void connectionChanged(bool connected);
    void robotModeChanged(RobotMode mode);
    void enabledChanged(bool enabled);
    void emergencyStopChanged(bool emergencyStop);
    void batteryVoltageChanged(int voltage);
    void robotDataReceived(const QByteArray &data);

private slots:
    void processPendingDatagrams();
    void sendControlPacket();
    void onConnectionTimeout();

private:
    void processStatusPacket(const QByteArray &data);
    void updateConnectionStatus(bool connected);

    QUdpSocket *m_socket;
    QTimer *m_controlTimer;
    QTimer *m_connectionTimer;
    
    bool m_connected;
    int m_teamNumber;
    RobotMode m_currentMode;
    bool m_enabled;
    bool m_emergencyStop;
    int m_batteryVoltage;
    
    QHostAddress m_robotAddress;
    quint16 m_robotPort;
    quint16 m_localPort;
    
    static const int CONTROL_INTERVAL = 20; // ms (50Hz)
    static const int CONNECTION_TIMEOUT = 1000; // ms
};

#endif // ROBOT_FMS_HANDLER_H
