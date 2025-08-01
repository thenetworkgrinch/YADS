#include "communicationhandler.h"
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QApplication>
#include <QMutexLocker>
#include <QRandomGenerator>

// FRC networking constants
static const quint16 DS_TO_ROBOT_PORT = 1110;
static const quint16 ROBOT_TO_DS_PORT = 1150;
static const int DISCOVERY_TIMEOUT_MS = 5000;
static const int HEARTBEAT_INTERVAL_MS = 100;
static const int CONTROL_PACKET_INTERVAL_MS = 20;
static const int PING_INTERVAL_MS = 1000;
static const int CONNECTION_TIMEOUT_MS = 3000;

CommunicationHandler::CommunicationHandler(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_robotPort(DS_TO_ROBOT_PORT)
    , m_localPort(ROBOT_TO_DS_PORT)
    , m_teamNumber(0)
    , m_robotIpAddress("")
    , m_connectionMode(TeamNumber)
    , m_connectionState(Disconnected)
    , m_discoveryTimer(nullptr)
    , m_heartbeatTimer(nullptr)
    , m_controlPacketTimer(nullptr)
    , m_pingTimer(nullptr)
    , m_robotEnabled(false)
    , m_robotMode(0)
    , m_alliance(0)
    , m_position(0)
    , m_emergencyStop(false)
    , m_sequenceNumber(0)
    , m_packetsSent(0)
    , m_packetsReceived(0)
    , m_packetsLost(0)
    , m_pingLatency(-1)
{
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Initializing communication handler");
    
    // Initialize joystick data
    for (int i = 0; i < 6; ++i) {
        m_joysticks[i].clear();
    }
    
    initializeSocket();
    setupTimers();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Communication handler initialized");
}

CommunicationHandler::~CommunicationHandler()
{
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Shutting down communication handler");
    
    disconnectFromRobot();
    
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Communication handler shutdown complete");
}

void CommunicationHandler::initializeSocket()
{
    m_socket = new QUdpSocket(this);
    
    connect(m_socket, &QUdpSocket::readyRead,
            this, &CommunicationHandler::onSocketReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
            this, &CommunicationHandler::onSocketError);
    
    // Bind to local port
    if (!m_socket->bind(QHostAddress::Any, m_localPort)) {
        Logger::instance().log(Logger::Error, "CommunicationHandler", 
                              QString("Failed to bind to port %1: %2")
                              .arg(m_localPort)
                              .arg(m_socket->errorString()));
    } else {
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              QString("Socket bound to port %1").arg(m_localPort));
    }
}

void CommunicationHandler::setupTimers()
{
    // Discovery timer
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(DISCOVERY_TIMEOUT_MS);
    connect(m_discoveryTimer, &QTimer::timeout,
            this, &CommunicationHandler::onDiscoveryTimeout);
    
    // Heartbeat timer
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL_MS);
    connect(m_heartbeatTimer, &QTimer::timeout,
            this, &CommunicationHandler::onHeartbeatTimeout);
    
    // Control packet timer
    m_controlPacketTimer = new QTimer(this);
    m_controlPacketTimer->setInterval(CONTROL_PACKET_INTERVAL_MS);
    connect(m_controlPacketTimer, &QTimer::timeout,
            this, &CommunicationHandler::onSendControlPacket);
    
    // Ping timer
    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(PING_INTERVAL_MS);
    connect(m_pingTimer, &QTimer::timeout,
            this, &CommunicationHandler::onPingTimeout);
}

void CommunicationHandler::setTeamNumber(int teamNumber)
{
    if (m_teamNumber != teamNumber) {
        QMutexLocker locker(&m_stateMutex);
        m_teamNumber = teamNumber;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              QString("Team number set to: %1").arg(teamNumber));
        
        // If we're currently connected, restart communication with new team number
        if (m_connectionState == Connected && m_connectionMode == TeamNumber) {
            restartCommunication();
        }
    }
}

void CommunicationHandler::setRobotIpAddress(const QString& ipAddress)
{
    if (m_robotIpAddress != ipAddress) {
        QMutexLocker locker(&m_stateMutex);
        m_robotIpAddress = ipAddress;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              QString("Robot IP address set to: %1").arg(ipAddress));
        
        // If we're currently connected, restart communication with new IP address
        if (m_connectionState == Connected && m_connectionMode == IpAddress) {
            restartCommunication();
        }
    }
}

