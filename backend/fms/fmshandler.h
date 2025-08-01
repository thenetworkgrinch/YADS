#ifndef FMSHANDLER_H
#define FMSHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QDateTime>
#include <QMutex>
#include "../core/logger.h"

/**
 * @brief Handles FMS (Field Management System) communication
 * 
 * This class manages communication with the FMS during official matches,
 * receiving match state information and robot control commands.
 */
class FMSHandler : public QObject
{
    Q_OBJECT

public:
    enum MatchType {
        None = 0,
        Practice = 1,
        Qualification = 2,
        Elimination = 3
    };
    Q_ENUM(MatchType)

    enum MatchPhase {
        PreMatch = 0,
        Autonomous = 1,
        Teleop = 2,
        PostMatch = 3
    };
    Q_ENUM(MatchPhase)

    struct MatchInfo {
        MatchType type = None;
        int matchNumber = 0;
        int replayNumber = 0;
        QString eventName;
        QDateTime startTime;
    };

    struct MatchState {
        MatchPhase phase = PreMatch;
        bool enabled = false;
        bool emergencyStop = false;
        int timeRemaining = 0;
        QDateTime timestamp;
    };

    explicit FMSHandler(QObject *parent = nullptr);
    ~FMSHandler();

    // Getters
    bool isConnected() const { return m_connected; }
    MatchInfo currentMatch() const { return m_currentMatch; }
    MatchState currentState() const { return m_currentState; }
    QHostAddress fmsAddress() const { return m_fmsAddress; }
    int fmsPort() const { return m_fmsPort; }

    // Control methods
    void startListening();
    void stopListening();
    void setListenPort(int port);

public slots:
    void connectToFMS(const QHostAddress& address, int port = 1750);
    void disconnectFromFMS();
    void sendRobotStatus(bool enabled, bool emergencyStop, double batteryVoltage);

signals:
    void fmsConnected();
    void fmsDisconnected();
    void fmsModeChanged(int mode);
    void matchInfoReceived(const MatchInfo& info);
    void matchStateChanged(const MatchState& state);
    void fmsError(const QString& error);

private slots:
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout();
    void sendHeartbeat();

private:
    // Network components
    QUdpSocket* m_socket;
    QHostAddress m_fmsAddress;
    int m_fmsPort;
    int m_listenPort;
    
    // Connection state
    bool m_connected;
    QDateTime m_lastHeartbeat;
    QTimer* m_heartbeatTimer;
    QTimer* m_timeoutTimer;
    
    // Match information
    MatchInfo m_currentMatch;
    MatchState m_currentState;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Packet processing
    void processIncomingPacket(const QByteArray& data, const QHostAddress& sender);
    void processMatchInfo(const QByteArray& data);
    void processMatchState(const QByteArray& data);
    void processControlCommand(const QByteArray& data);
    
    // Packet creation
    QByteArray createHeartbeatPacket();
    QByteArray createStatusPacket(bool enabled, bool emergencyStop, double batteryVoltage);
    
    // Helper methods
    void updateConnectionState(bool connected);
    void resetMatchInfo();
    void logFMSEvent(const QString& event);
};

#endif // FMSHANDLER_H
