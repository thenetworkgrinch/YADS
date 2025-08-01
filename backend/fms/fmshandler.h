#ifndef FMSHANDLER_H
#define FMSHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkDatagram>
#include "backend/core/constants.h"

class FMSHandler : public QObject
{
    Q_OBJECT

public:
    explicit FMSHandler(QObject *parent = nullptr);
    ~FMSHandler();

    enum MatchState {
        Unknown = 0,
        Autonomous = 1,
        Teleop = 2,
        Test = 3,
        Disabled = 4
    };
    Q_ENUM(MatchState)

    enum AllianceColor {
        Red = 0,
        Blue = 1,
        InvalidAlliance = 2
    };
    Q_ENUM(AllianceColor)

    bool isConnected() const { return m_connected; }
    MatchState matchState() const { return m_matchState; }
    AllianceColor allianceColor() const { return m_allianceColor; }
    int matchNumber() const { return m_matchNumber; }
    int matchTime() const { return m_matchTime; }
    bool isEnabled() const { return m_enabled; }
    bool isEmergencyStop() const { return m_emergencyStop; }

public slots:
    void connectToFMS();
    void disconnectFromFMS();
    void setTeamNumber(int teamNumber);
    void sendHeartbeat();

signals:
    void connectionChanged(bool connected);
    void matchStateChanged(MatchState state);
    void allianceColorChanged(AllianceColor color);
    void matchNumberChanged(int number);
    void matchTimeChanged(int time);
    void enabledChanged(bool enabled);
    void emergencyStopChanged(bool emergencyStop);
    void fmsDataReceived(const QByteArray &data);

private slots:
    void processPendingDatagrams();
    void onHeartbeatTimer();
    void onConnectionTimeout();

private:
    void processPacket(const QByteArray &data, const QHostAddress &sender);
    void updateConnectionStatus(bool connected);
    void parseControlPacket(const QByteArray &data);
    void sendStatusPacket();

    QUdpSocket *m_socket;
    QTimer *m_heartbeatTimer;
    QTimer *m_connectionTimer;
    
    bool m_connected;
    int m_teamNumber;
    MatchState m_matchState;
    AllianceColor m_allianceColor;
    int m_matchNumber;
    int m_matchTime;
    bool m_enabled;
    bool m_emergencyStop;
    
    QHostAddress m_fmsAddress;
    quint16 m_fmsPort;
    quint16 m_localPort;
    
    static const int HEARTBEAT_INTERVAL = 1000; // ms
    static const int CONNECTION_TIMEOUT = 5000; // ms
};

#endif // FMSHANDLER_H