void CommunicationHandler::setConnectionMode(ConnectionMode mode)
{
    if (m_connectionMode != mode) {
        QMutexLocker locker(&m_stateMutex);
        m_connectionMode = mode;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              QString("Connection mode set to: %1")
                              .arg(mode == TeamNumber ? "Team Number" : "IP Address"));
        
        // If we're currently connected, restart communication with new mode
        if (m_connectionState == Connected) {
            restartCommunication();
        }
    }
}

void CommunicationHandler::connectToRobot()
{
    if (m_connectionMode == TeamNumber && m_teamNumber == 0) {
        Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                              "Cannot connect - team number not set");
        emit communicationError("Team number not set");
        return;
    }
    
    if (m_connectionMode == IpAddress && m_robotIpAddress.isEmpty()) {
        Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                              "Cannot connect - IP address not set");
        emit communicationError("IP address not set");
        return;
    }
    
    if (m_connectionState == Connected || m_connectionState == Connecting) {
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              "Already connected or connecting");
        return;
    }
    
    QString target = (m_connectionMode == TeamNumber) ? 
        QString("team %1").arg(m_teamNumber) : m_robotIpAddress;
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Starting connection to %1").arg(target));
    
    updateConnectionState(Discovering);
    startRobotDiscovery();
}

void CommunicationHandler::disconnectFromRobot()
{
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Disconnecting from robot");
    
    // Stop all timers
    if (m_discoveryTimer) m_discoveryTimer->stop();
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    if (m_controlPacketTimer) m_controlPacketTimer->stop();
    if (m_pingTimer) m_pingTimer->stop();
    
    // Reset state
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = false;
    m_emergencyStop = false;
    m_robotAddress = QHostAddress();
    locker.unlock();
    
    updateConnectionState(Disconnected);
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Disconnected from robot");
}

void CommunicationHandler::restartCommunication()
{
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Restarting communication");
    
    disconnectFromRobot();
    
    // Wait a moment before reconnecting
    QTimer::singleShot(1000, this, &CommunicationHandler::connectToRobot);
}

void CommunicationHandler::startRobotDiscovery()
{
    QString target = (m_connectionMode == TeamNumber) ? 
        QString("team %1").arg(m_teamNumber) : m_robotIpAddress;
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Starting robot discovery for %1").arg(target));
    
    updateConnectionState(Discovering);
    
    // Generate possible robot addresses
    QList<QHostAddress> addresses;
    if (m_connectionMode == TeamNumber) {
        addresses = generateRobotAddresses();
    } else {
        // Direct IP address mode
        QHostAddress directAddress(m_robotIpAddress);
        if (!directAddress.isNull()) {
            addresses.append(directAddress);
        }
    }
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Trying %1 possible robot addresses").arg(addresses.size()));
    
    // Try each address
    bool foundRobot = false;
    for (const QHostAddress& address : addresses) {
        if (pingRobotAddress(address)) {
            Logger::instance().log(Logger::Info, "CommunicationHandler", 
                                  QString("Found robot at %1").arg(address.toString()));
            
            QMutexLocker locker(&m_stateMutex);
            m_robotAddress = address;
            m_connectionStartTime = QDateTime::currentDateTime();
            locker.unlock();
            
            updateConnectionState(Connected);
            emit robotAddressChanged(address);
            emit robotConnected();
            
            // Start communication timers
            m_heartbeatTimer->start();
            m_controlPacketTimer->start();
            m_pingTimer->start();
            
            foundRobot = true;
            break;
        }
    }
    
    if (!foundRobot) {
        // Start discovery timeout
        m_discoveryTimer->start();
    }
}

void CommunicationHandler::stopRobotDiscovery()
{
    if (m_discoveryTimer) {
        m_discoveryTimer->stop();
    }
}

QList<QHostAddress> CommunicationHandler::generateRobotAddresses() const
{
    QList<QHostAddress> addresses;
    
    if (m_teamNumber == 0) {
        return addresses;
    }
    
    // Standard FRC robot IP addresses
    // Format: 10.TE.AM.2 where TEAM is the team number
    int teamHigh = m_teamNumber / 100;
    int teamLow = m_teamNumber % 100;
    
    // Primary address: 10.TE.AM.2
    QString primaryIP = QString("10.%1.%2.2").arg(teamHigh).arg(teamLow);
    addresses.append(QHostAddress(primaryIP));
    
    // Secondary address: 172.22.11.2 (USB connection)
    addresses.append(QHostAddress("172.22.11.2"));
    
    // Tertiary address: 192.168.1.2 (Ethernet bridge)
    addresses.append(QHostAddress("192.168.1.2"));
    
    // mDNS address: roboRIO-TEAM-FRC.local
    QString mdnsName = QString("roboRIO-%1-FRC.local").arg(m_teamNumber);
    QHostInfo hostInfo = QHostInfo::fromName(mdnsName);
    if (hostInfo.error() == QHostInfo::NoError && !hostInfo.addresses().isEmpty()) {
        addresses.append(hostInfo.addresses().first());
    }
    
    return addresses;
}

