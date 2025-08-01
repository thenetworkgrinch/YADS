#ifndef FMSHANDLER_H
#define FMSHANDLER_H

#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <memory>

namespace FRCDriverStation {

class Logger;

/**
 * @brief Handles FMS (Field Management System) connection detection and management
 * 
 * The FMS handler monitors for FMS presence on the network and manages
 * the connection state. It detects when FMS comes online during a match
 * and when it disconnects after the match ends.
 */
class FMSHandler : public QObject
{
    Q_OBJECT

public:
    enum FMSState {
        Disconnected = 0,
        Detecting = 1,
        Connected = 2,
        Error = 3
    };
    Q_ENUM(FMSState)

    explicit FMSHandler(std::shared_ptr<Logger> logger, QObject *parent = nullptr);
    ~FMSHandler();

    /**
     * @brief Start monitoring for FMS connection
     */
    void startMonitoring();

    /**
     * @brief Stop monitoring for FMS connection
     */
    void stopMonitoring();

    /**
     * @brief Get current FMS connection state
     * @return Current FMS state
     */
    FMSState currentState() const { return m_currentState; }

    /**
     * @brief Check if FMS is currently connected
     * @return true if FMS is connected
     */
    bool isConnected() const { return m_currentState == Connected; }

    /**
     * @brief Get current match number (only valid when FMS connected)
     * @return Match number or -1 if not available
     */
    int currentMatchNumber() const { return m_currentMatchNumber; }

    /**
     * @brief Get current match type (only valid when FMS connected)
     * @return Match type string
     */
    QString currentMatchType() const { return m_currentMatchType; }

    /**
     * @brief Get FMS IP address if connected
     * @return FMS IP address or null address if not connected
     */
    QHostAddress fmsAddress() const { return m_fmsAddress; }

    /**
     * @brief Get time since FMS was last seen
     * @return Milliseconds since last FMS contact
     */
    qint64 timeSinceLastContact() const;

signals:
    /**
     * @brief Emitted when FMS connection state changes
     * @param state New FMS state
     * @param address FMS IP address (if connected)
     */
    void stateChanged(FMSState state, const QHostAddress &address);

    /**
     * @brief Emitted when FMS connects
     * @param address FMS IP address
     */
    void fmsConnected(const QHostAddress &address);

    /**
     * @brief Emitted when FMS disconnects
     */
    void fmsDisconnected();

    /**
     * @brief Emitted when match information is received from FMS
     * @param matchNumber Match number
     * @param matchType Match type (Practice, Qualification, Playoff, etc.)
     */
    void matchInfoReceived(int matchNumber, const QString &matchType);

    /**
     * @brief Emitted when FMS sends match control data
     * @param enabled Robot should be enabled
     * @param autonomous Robot should be in autonomous mode
     * @param test Robot should be in test mode
     */
    void matchControlReceived(bool enabled, bool autonomous, bool test);

private slots:
    void onDetectionTimerTimeout();
    void onHeartbeatTimerTimeout();
    void onFmsDataReceived();
    void onFmsTcpConnected();
    void onFmsTcpDisconnected();
    void onFmsTcpError();

private:
    void detectFMS();
    void connectToFMS(const QHostAddress &address);
    void disconnectFromFMS();
    void updateState(FMSState newState);
    void parseFMSPacket(const QByteArray &data);
    void sendHeartbeat();
    QHostAddress findFMSAddress();

    std::shared_ptr<Logger> m_logger;
    
    // Network components
    std::unique_ptr<QUdpSocket> m_detectionSocket;
    std::unique_ptr<QTcpSocket> m_fmsSocket;
    std::unique_ptr<QTimer> m_detectionTimer;
    std::unique_ptr<QTimer> m_heartbeatTimer;
    
    // State tracking
    FMSState m_currentState;
    QHostAddress m_fmsAddress;
    qint64 m_lastContactTime;
    int m_currentMatchNumber;
    QString m_currentMatchType;
    
    // Configuration
    static const int DETECTION_INTERVAL_MS = 2000;  // Check every 2 seconds
    static const int HEARTBEAT_INTERVAL_MS = 1000;  // Heartbeat every second
    static const int FMS_TIMEOUT_MS = 5000;         // Consider disconnected after 5 seconds
    static const quint16 FMS_DETECTION_PORT = 1750; // FMS detection port
    static const quint16 FMS_DATA_PORT = 1735;      // FMS data port
};

} // namespace FRCDriverStation

#endif // FMSHANDLER_H
