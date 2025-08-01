#include "fmshandler.h"
#include <QNetworkDatagram>
#include <QDataStream>
#include <QMutexLocker>
#include <QDebug>

FMSHandler::FMSHandler(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_fmsAddress()
    , m_fmsPort(1750)
    , m_listenPort(1110)
    , m_connected(false)
    , m_heartbeatTimer(nullptr)
    , m_timeoutTimer(nullptr)
{
    Logger::instance().log(Logger::Info, "FMSHandler", "Initializing FMS handler");
    
    // Initialize socket
    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &FMSHandler::onSocketReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
            this, &FMSHandler::onSocketError);
    
    // Initialize timers
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(1000); // 1 second heartbeat
    connect(m_heartbeatTimer, &QTimer::timeout, this, &FMSHandler::sendHeartbeat);
    
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(5000); // 5 second timeout
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &FMSHandler::onHeartbeatTimeout);
    
    // Initialize match info
    resetMatchInfo();
    
    Logger::instance().log(Logger::Info, "FMSHandler", "FMS handler initialized");
}

FMSHandler::~FMSHandler()
{
    Logger::instance().log(Logger::Info, "FMSHandler", "Shutting down FMS handler");
    
    if (m_connected) {
        disconnectFromFMS();
    }
    
    if (m_socket && m_socket->state() == QUdpSocket::BoundState) {
        m_socket->close();
    }
    
    Logger::instance().log(Logger::Info, "FMSHandler", "FMS handler shutdown complete");
}

void FMSHandler::startListening()
{
    if (m_socket->state() == QUdpSocket::BoundState) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Already listening for FMS packets");
        return;
    }
    
    if (m_socket->bind(QHostAddress::Any, m_listenPort)) {
        Logger::instance().log(Logger::Info, "FMSHandler", 
                              QString("Started listening for FMS packets on port %1").arg(m_listenPort));
    } else {
        Logger::instance().log(Logger::Error, "FMSHandler", 
                              QString("Failed to bind to port %1: %2")
                              .arg(m_listenPort)
                              .arg(m_socket->errorString()));
        emit fmsError(QString("Failed to bind to FMS port %1").arg(m_listenPort));
    }
}

void FMSHandler::stopListening()
{
    if (m_socket->state() == QUdpSocket::BoundState) {
        m_socket->close();
        Logger::instance().log(Logger::Info, "FMSHandler", "Stopped listening for FMS packets");
    }
}

void FMSHandler::setListenPort(int port)
{
    if (m_listenPort != port) {
        bool wasListening = (m_socket->state() == QUdpSocket::BoundState);
        
        if (wasListening) {
            stopListening();
        }
        
        m_listenPort = port;
        Logger::instance().log(Logger::Info, "FMSHandler", 
                              QString("FMS listen port changed to %1").arg(port));
        
        if (wasListening) {
            startListening();
        }
    }
}

void FMSHandler::connectToFMS(const QHostAddress& address, int port)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_connected && m_fmsAddress == address && m_fmsPort == port) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Already connected to this FMS");
        return;
    }
    
    // Disconnect from current FMS if connected
    if (m_connected) {
        disconnectFromFMS();
    }
    
    m_fmsAddress = address;
    m_fmsPort = port;
    
    Logger::instance().log(Logger::Info, "FMSHandler", 
                          QString("Connecting to FMS at %1:%2")
                          .arg(address.toString())
                          .arg(port));
    
    // Start listening for FMS packets
    startListening();
    
    // Start heartbeat
    m_heartbeatTimer->start();
    m_timeoutTimer->start();
    
    // Send initial heartbeat
    sendHeartbeat();
    
    updateConnectionState(true);
}

void FMSHandler::disconnectFromFMS()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connected) {
        return;
    }
    
    Logger::instance().log(Logger::Info, "FMSHandler", "Disconnecting from FMS");
    
    // Stop timers
    m_heartbeatTimer->stop();
    m_timeoutTimer->stop();
    
    // Stop listening
    stopListening();
    
    // Reset state
    resetMatchInfo();
    updateConnectionState(false);
    
    Logger::instance().log(Logger::Info, "FMSHandler", "Disconnected from FMS");
}