bool CommunicationHandler::pingRobotAddress(const QHostAddress& address)
{
    // Send a heartbeat packet to test connectivity
    QByteArray heartbeat = createHeartbeatPacket();
    
    qint64 bytesWritten = m_socket->writeDatagram(heartbeat, address, m_robotPort);
    
    if (bytesWritten == -1) {
        Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                              QString("Failed to ping %1: %2")
                              .arg(address.toString())
                              .arg(m_socket->errorString()));
        return false;
    }
    
    // Wait for response (simplified - in real implementation would be async)
    QApplication::processEvents();
    QThread::msleep(100);
    
    // Check if we received any data (simplified check)
    return m_socket->hasPendingDatagrams();
}

QByteArray CommunicationHandler::createHeartbeatPacket() const
{
    QByteArray packet;
    packet.resize(8);
    
    // Simple heartbeat packet format
    packet[0] = 0x00; // Packet type: heartbeat
    packet[1] = 0x00; // Reserved
    
    // Timestamp (4 bytes)
    quint32 timestamp = QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF;
    packet[2] = (timestamp >> 24) & 0xFF;
    packet[3] = (timestamp >> 16) & 0xFF;
    packet[4] = (timestamp >> 8) & 0xFF;
    packet[5] = timestamp & 0xFF;
    
    // Sequence number (2 bytes)
    packet[6] = (m_sequenceNumber >> 8) & 0xFF;
    packet[7] = m_sequenceNumber & 0xFF;
    
    return packet;
}

void CommunicationHandler::onSocketReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        
        if (datagram.isValid()) {
            QMutexLocker locker(&m_stateMutex);
            m_packetsReceived++;
            m_lastPacketTime = QDateTime::currentDateTime();
            locker.unlock();
            
            processReceivedPacket(datagram.data(), datagram.senderAddress());
            emit packetReceived(datagram.data());
        }
    }
}

void CommunicationHandler::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    Logger::instance().log(Logger::Error, "CommunicationHandler", 
                          QString("Socket error: %1").arg(errorString));
    
    emit communicationError(errorString);
    
    if (m_connectionState == Connected) {
        updateConnectionState(ConnectionLost);
    }
}

void CommunicationHandler::onDiscoveryTimeout()
{
    Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                          "Robot discovery timeout - no robot found");
    
    updateConnectionState(Disconnected);
    emit communicationError("Robot not found - discovery timeout");
}

void CommunicationHandler::onHeartbeatTimeout()
{
    if (m_connectionState == Connected) {
        sendHeartbeatPacket();
        
        // Check for connection timeout
        QMutexLocker locker(&m_stateMutex);
        if (m_lastPacketTime.isValid() && 
            m_lastPacketTime.msecsTo(QDateTime::currentDateTime()) > CONNECTION_TIMEOUT_MS) {
            locker.unlock();
            
            Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                                  "Connection timeout - no packets received");
            updateConnectionState(ConnectionLost);
            emit communicationError("Connection timeout");
        }
    }
}

void CommunicationHandler::onSendControlPacket()
{
    if (m_connectionState == Connected) {
        sendControlPacket();
    }
}

void CommunicationHandler::onPingTimeout()
{
    if (m_connectionState == Connected) {
        sendPingPacket();
    }
}

void CommunicationHandler::processReceivedPacket(const QByteArray& packet, const QHostAddress& sender)
{
    if (packet.size() < 2) {
        Logger::instance().log(Logger::Warning, "CommunicationHandler", 
                              "Received invalid packet - too small");
        return;
    }
    
    quint8 packetType = static_cast<quint8>(packet[0]);
    
    switch (packetType) {
        case 0x01: // Status packet
            handleStatusPacket(packet);
            break;
        case 0x02: // Console packet
            handleConsolePacket(packet);
            break;
        case 0x03: // Heartbeat response
            handleHeartbeatPacket(packet);
            break;
        default:
            Logger::instance().log(Logger::Debug, "CommunicationHandler", 
                                  QString("Received packet type: 0x%1")
                                  .arg(packetType, 2, 16, QChar('0')));
            break;
    }
}

