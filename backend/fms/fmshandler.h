#ifndef FMSHANDLER_H
#define FMSHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * @brief Handles communication with the Field Management System (FMS)
 * 
 * The FMSHandler manages the connection and communication with the FMS during
 * official competitions. It handles match state, timing, and field control.
 */
class FMSHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString matchType READ matchType NOTIFY matchTypeChanged)
    Q_PROPERTY(int matchNumber READ matchNumber NOTIFY matchNumberChanged)
    Q_PROPERTY(int replayNumber READ replayNumber NOTIFY replayNumberChanged)
    Q_PROPERTY(QString alliance READ alliance NOTIFY allianceChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(int timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(QString gameSpecificMessage READ gameSpecificMessage NOTIFY gameSpecificMessageChanged)

public:
    explicit FMSHandler(QObject *parent = nullptr);
    ~FMSHandler();

    // Connection status
    bool isConnected() const { return m_connected; }
    
    // Match information
    QString matchType() const { return m_matchType; }
    int matchNumber() const { return m_matchNumber; }
    int replayNumber() const { return m_replayNumber; }
    QString alliance() const { return m_alliance; }
    int position() const { return m_position; }
    int timeRemaining() const { return m_timeRemaining; }
    QString gameSpecificMessage() const { return m_gameSpecificMessage; }

    // FMS addresses
    static QHostAddress getFMSAddress();
    static quint16 getFMSPort();

public slots:
    void connectToFMS();
    void disconnectFromFMS();
    void sendHeartbeat();

signals:
    void connectedChanged(bool connected);
    void matchTypeChanged(const QString &matchType);
    void matchNumberChanged(int matchNumber);
    void replayNumberChanged(int replayNumber);
    void allianceChanged(const QString &alliance);
    void positionChanged(int position);
    void timeRemainingChanged(int timeRemaining);
    void gameSpecificMessageChanged(const QString &message);
    void matchStarted();
    void matchEnded();
    void emergencyStop();

private slots:
    void processFMSData();
    void handleSocketError(QAbstractSocket::SocketError error);
    void onHeartbeatTimer();

private:
    void parseFMSPacket(const QByteArray &data);
    void updateMatchInfo(const QJsonObject &matchInfo);
    void detectFMSNetwork();

    QUdpSocket *m_socket;
    QTimer *m_heartbeatTimer;
    QTimer *m_connectionTimer;
    
    bool m_connected;
    QString m_matchType;
    int m_matchNumber;
    int m_replayNumber;
    QString m_alliance;
    int m_position;
    int m_timeRemaining;
    QString m_gameSpecificMessage;
    
    QHostAddress m_fmsAddress;
    quint16 m_fmsPort;
    
    static const int HEARTBEAT_INTERVAL = 1000; // 1 second
    static const int CONNECTION_TIMEOUT = 5000; // 5 seconds
};

#endif // FMSHANDLER_H