void FMSHandler::sendRobotStatus(bool enabled, bool emergencyStop, double batteryVoltage)
{
    if (!m_connected) {
        return;
    }
    
    QByteArray packet = createStatusPacket(enabled, emergencyStop, batteryVoltage);
    
    qint64 bytesWritten = m_socket->writeDatagram(packet, m_fmsAddress, m_fmsPort);
    if (bytesWritten == -1) {
        Logger::instance().log(Logger::Error, "FMSHandler", 
                              QString("Failed to send status packet: %1").arg(m_socket->errorString()));
        emit fmsError("Failed to send robot status to FMS");
    }
}

void FMSHandler::onSocketReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        
        if (datagram.isValid()) {
            processIncomingPacket(datagram.data(), datagram.senderAddress());
        }
    }
}

void FMSHandler::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    Logger::instance().log(Logger::Error, "FMSHandler", 
                          QString("Socket error: %1 (%2)").arg(errorString).arg(error));
    
    emit fmsError(QString("FMS communication error: %1").arg(errorString));
    
    if (m_connected) {
        updateConnectionState(false);
    }
}

void FMSHandler::onHeartbeatTimeout()
{
    Logger::instance().log(Logger::Warning, "FMSHandler", "FMS heartbeat timeout");
    
    if (m_connected) {
        updateConnectionState(false);
        emit fmsError("Lost connection to FMS (heartbeat timeout)");
    }
}

void FMSHandler::sendHeartbeat()
{
    if (!m_connected) {
        return;
    }
    
    QByteArray packet = createHeartbeatPacket();
    
    qint64 bytesWritten = m_socket->writeDatagram(packet, m_fmsAddress, m_fmsPort);
    if (bytesWritten == -1) {
        Logger::instance().log(Logger::Error, "FMSHandler", 
                              QString("Failed to send heartbeat: %1").arg(m_socket->errorString()));
    } else {
        m_lastHeartbeat = QDateTime::currentDateTime();
        m_timeoutTimer->start(); // Restart timeout timer
    }
}

void FMSHandler::processIncomingPacket(const QByteArray& data, const QHostAddress& sender)
{
    // Verify sender is the FMS we're connected to
    if (m_connected && sender != m_fmsAddress) {
        return;
    }
    
    if (data.size() < 4) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Received malformed FMS packet");
        return;
    }
    
    // Parse packet header
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint16 packetType;
    quint16 packetLength;
    stream >> packetType >> packetLength;
    
    if (packetLength != data.size()) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "FMS packet length mismatch");
        return;
    }
    
    // Process based on packet type
    switch (packetType) {
        case 0x01: // Match info
            processMatchInfo(data);
            break;
        case 0x02: // Match state
            processMatchState(data);
            break;
        case 0x03: // Control command
            processControlCommand(data);
            break;
        case 0xFF: // Heartbeat response
            m_timeoutTimer->start(); // Reset timeout
            break;
        default:
            Logger::instance().log(Logger::Warning, "FMSHandler", 
                                  QString("Unknown FMS packet type: 0x%1").arg(packetType, 2, 16, QChar('0')));
            break;
    }
}

void FMSHandler::processMatchInfo(const QByteArray& data)
{
    if (data.size() < 20) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Invalid match info packet size");
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(4); // Skip header
    
    MatchInfo info;
    quint8 matchType;
    quint16 matchNumber;
    quint8 replayNumber;
    quint64 startTime;
    
    stream >> matchType >> matchNumber >> replayNumber >> startTime;
    
    info.type = static_cast<MatchType>(matchType);
    info.matchNumber = matchNumber;
    info.replayNumber = replayNumber;
    info.startTime = QDateTime::fromSecsSinceEpoch(startTime);
    
    // Read event name (remaining bytes)
    int remainingBytes = data.size() - stream.device()->pos();
    if (remainingBytes > 0) {
        QByteArray eventNameBytes(remainingBytes, 0);
        stream.readRawData(eventNameBytes.data(), remainingBytes);
        info.eventName = QString::fromUtf8(eventNameBytes).trimmed();
    }
    
    QMutexLocker locker(&m_mutex);
    m_currentMatch = info;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "FMSHandler", 
                          QString("Match info received: %1 Match %2 at %3")
                          .arg(info.type == Practice ? "Practice" : 
                               info.type == Qualification ? "Qualification" : "Elimination")
                          .arg(info.matchNumber)
                          .arg(info.eventName));
    
    emit matchInfoReceived(info);
}