void CommunicationHandler::handleStatusPacket(const QByteArray& packet)
{
    if (packet.size() < 10) {
        return;
    }
    
    // Parse status packet
    bool wasEnabled = m_robotEnabled;
    
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = (packet[2] & 0x01) != 0;
    locker.unlock();
    
    // Extract battery voltage (bytes 3-4, as 16-bit fixed point)
    quint16 voltageRaw = (static_cast<quint16>(packet[3]) << 8) | static_cast<quint16>(packet[4]);
    double voltage = voltageRaw / 256.0; // Convert from 8.8 fixed point
    
    // Emit signals for state changes
    if (wasEnabled != m_robotEnabled) {
        if (m_robotEnabled) {
            emit robotEnabled();
        } else {
            emit robotDisabled();
        }
    }
    
    emit batteryVoltageChanged(voltage);
    emit robotStatusChanged(packet);
    
    Logger::instance().log(Logger::Debug, "CommunicationHandler", 
                          QString("Status: enabled=%1, battery=%2V")
                          .arg(m_robotEnabled ? "true" : "false")
                          .arg(voltage, 0, 'f', 2));
}

void CommunicationHandler::handleConsolePacket(const QByteArray& packet)
{
    if (packet.size() < 3) {
        return;
    }
    
    // Extract message (skip packet type and length bytes)
    QString message = QString::fromUtf8(packet.mid(2));
    QDateTime timestamp = QDateTime::currentDateTime();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Console: %1").arg(message));
    
    emit consoleMessageReceived(message, timestamp);
}

void CommunicationHandler::handleHeartbeatPacket(const QByteArray& packet)
{
    if (packet.size() < 8) {
        return;
    }
    
    // Calculate ping latency
    if (m_lastPingTime.isValid()) {
        int latency = m_lastPingTime.msecsTo(QDateTime::currentDateTime());
        
        QMutexLocker locker(&m_stateMutex);
        m_pingLatency = latency;
        locker.unlock();
        
        emit pingLatencyChanged(latency);
    }
    
    Logger::instance().log(Logger::Debug, "CommunicationHandler", 
                          QString("Heartbeat received (latency: %1ms)").arg(m_pingLatency));
}

QByteArray CommunicationHandler::createControlPacket() const
{
    QMutexLocker locker(&m_stateMutex);
    
    QByteArray packet;
    packet.resize(32); // Standard control packet size
    
    // Packet header
    packet[0] = 0x10; // Control packet type
    packet[1] = 0x00; // Reserved
    
    // Sequence number
    packet[2] = (m_sequenceNumber >> 8) & 0xFF;
    packet[3] = m_sequenceNumber & 0xFF;
    
    // Control flags
    quint8 flags = 0;
    if (m_robotEnabled && !m_emergencyStop) flags |= 0x01; // Enabled
    if (m_robotMode == 1) flags |= 0x02; // Autonomous
    if (m_robotMode == 3) flags |= 0x04; // Test
    if (m_emergencyStop) flags |= 0x08; // Emergency stop
    flags |= 0x10; // DS attached
    
    packet[4] = flags;
    
    // Alliance and position
    packet[5] = m_alliance;
    packet[6] = m_position;
    
    // Reserved bytes
    packet[7] = 0x00;
    
    // Joystick data (simplified - would normally include full joystick state)
    for (int i = 0; i < 6 && i < 24; ++i) {
        packet[8 + i] = (i < m_joysticks->size()) ? m_joysticks[0][i] : 0;
    }
    
    return packet;
}

void CommunicationHandler::sendControlPacket()
{
    QByteArray packet = createControlPacket();
    sendPacket(packet);
    
    QMutexLocker locker(&m_stateMutex);
    m_sequenceNumber++;
}

void CommunicationHandler::sendHeartbeatPacket()
{
    QByteArray packet = createHeartbeatPacket();
    sendPacket(packet);
}

void CommunicationHandler::sendPingPacket()
{
    QMutexLocker locker(&m_stateMutex);
    m_lastPingTime = QDateTime::currentDateTime();
    locker.unlock();
    
    sendHeartbeatPacket(); // Use heartbeat as ping
}

void CommunicationHandler::sendPacket(const QByteArray& packet)
{
    if (m_connectionState != Connected || m_robotAddress.isNull()) {
        return;
    }
    
    qint64 bytesWritten = m_socket->writeDatagram(packet, m_robotAddress, m_robotPort);
    
    if (bytesWritten == -1) {
        Logger::instance().log(Logger::Error, "CommunicationHandler", 
                              QString("Failed to send packet: %1").arg(m_socket->errorString()));
        emit communicationError(m_socket->errorString());
    } else {
        QMutexLocker locker(&m_stateMutex);
        m_packetsSent++;
        locker.unlock();
        
        emit packetSent(packet);
    }
}

