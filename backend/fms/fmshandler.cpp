#include "fmshandler.h"
#include "backend/core/logger.h"
#include <QDataStream>
#include <QDateTime>

FMSHandler::FMSHandler(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_connectionTimer(new QTimer(this))
    , m_connected(false)
    , m_teamNumber(0)
    , m_matchState(Unknown)
    , m_allianceColor(InvalidAlliance)
    , m_matchNumber(0)
    , m_matchTime(0)
    , m_enabled(false)
    , m_emergencyStop(false)
    , m_fmsAddress(QHostAddress("10.0.100.5"))
    , m_fmsPort(1160)
    , m_localPort(1110)
{
    // Setup timers
    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL);
    m_heartbeatTimer->setSingleShot(false);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &FMSHandler::onHeartbeatTimer);
    
    m_connectionTimer->setInterval(CONNECTION_TIMEOUT);
    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout, this, &FMSHandler::onConnectionTimeout);
    
    // Setup socket
    connect(m_socket, &QUdpSocket::readyRead, this, &FMSHandler::processPendingDatagrams);
    
    Logger::instance().log(Logger::Info, "FMS Handler initialized");
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
    
    if (!m_socket->bind(QHostAddress::Any, m_localPort)) {
        Logger::instance().log(Logger::Error, QString("Failed to bind FMS socket to port %1").arg(m_localPort));
        return;
    }
    
    m_heartbeatTimer->start();
    m_connectionTimer->start();
    
    Logger::instance().log(Logger::Info, QString("Attempting to connect to FMS at %1:%2")
                          .arg(m_fmsAddress.toString()).arg(m_fmsPort));
    
    sendHeartbeat();
}

void FMSHandler::disconnectFromFMS()
{
    m_heartbeatTimer->stop();
    m_connectionTimer->stop();
    m_socket->close();
    
    if (m_connected) {
        updateConnectionStatus(false);
        Logger::instance().log(Logger::Info, "Disconnected from FMS");
    }
}

void FMSHandler::setTeamNumber(int teamNumber)
{
    if (m_teamNumber != teamNumber) {
        m_teamNumber = teamNumber;
        
        // Update FMS address based on team number
        if (teamNumber > 0) {
            int firstOctet = teamNumber / 100;
            int secondOctet = teamNumber % 100;
            m_fmsAddress = QHostAddress(QString("10.%1.%2.5").arg(firstOctet).arg(secondOctet));
        }
        
        Logger::instance().log(Logger::Info, QString("Team number set to %1, FMS address: %2")
                              .arg(teamNumber).arg(m_fmsAddress.toString()));
    }
}

void FMSHandler::sendHeartbeat()
{
    sendStatusPacket();
}

void FMSHandler::processPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        processPacket(datagram.data(), datagram.senderAddress());
    }
}

void FMSHandler::onHeartbeatTimer()
{
    sendStatusPacket();
}

void FMSHandler::onConnectionTimeout()
{
    if (m_connected) {
        updateConnectionStatus(false);
        Logger::instance().log(Logger::Warning, "FMS connection timeout");
    }
}

void FMSHandler::processPacket(const QByteArray &data, const QHostAddress &sender)
{
    if (data.size() < 8) {
        return; // Invalid packet size
    }
    
    // Reset connection timeout
    m_connectionTimer->start();
    
    if (!m_connected) {
        updateConnectionStatus(true);
    }
    
    parseControlPacket(data);
    emit fmsDataReceived(data);
}

void FMSHandler::updateConnectionStatus(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit connectionChanged(connected);
    }
}

void FMSHandler::parseControlPacket(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint16 packetNumber;
    quint8 matchType;
    quint16 matchNumber;
    quint8 playNumber;
    quint16 timeRemaining;
    quint8 controlByte;
    
    stream >> packetNumber >> matchType >> matchNumber >> playNumber >> timeRemaining >> controlByte;
    
    // Parse control byte
    bool enabled = (controlByte & 0x01) != 0;
    bool autonomous = (controlByte & 0x02) != 0;
    bool test = (controlByte & 0x04) != 0;
    bool emergencyStop = (controlByte & 0x80) != 0;
    AllianceColor alliance = (controlByte & 0x08) ? Blue : Red;
    
    // Update match state
    MatchState newState;
    if (emergencyStop || !enabled) {
        newState = Disabled;
    } else if (test) {
        newState = Test;
    } else if (autonomous) {
        newState = Autonomous;
    } else {
        newState = Teleop;
    }
    
    // Emit changes
    if (m_matchState != newState) {
        m_matchState = newState;
        emit matchStateChanged(newState);
    }
    
    if (m_allianceColor != alliance) {
        m_allianceColor = alliance;
        emit allianceColorChanged(alliance);
    }
    
    if (m_matchNumber != matchNumber) {
        m_matchNumber = matchNumber;
        emit matchNumberChanged(matchNumber);
    }
    
    if (m_matchTime != timeRemaining) {
        m_matchTime = timeRemaining;
        emit matchTimeChanged(timeRemaining);
    }
    
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
    }
    
    if (m_emergencyStop != emergencyStop) {
        m_emergencyStop = emergencyStop;
        emit emergencyStopChanged(emergencyStop);
    }
}

void FMSHandler::sendStatusPacket()
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // FMS status packet format
    stream << quint16(m_teamNumber);  // Team number
    stream << quint8(0x01);           // Packet type (status)
    stream << quint8(0x00);           // Reserved
    stream << quint32(QDateTime::currentSecsSinceEpoch()); // Timestamp
    stream << quint8(0x00);           // Status flags
    stream << quint8(0x00);           // Reserved
    
    m_socket->writeDatagram(packet, m_fmsAddress, m_fmsPort);
}