void FMSHandler::processMatchState(const QByteArray& data)
{
    if (data.size() < 12) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Invalid match state packet size");
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(4); // Skip header
    
    MatchState state;
    quint8 phase;
    quint8 enabled;
    quint8 emergencyStop;
    quint16 timeRemaining;
    
    stream >> phase >> enabled >> emergencyStop >> timeRemaining;
    
    state.phase = static_cast<MatchPhase>(phase);
    state.enabled = (enabled != 0);
    state.emergencyStop = (emergencyStop != 0);
    state.timeRemaining = timeRemaining;
    state.timestamp = QDateTime::currentDateTime();
    
    QMutexLocker locker(&m_mutex);
    m_currentState = state;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "FMSHandler", 
                          QString("Match state: Phase=%1, Enabled=%2, E-Stop=%3, Time=%4")
                          .arg(state.phase)
                          .arg(state.enabled ? "Yes" : "No")
                          .arg(state.emergencyStop ? "Yes" : "No")
                          .arg(state.timeRemaining));
    
    emit matchStateChanged(state);
    emit fmsModeChanged(static_cast<int>(state.phase));
}

void FMSHandler::processControlCommand(const QByteArray& data)
{
    if (data.size() < 8) {
        Logger::instance().log(Logger::Warning, "FMSHandler", "Invalid control command packet size");
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(4); // Skip header
    
    quint8 command;
    quint8 value;
    
    stream >> command >> value;
    
    switch (command) {
        case 0x01: // Emergency stop
            if (value != 0) {
                Logger::instance().log(Logger::Warning, "FMSHandler", "FMS emergency stop command received");
                emit fmsModeChanged(0); // Disabled mode
            }
            break;
        case 0x02: // Enable/disable
            Logger::instance().log(Logger::Info, "FMSHandler", 
                                  QString("FMS %1 command received").arg(value ? "enable" : "disable"));
            break;
        default:
            Logger::instance().log(Logger::Warning, "FMSHandler", 
                                  QString("Unknown FMS control command: 0x%1").arg(command, 2, 16, QChar('0')));
            break;
    }
}

QByteArray FMSHandler::createHeartbeatPacket()
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << quint16(0xFF) << quint16(8); // Heartbeat packet type and length
    stream << quint32(QDateTime::currentSecsSinceEpoch()); // Timestamp
    
    return packet;
}

QByteArray FMSHandler::createStatusPacket(bool enabled, bool emergencyStop, double batteryVoltage)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << quint16(0x10) << quint16(12); // Status packet type and length
    stream << quint8(enabled ? 1 : 0);
    stream << quint8(emergencyStop ? 1 : 0);
    stream << quint16(static_cast<quint16>(batteryVoltage * 100)); // Voltage in centivolt
    stream << quint32(QDateTime::currentSecsSinceEpoch()); // Timestamp
    
    return packet;
}

void FMSHandler::updateConnectionState(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        
        if (connected) {
            Logger::instance().log(Logger::Info, "FMSHandler", "Connected to FMS");
            emit fmsConnected();
        } else {
            Logger::instance().log(Logger::Info, "FMSHandler", "Disconnected from FMS");
            emit fmsDisconnected();
        }
    }
}

void FMSHandler::resetMatchInfo()
{
    QMutexLocker locker(&m_mutex);
    
    m_currentMatch = MatchInfo();
    m_currentState = MatchState();
    
    Logger::instance().log(Logger::Info, "FMSHandler", "Match info reset");
}

void FMSHandler::logFMSEvent(const QString& event)
{
    Logger::instance().log(Logger::Info, "FMSHandler", 
                          QString("FMS Event: %1 (Match: %2, Phase: %3)")
                          .arg(event)
                          .arg(m_currentMatch.matchNumber)
                          .arg(m_currentState.phase));
}
