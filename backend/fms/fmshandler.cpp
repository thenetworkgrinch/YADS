#include "fmshandler.h"
#include "../core/logger.h"
#include <QNetworkDatagram>
#include <QDataStream>
#include <QDebug>
#include <QNetworkInterface>

using namespace FRCDriverStation;

FMSHandler::FMSHandler(std::shared_ptr<Logger> logger, QObject *parent)
    : QObject(parent)
    , m_logger(logger)
    , m_detectionSocket(std::make_unique<QUdpSocket>(this))
    , m_fmsSocket(std::make_unique<QTcpSocket>(this))
    , m_detectionTimer(std::make_unique<QTimer>(this))
    , m_heartbeatTimer(std::make_unique<QTimer>(this))
    , m_currentState(Disconnected)
    , m_lastContactTime(0)
    , m_currentMatchNumber(-1)
{
    // Setup detection timer
    m_detectionTimer->setInterval(DETECTION_INTERVAL_MS);
    connect(m_detectionTimer.get(), &QTimer::timeout, this, &FMSHandler::onDetectionTimerTimeout);
    
    // Setup heartbeat timer
    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL_MS);
    connect(m_heartbeatTimer.get(), &QTimer::timeout, this, &FMSHandler::onHeartbeatTimerTimeout);
    
    // Setup FMS TCP socket
    connect(m_fmsSocket.get(), &QTcpSocket::connected, this, &FMSHandler::onFmsTcpConnected);
    connect(m_fmsSocket.get(), &QTcpSocket::disconnected, this, &FMSHandler::onFmsTcpDisconnected);
    connect(m_fmsSocket.get(), &QTcpSocket::readyRead, this, &FMSHandler::onFmsDataReceived);
    connect(m_fmsSocket.get(), QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &FMSHandler::onFmsTcpError);
    
    // Setup detection UDP socket
    connect(m_detectionSocket.get(), &QUdpSocket::readyRead, this, [this]() {
        while (m_detectionSocket->hasPendingDatagrams()) {
            QNetworkDatagram datagram = m_detectionSocket->receiveDatagram();
            
            // Check if this looks like an FMS response
            if (datagram.data().size() >= 4) {
                QDataStream stream(datagram.data());
                stream.setByteOrder(QDataStream::BigEndian);
                quint32 magic;
                stream >> magic;
                
                if (magic == 0x464D5300) { // "FMS\0" magic number
                    QHostAddress fmsAddr = datagram.senderAddress();
                    m_logger->info("FMS Detection", "FMS detected via UDP broadcast", fmsAddr.toString());
                    connectToFMS(fmsAddr);
                }
            }
        }
    });
    
    m_logger->info("FMS Handler", "FMS handler initialized");
}

FMSHandler::~FMSHandler()
{
    stopMonitoring();
}

void FMSHandler::startMonitoring()
{
    if (m_currentState != Disconnected) {
        return;
    }
    
    updateState(Detecting);
    
    // Bind detection socket to listen for FMS broadcasts
    if (!m_detectionSocket->bind(FMS_DETECTION_PORT, QUdpSocket::ShareAddress)) {
        m_logger->error("FMS Detection", "Failed to bind detection socket", 
                       QString("Port %1: %2").arg(FMS_DETECTION_PORT).arg(m_detectionSocket->errorString()));
        updateState(Error);
        return;
    }
    
    m_detectionTimer->start();
    m_logger->info("FMS Handler", "Started monitoring for FMS connection");
}

void FMSHandler::stopMonitoring()
{
    m_detectionTimer->stop();
    m_heartbeatTimer->stop();
    
    if (m_fmsSocket->state() != QAbstractSocket::UnconnectedState) {
        m_fmsSocket->disconnectFromHost();
    }
    
    if (m_detectionSocket->state() == QAbstractSocket::BoundState) {
        m_detectionSocket->close();
    }
    
    if (m_currentState != Disconnected) {
        updateState(Disconnected);
    }
    
    m_logger->info("FMS Handler", "Stopped monitoring for FMS connection");
}

qint64 FMSHandler::timeSinceLastContact() const
{
    if (m_lastContactTime == 0) {
        return -1;
    }
    return QDateTime::currentMSecsSinceEpoch() - m_lastContactTime;
}

void FMSHandler::onDetectionTimerTimeout()
{
    if (m_currentState == Detecting) {
        detectFMS();
    } else if (m_currentState == Connected) {
        // Check if we've lost contact with FMS
        qint64 timeSinceContact = timeSinceLastContact();
        if (timeSinceContact > FMS_TIMEOUT_MS) {
            m_logger->warning("FMS Handler", "FMS connection timeout", 
                            QString("No contact for %1ms").arg(timeSinceContact));
            disconnectFromFMS();
        }
    }
}

void FMSHandler::onHeartbeatTimerTimeout()
{
    if (m_currentState == Connected) {
        sendHeartbeat();
    }
}

void FMSHandler::onFmsDataReceived()
{
    QByteArray data = m_fmsSocket->readAll();
    m_lastContactTime = QDateTime::currentMSecsSinceEpoch();
    
    parseFMSPacket(data);
}

void FMSHandler::onFmsTcpConnected()
{
    m_logger->info("FMS Handler", "TCP connection to FMS established", m_fmsAddress.toString());
    updateState(Connected);
    m_lastContactTime = QDateTime::currentMSecsSinceEpoch();
    m_heartbeatTimer->start();
    
    emit fmsConnected(m_fmsAddress);
}

