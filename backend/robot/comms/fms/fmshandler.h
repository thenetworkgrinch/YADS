#ifndef FMSHANDLER_H
#define FMSHANDLER_H

#include "../../../core/constants.h"

#ifdef ENABLE_FMS_SUPPORT

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
 * 
 * Design principles:
 * - Single responsibility: Only handles FMS communication
 * - Fail fast: Clear error reporting and recovery
 * - Non-blocking: All operations are asynchronous
 * - Observable: Emits clear state change signals
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

    // State queries
    FMSState currentState() const { return m_currentState; }
    bool isConnected() const { return m_currentState == Connected; }
    int currentMatchNumber() const { return m_currentMatchNumber; }
    QString currentMatchType() const { return m_currentMatchType; }
    QHostAddress fmsAddress() const { return m_fmsAddress; }
    qint64 timeSinceLastContact() const;

    // Control methods
    void startMonitoring();
    void stopMonitoring();

signals:
    void stateChanged(FMSState state, const QHostAddress &address);
    void fmsConnected(const QHostAddress &address);
    void fmsDisconnected();
    void matchInfoReceived(int matchNumber, const QString &matchType);
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
};

} // namespace FRCDriverStation

#else // !ENABLE_FMS_SUPPORT

// Stub implementation when FMS support is disabled
namespace FRCDriverStation {

class Logger;

class FMSHandler : public QObject
{
    Q_OBJECT

public:
    enum FMSState { Disconnected = 0 };
    Q_ENUM(FMSState)

    explicit FMSHandler(std::shared_ptr<Logger>, QObject *parent = nullptr) : QObject(parent) {}
    ~FMSHandler() = default;

    FMSState currentState() const { return Disconnected; }
    bool isConnected() const { return false; }
    int currentMatchNumber() const { return -1; }
    QString currentMatchType() const { return QString(); }
    QHostAddress fmsAddress() const { return QHostAddress(); }
    qint64 timeSinceLastContact() const { return -1; }

    void startMonitoring() {}
    void stopMonitoring() {}

signals:
    void stateChanged(FMSState, const QHostAddress &);
    void fmsConnected(const QHostAddress &);
    void fmsDisconnected();
    void matchInfoReceived(int, const QString &);
    void matchControlReceived(bool, bool, bool);
};

} // namespace FRCDriverStation

#endif // ENABLE_FMS_SUPPORT

#endif // FMSHANDLER_H
