#ifndef COMMUNICATIONHANDLER_H
#define COMMUNICATIONHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QDateTime>
#include <QMutex>
#include <QQueue>
#include <QThread>

#include "packets.h"
#include "../core/logger.h"

/**
 * @brief Handles all network communication with the robot
 * 
 * This class manages UDP communication with FRC robots, including:
 * - Robot discovery and connection
 * - Packet transmission and reception
 * - Connection monitoring and recovery
 * - Latency measurement
 */
class CommunicationHandler : public QObject
{
    Q_OBJECT
    
public:
    enum ConnectionState {
        Disconnected,
        Discovering,
        Connecting,
        Connected,
        ConnectionLost
    };
    Q_ENUM(ConnectionState)
    
    enum ConnectionMode {
        TeamNumber,
        IpAddress
    };
    Q_ENUM(ConnectionMode)
    
    explicit CommunicationHandler(QObject *parent = nullptr);
    ~CommunicationHandler();
    
    // Connection management
    void setTeamNumber(int teamNumber);
    void setRobotIpAddress(const QString& ipAddress);
    void setConnectionMode(ConnectionMode mode);
    int teamNumber() const { return m_teamNumber; }
    QString robotIpAddress() const { return m_robotIpAddress; }
    ConnectionMode connectionMode() const { return m_connectionMode; }
    
    ConnectionState connectionState() const { return m_connectionState; }
    QHostAddress robotAddress() const { return m_robotAddress; }
    int pingLatency() const { return m_pingLatency; }
    
    // Robot control
    void enableRobot();
    void disableRobot();
    void emergencyStop();
    void setRobotMode(int mode);
    void setAlliance(int alliance, int position);
    
    // Joystick data
    void updateJoystickData(int joystickIndex, const QByteArray& data);
    
    // Statistics
    int packetsSent() const { return m_packetsSent; }
    int packetsReceived() const { return m_packetsReceived; }
    int packetsLost() const { return m_packetsLost; }
    double packetLossRate() const;

public slots:
    void connectToRobot();
    void disconnectFromRobot();
    void restartCommunication();

signals: 
    void connectToRobot();
    void disconnectFromRobot();
    void restartCommunication();

signals:
    // Connection events
    void robotConnected();
    void robotDisconnected();
    void connectionStateChanged(ConnectionState state);
    void robotAddressChanged(const QHostAddress& address);
    
    // Robot state events
    void robotEnabled();
    void robotDisabled();
    void robotModeChanged(int mode);
    void batteryVoltageChanged(double voltage);
    void robotStatusChanged(const QByteArray& status);
    
    // Communication events
    void packetSent(const QByteArray& packet);
    void packetReceived(const QByteArray& packet);
    void communicationError(const QString& error);
    void pingLatencyChanged(int latency);
    
    // Console messages
    void consoleMessageReceived(const QString& message, const QDateTime& timestamp);

private slots:
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onDiscoveryTimeout();
    void onHeartbeatTimeout();
    void onSendControlPacket();
    void onPingTimeout();

private:
    // Network components
    QUdpSocket* m_socket;
    QHostAddress m_robotAddress;
    quint16 m_robotPort;
    quint16 m_localPort;
    
    // Connection configuration
    int m_teamNumber;
    QString m_robotIpAddress;
    ConnectionMode m_connectionMode;
    ConnectionState m_connectionState;
    QDateTime m_lastPacketTime;
    QDateTime m_connectionStartTime;
    
    // Timers
    QTimer* m_discoveryTimer;
    QTimer* m_heartbeatTimer;
    QTimer* m_controlPacketTimer;
    QTimer* m_pingTimer;
    
    // Robot state
    bool m_robotEnabled;
    int m_robotMode;
    int m_alliance;
    int m_position;
    bool m_emergencyStop;
    QByteArray m_joysticks[6];
    
    // Packet management
    quint16 m_sequenceNumber;
    QQueue<QByteArray> m_sendQueue;
    mutable QMutex m_sendQueueMutex;
    
    // Statistics
    int m_packetsSent;
    int m_packetsReceived;
    int m_packetsLost;
    int m_pingLatency;
    QDateTime m_lastPingTime;
    
    // Thread safety
    mutable QMutex m_stateMutex;
    
    // Helper methods
    void initializeSocket();
    void setupTimers();
    void updateConnectionState(ConnectionState newState);
    
    // Robot discovery
    void startRobotDiscovery();
    void stopRobotDiscovery();
    QList<QHostAddress> generateRobotAddresses() const;
    bool pingRobotAddress(const QHostAddress& address);
    
    // Packet handling
    void sendPacket(const QByteArray& packet);
    void processReceivedPacket(const QByteArray& packet, const QHostAddress& sender);
    void handleControlPacket(const QByteArray& packet);
    void handleStatusPacket(const QByteArray& packet);
    void handleConsolePacket(const QByteArray& packet);
    void handleHeartbeatPacket(const QByteArray& packet);
    
    // Control packet creation
    QByteArray createControlPacket() const;
    void sendControlPacket();
    void sendHeartbeatPacket();
    void sendPingPacket();
    
    // Utility methods
    QString connectionStateToString(ConnectionState state) const;
    void logPacketStatistics();
    void resetStatistics();
    
    // Network utilities
    QHostAddress getLocalAddress() const;
    bool isValidRobotAddress(const QHostAddress& address) const;
    quint16 calculateTeamPort(int teamNumber) const;
};

#endif // COMMUNICATIONHANDLER_H