void FMSHandler::onFmsTcpDisconnected()
{
    m_logger->info("FMS Handler", "TCP connection to FMS lost");
    disconnectFromFMS();
}

void FMSHandler::onFmsTcpError()
{
    QString error = m_fmsSocket->errorString();
    m_logger->error("FMS Handler", "FMS TCP connection error", error);
    
    if (m_currentState == Connected) {
        disconnectFromFMS();
    } else {
        updateState(Detecting);
    }
}

void FMSHandler::detectFMS()
{
    // Try to find FMS on the network
    QHostAddress fmsAddr = findFMSAddress();
    
    if (!fmsAddr.isNull()) {
        connectToFMS(fmsAddr);
    } else {
        // Send UDP broadcast to detect FMS
        QByteArray packet;
        QDataStream stream(&packet, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << quint32(0x44535300); // "DS\0\0" magic number
        stream << quint32(QDateTime::currentMSecsSinceEpoch() / 1000); // Timestamp
        
        // Broadcast to common FMS addresses
        QList<QHostAddress> broadcastAddresses;
        broadcastAddresses << QHostAddress("10.0.100.5");  // Standard FMS address
        broadcastAddresses << QHostAddress("192.168.1.20"); // Backup FMS address
        
        for (const QHostAddress &addr : broadcastAddresses) {
            m_detectionSocket->writeDatagram(packet, addr, FMS_DETECTION_PORT);
        }
    }
}

void FMSHandler::connectToFMS(const QHostAddress &address)
{
    if (m_currentState == Connected && m_fmsAddress == address) {
        return; // Already connected to this FMS
    }
    
    m_fmsAddress = address;
    m_logger->info("FMS Handler", "Attempting to connect to FMS", address.toString());
    
    if (m_fmsSocket->state() != QAbstractSocket::UnconnectedState) {
        m_fmsSocket->disconnectFromHost();
        m_fmsSocket->waitForDisconnected(1000);
    }
    
    m_fmsSocket->connectToHost(address, FMS_DATA_PORT);
}

void FMSHandler::disconnectFromFMS()
{
    m_heartbeatTimer->stop();
    
    if (m_fmsSocket->state() != QAbstractSocket::UnconnectedState) {
        m_fmsSocket->disconnectFromHost();
    }
    
    m_currentMatchNumber = -1;
    m_currentMatchType.clear();
    m_fmsAddress = QHostAddress();
    m_lastContactTime = 0;
    
    updateState(Detecting);
    emit fmsDisconnected();
}

void FMSHandler::updateState(FMSState newState)
{
    if (m_currentState != newState) {
        FMSState oldState = m_currentState;
        m_currentState = newState;
        
        QStringList stateNames = {"Disconnected", "Detecting", "Connected", "Error"};
        m_logger->info("FMS Handler", "State changed", 
                      QString("From %1 to %2").arg(stateNames[oldState]).arg(stateNames[newState]));
        
        emit stateChanged(newState, m_fmsAddress);
    }
}

void FMSHandler::parseFMSPacket(const QByteArray &data)
{
    if (data.size() < 8) {
        return; // Packet too small
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint32 packetType;
    stream >> packetType;
    
    switch (packetType) {
        case 0x464D5301: { // FMS match info packet
            quint16 matchNumber;
            quint8 matchType;
            quint8 flags;
            
            stream >> matchNumber >> matchType >> flags;
            
            if (m_currentMatchNumber != static_cast<int>(matchNumber)) {
                m_currentMatchNumber = matchNumber;
                
                QStringList matchTypes = {"Practice", "Qualification", "Playoff", "Final"};
                m_currentMatchType = (matchType < matchTypes.size()) ? 
                                   matchTypes[matchType] : "Unknown";
                
                m_logger->info("FMS Match", "Match info received", 
                              QString("Match %1 (%2)").arg(matchNumber).arg(m_currentMatchType));
                
                emit matchInfoReceived(m_currentMatchNumber, m_currentMatchType);
            }
            break;
        }
        
        case 0x464D5302: { // FMS control packet
            quint8 controlFlags;
            stream >> controlFlags;
            
            bool enabled = (controlFlags & 0x01) != 0;
            bool autonomous = (controlFlags & 0x02) != 0;
            bool test = (controlFlags & 0x04) != 0;
            
            emit matchControlReceived(enabled, autonomous, test);
            break;
        }
        
        default:
            // Unknown packet type, ignore
            break;
    }
}

void FMSHandler::sendHeartbeat()
{
    if (m_fmsSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << quint32(0x44535301); // DS heartbeat packet type
    stream << quint32(QDateTime::currentMSecsSinceEpoch() / 1000); // Timestamp
    
    m_fmsSocket->write(packet);
}

QHostAddress FMSHandler::findFMSAddress()
{
    // Check common FMS IP addresses
    QList<QHostAddress> commonAddresses;
    commonAddresses << QHostAddress("10.0.100.5");   // Standard FMS
    commonAddresses << QHostAddress("192.168.1.20"); // Backup FMS
    
    for (const QHostAddress &addr : commonAddresses) {
        QTcpSocket testSocket;
        testSocket.connectToHost(addr, FMS_DATA_PORT);
        
        if (testSocket.waitForConnected(500)) { // 500ms timeout
            testSocket.disconnectFromHost();
            return addr;
        }
    }
    
    return QHostAddress(); // Not found
}