void CommunicationHandler::enableRobot()
{
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = true;
    m_emergencyStop = false;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Robot enabled");
}

void CommunicationHandler::disableRobot()
{
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = false;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", "Robot disabled");
}

void CommunicationHandler::emergencyStop()
{
    QMutexLocker locker(&m_stateMutex);
    m_robotEnabled = false;
    m_emergencyStop = true;
    locker.unlock();
    
    Logger::instance().log(Logger::Error, "CommunicationHandler", "EMERGENCY STOP");
}

void CommunicationHandler::setRobotMode(int mode)
{
    QMutexLocker locker(&m_stateMutex);
    m_robotMode = mode;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Robot mode set to: %1").arg(mode));
    
    emit robotModeChanged(mode);
}

void CommunicationHandler::setAlliance(int alliance, int position)
{
    QMutexLocker locker(&m_stateMutex);
    m_alliance = alliance;
    m_position = position;
    locker.unlock();
    
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Alliance set to: %1 %2").arg(alliance).arg(position));
}

void CommunicationHandler::updateJoystickData(int joystickIndex, const QByteArray& data)
{
    if (joystickIndex >= 0 && joystickIndex < 6) {
        QMutexLocker locker(&m_stateMutex);
        m_joysticks[joystickIndex] = data;
    }
}

void CommunicationHandler::updateConnectionState(ConnectionState newState)
{
    if (m_connectionState != newState) {
        ConnectionState oldState = m_connectionState;
        
        QMutexLocker locker(&m_stateMutex);
        m_connectionState = newState;
        locker.unlock();
        
        Logger::instance().log(Logger::Info, "CommunicationHandler", 
                              QString("Connection state changed: %1 -> %2")
                              .arg(connectionStateToString(oldState))
                              .arg(connectionStateToString(newState)));
        
        emit connectionStateChanged(newState);
        
        // Handle state-specific actions
        switch (newState) {
            case Connected:
                resetStatistics();
                break;
            case Disconnected:
            case ConnectionLost:
                emit robotDisconnected();
                break;
            default:
                break;
        }
    }
}

QString CommunicationHandler::connectionStateToString(ConnectionState state) const
{
    switch (state) {
        case Disconnected: return "Disconnected";
        case Discovering: return "Discovering";
        case Connecting: return "Connecting";
        case Connected: return "Connected";
        case ConnectionLost: return "ConnectionLost";
        default: return "Unknown";
    }
}

double CommunicationHandler::packetLossRate() const
{
    QMutexLocker locker(&m_stateMutex);
    if (m_packetsSent == 0) {
        return 0.0;
    }
    return static_cast<double>(m_packetsLost) / m_packetsSent * 100.0;
}

void CommunicationHandler::logPacketStatistics()
{
    QMutexLocker locker(&m_stateMutex);
    Logger::instance().log(Logger::Info, "CommunicationHandler", 
                          QString("Packet statistics - Sent: %1, Received: %2, Lost: %3, Loss rate: %4%")
                          .arg(m_packetsSent)
                          .arg(m_packetsReceived)
                          .arg(m_packetsLost)
                          .arg(packetLossRate(), 0, 'f', 2));
}

void CommunicationHandler::resetStatistics()
{
    QMutexLocker locker(&m_stateMutex);
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_packetsLost = 0;
    m_pingLatency = -1;
}

QHostAddress CommunicationHandler::getLocalAddress() const
{
    // Get the first non-loopback IPv4 address
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && 
            !address.isLoopback() && 
            address != QHostAddress::LocalHost) {
            return address;
        }
    }
    return QHostAddress::LocalHost;
}

bool CommunicationHandler::isValidRobotAddress(const QHostAddress& address) const
{
    // Check if address is in valid FRC ranges
    quint32 addr = address.toIPv4Address();
    
    // 10.x.x.x range
    if ((addr & 0xFF000000) == 0x0A000000) {
        return true;
    }
    
    // 172.22.11.x range (USB)
    if ((addr & 0xFFFFFF00) == 0xAC160B00) {
        return true;
    }
    
    // 192.168.1.x range (Ethernet bridge)
    if ((addr & 0xFFFFFF00) == 0xC0A80100) {
        return true;
    }
    
    return false;
}

quint16 CommunicationHandler::calculateTeamPort(int teamNumber) const
{
    // Standard FRC port calculation (if needed for future use)
    return DS_TO_ROBOT_PORT + (teamNumber % 100);
}
