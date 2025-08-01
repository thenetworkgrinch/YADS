#include "fmshandler.h"
#include "../core/logger.h"
#include "../core/constants.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QDebug>

FMSHandler::FMSHandler(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_connectionTimer(new QTimer(this))
    , m_connected(false)
    , m_matchType("None")
    , m_matchNumber(0)
    , m_replayNumber(0)
    , m_alliance("Red")
    , m_position(1)
    , m_timeRemaining(0)
    , m_gameSpecificMessage("")
    , m_fmsAddress(QHostAddress("10.0.100.5"))
    , m_fmsPort(1750)
{
    // Setup socket connections
    connect(m_socket, &QUdpSocket::readyRead, this, &FMSHandler::processFMSData);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &FMSHandler::handleSocketError);

    // Setup timers
    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &FMSHandler::onHeartbeatTimer);

    m_connectionTimer->setSingleShot(true);
    m_connectionTimer->setInterval(CONNECTION_TIMEOUT);
    connect(m_connectionTimer, &QTimer::timeout, this, [this]() {
        if (m_connected) {
            m_connected = false;
            emit connectedChanged(false);
            Logger::instance().log("FMS connection timeout", Logger::Warning);
        }
    });

    detectFMSNetwork();
}

FMSHandler::~FMSHandler()
{
    disconnectFromFMS();
}

void FMSHandler::connectToFMS()
{
    if (m_connected) {
        return;
    }

    Logger::instance().log("Attempting to connect to FMS at " + m_fmsAddress.toString(), Logger::Info);

    if (!m_socket->bind(QHostAddress::Any, 1110)) {
        Logger::instance().log("Failed to bind FMS socket: " + m_socket->errorString(), Logger::Error);
        return;
    }

    m_heartbeatTimer->start();
    sendHeartbeat();
}

void FMSHandler::disconnectFromFMS()
{
    m_heartbeatTimer->stop();
    m_connectionTimer->stop();
    
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }

    if (m_connected) {
        m_connected = false;
        emit connectedChanged(false);
        Logger::instance().log("Disconnected from FMS", Logger::Info);
    }
}

void FMSHandler::sendHeartbeat()
{
    QJsonObject heartbeat;
    heartbeat["type"] = "heartbeat";
    heartbeat["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    heartbeat["teamNumber"] = Constants::TEAM_NUMBER;

    QJsonDocument doc(heartbeat);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    qint64 sent = m_socket->writeDatagram(data, m_fmsAddress, m_fmsPort);
    if (sent == -1) {
        Logger::instance().log("Failed to send FMS heartbeat: " + m_socket->errorString(), Logger::Warning);
    }
}

void FMSHandler::processFMSData()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        qint64 received = m_socket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        if (received > 0) {
            parseFMSPacket(data);
            
            // Reset connection timeout
            m_connectionTimer->start();
            
            if (!m_connected) {
                m_connected = true;
                emit connectedChanged(true);
                Logger::instance().log("Connected to FMS", Logger::Info);
            }
        }
    }
}

void FMSHandler::parseFMSPacket(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        Logger::instance().log("Failed to parse FMS packet: " + error.errorString(), Logger::Warning);
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "matchInfo") {
        updateMatchInfo(obj);
    } else if (type == "emergencyStop") {
        emit emergencyStop();
        Logger::instance().log("Emergency stop received from FMS", Logger::Critical);
    } else if (type == "matchStart") {
        emit matchStarted();
        Logger::instance().log("Match started", Logger::Info);
    } else if (type == "matchEnd") {
        emit matchEnded();
        Logger::instance().log("Match ended", Logger::Info);
    }
}

void FMSHandler::updateMatchInfo(const QJsonObject &matchInfo)
{
    QString newMatchType = matchInfo["matchType"].toString();
    if (newMatchType != m_matchType) {
        m_matchType = newMatchType;
        emit matchTypeChanged(m_matchType);
    }

    int newMatchNumber = matchInfo["matchNumber"].toInt();
    if (newMatchNumber != m_matchNumber) {
        m_matchNumber = newMatchNumber;
        emit matchNumberChanged(m_matchNumber);
    }

    int newReplayNumber = matchInfo["replayNumber"].toInt();
    if (newReplayNumber != m_replayNumber) {
        m_replayNumber = newReplayNumber;
        emit replayNumberChanged(m_replayNumber);
    }

    QString newAlliance = matchInfo["alliance"].toString();
    if (newAlliance != m_alliance) {
        m_alliance = newAlliance;
        emit allianceChanged(m_alliance);
    }

    int newPosition = matchInfo["position"].toInt();
    if (newPosition != m_position) {
        m_position = newPosition;
        emit positionChanged(m_position);
    }

    int newTimeRemaining = matchInfo["timeRemaining"].toInt();
    if (newTimeRemaining != m_timeRemaining) {
        m_timeRemaining = newTimeRemaining;
        emit timeRemainingChanged(m_timeRemaining);
    }

    QString newGameSpecificMessage = matchInfo["gameSpecificMessage"].toString();
    if (newGameSpecificMessage != m_gameSpecificMessage) {
        m_gameSpecificMessage = newGameSpecificMessage;
        emit gameSpecificMessageChanged(m_gameSpecificMessage);
    }
}

void FMSHandler::handleSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    Logger::instance().log("FMS socket error: " + errorString, Logger::Error);

    if (m_connected) {
        m_connected = false;
        emit connectedChanged(false);
    }
}

void FMSHandler::onHeartbeatTimer()
{
    sendHeartbeat();
}

void FMSHandler::detectFMSNetwork()
{
    // Try to detect FMS network automatically
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            for (const QNetworkAddressEntry &entry : entries) {
                QHostAddress addr = entry.ip();
                if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                    QString addrStr = addr.toString();
                    // Check if we're on the FMS network (10.0.100.x)
                    if (addrStr.startsWith("10.0.100.")) {
                        m_fmsAddress = QHostAddress("10.0.100.5");
                        Logger::instance().log("Detected FMS network, using address: " + m_fmsAddress.toString(), Logger::Info);
                        return;
                    }
                }
            }
        }
    }
    
    // Default FMS address if not detected
    Logger::instance().log("Using default FMS address: " + m_fmsAddress.toString(), Logger::Info);
}

QHostAddress FMSHandler::getFMSAddress()
{
    return QHostAddress("10.0.100.5");
}

quint16 FMSHandler::getFMSPort()
{
    return 1750;
}
