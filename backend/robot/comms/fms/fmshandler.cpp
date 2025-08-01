#include "fmshandler.h"
#include "backend/core/logger.h"
#include <QDataStream>
#include <QDateTime>

RobotFMSHandler::RobotFMSHandler(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_controlTimer(new QTimer(this))
    , m_connectionTimer(new QTimer(this))
    , m_connected(false)
    , m_teamNumber(0)
    , m_currentMode(Disabled)
    , m_enabled(false)
    , m_emergencyStop(false)
    , m_batteryVoltage(0)
    , m_robotAddress(QHostAddress("10.0.0.2"))
    , m_robotPort(1110)
    , m_localPort(1150)
{
    // Setup timers
    m_controlTimer->setInterval(CONTROL_INTERVAL);
    m_controlTimer->setSingleShot(false);
    connect(m_controlTimer, &QTimer::timeout, this, &RobotFMSHandler::sendControlPacket);
    
    m_connectionTimer->setInterval(CONNECTION_TIMEOUT);
    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout, this, &RobotFMSHandler::onConnectionTimeout);
    
    // Setup socket
    connect(m_socket, &QUdpSocket::readyRead, this, &RobotFMSHandler::processPendingDatagrams);
    
    Logger::instance().log(Logger::Info, "Robot FMS Handler initialized");
}

RobotFMSHandler::~RobotFMSHandler()
{
    disconnectFromRobot();
}

void RobotFMSHandler::connectToRobot()
{
    if (m_connected) {
        return;
    }
    
    if (!m_socket->bind(QHostAddress::Any, m_localPort)) {
        Logger::instance().log(Logger::Error, QString("Failed to bind robot FMS socket to port %1").arg(m_localPort));
        return;
    }
    
    m_controlTimer->start();
    m_connectionTimer->start();
    
    Logger::instance().log(Logger::Info, QString("Attempting to connect to robot at %1:%2")
                          .arg(m_robotAddress.toString()).arg(m_robotPort));
}

void RobotFMSHandler::disconnectFromRobot()
{
    m_controlTimer->stop();
    m_connectionTimer->stop();
    m_socket->close();
    
    if (m_connected) {
        updateConnectionStatus(false);
        Logger::instance().log(Logger::Info, "Disconnected from robot");
    }
}

void RobotFMSHandler::setTeamNumber(int teamNumber)
{
    if (m_teamNumber != teamNumber) {
        m_teamNumber = teamNumber;
        
        // Update robot address based on team number
        if (teamNumber > 0) {
            int firstOctet = teamNumber / 100;
            int secondOctet = teamNumber % 100;
            m_robotAddress = QHostAddress(QString("10.%1.%2.2").arg(firstOctet).arg(secondOctet));
        }
        
        Logger::instance().log(Logger::Info, QString("Team number set to %1, robot address: %2")
                              .arg(teamNumber).arg(m_robotAddress.toString()));
    }
}

void RobotFMSHandler::setRobotMode(RobotMode mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit robotModeChanged(mode);
        Logger::instance().log(Logger::Info, QString("Robot mode changed to %1").arg(mode));
    }
}

void RobotFMSHandler::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
        Logger::instance().log(Logger::Info, QString("Robot %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void RobotFMSHandler::setEmergencyStop(bool emergencyStop)
{
    if (m_emergencyStop != emergencyStop) {
        m_emergencyStop = emergencyStop;
        emit emergencyStopChanged(emergencyStop);
        Logger::instance().log(Logger::Warning, QString("Emergency stop %1").arg(emergencyStop ? "activated" : "deactivated"));
    }
}

void RobotFMSHandler::processPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        processStatusPacket(datagram.data());
    }
}

void RobotFMSHandler::sendControlPacket()
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Control packet format
    quint8 controlByte = 0;
    if (m_enabled) controlByte |= 0x01;
    if (m_currentMode == Autonomous) controlByte |= 0x02;
    if (m_currentMode == Test) controlByte |= 0x04;
    if (m_emergencyStop) controlByte |= 0x80;
    
    stream << quint16(0);                    // Packet number
    stream << quint8(controlByte);           // Control byte
    stream << quint8(0x00);                  // Request byte
    stream << quint16(m_teamNumber);         // Team number
    stream << quint8(0x01);                  // Alliance (red=0, blue=1)
    stream << quint8(0x01);                  // Position
    
    m_socket->writeDatagram(packet, m_robotAddress, m_robotPort);
}

void RobotFMSHandler::onConnectionTimeout()
{
    if (m_connected) {
        updateConnectionStatus(false);
        Logger::instance().log(Logger::Warning, "Robot connection timeout");
    }
}

void RobotFMSHandler::processStatusPacket(const QByteArray &data)
{
    if (data.size() < 8) {
        return; // Invalid packet size
    }
    
    // Reset connection timeout
    m_connectionTimer->start();
    
    if (!m_connected) {
        updateConnectionStatus(true);
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint16 packetNumber;
    quint8 statusByte;
    quint8 batteryHigh, batteryLow;
    
    stream >> packetNumber >> statusByte >> batteryHigh >> batteryLow;
    
    // Parse battery voltage (in 0.1V increments)
    int voltage = (batteryHigh << 8) | batteryLow;
    if (m_batteryVoltage != voltage) {
        m_batteryVoltage = voltage;
        emit batteryVoltageChanged(voltage);
    }
    
    emit robotDataReceived(data);
}

void RobotFMSHandler::updateConnectionStatus(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit connectionChanged(connected);
    }
}
