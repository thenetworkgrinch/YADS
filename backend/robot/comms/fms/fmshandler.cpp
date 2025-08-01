#include "fmshandler.h"
#include "../../../core/logger.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

RobotFMSHandler::RobotFMSHandler(QObject *parent)
    : QObject(parent)
    , m_commandSocket(new QUdpSocket(this))
    , m_statusTimer(new QTimer(this))
    , m_fmsConnected(false)
    , m_robotEnabled(false)
    , m_robotMode("Disabled")
    , m_fmsAddress(QHostAddress("10.0.100.5"))
    , m_commandPort(1750)
    , m_statusPort(1751)
{
    // Setup command socket
    connect(m_commandSocket, &QUdpSocket::readyRead, this, &RobotFMSHandler::processFMSCommand);
    
    // Setup status timer
    m_statusTimer->setInterval(100); // 10Hz status updates
    connect(m_statusTimer, &QTimer::timeout, this, &RobotFMSHandler::sendRobotStatus);
    
    // Bind to command port
    if (m_commandSocket->bind(QHostAddress::Any, m_commandPort)) {
        m_statusTimer->start();
        Logger::instance().log("Robot FMS handler initialized", Logger::Info);
    } else {
        Logger::instance().log("Failed to bind robot FMS command socket", Logger::Error);
    }
}

RobotFMSHandler::~RobotFMSHandler()
{
    m_statusTimer->stop();
    if (m_commandSocket->state() != QAbstractSocket::UnconnectedState) {
        m_commandSocket->close();
    }
}

void RobotFMSHandler::enableRobot()
{
    if (!m_robotEnabled) {
        m_robotEnabled = true;
        emit robotEnabledChanged(true);
        Logger::instance().log("Robot enabled via FMS", Logger::Info);
    }
}

void RobotFMSHandler::disableRobot()
{
    if (m_robotEnabled) {
        m_robotEnabled = false;
        emit robotEnabledChanged(false);
        Logger::instance().log("Robot disabled via FMS", Logger::Info);
    }
}

void RobotFMSHandler::setRobotMode(const QString &mode)
{
    if (m_robotMode != mode) {
        m_robotMode = mode;
        emit robotModeChanged(mode);
        Logger::instance().log("Robot mode changed to: " + mode, Logger::Info);
    }
}

void RobotFMSHandler::emergencyStop()
{
    disableRobot();
    setRobotMode("Emergency Stop");
    Logger::instance().log("Emergency stop activated via FMS", Logger::Critical);
}

void RobotFMSHandler::processFMSCommand()
{
    while (m_commandSocket->hasPendingDatagrams()) {
        QByteArray data;
        data.resize(m_commandSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        qint64 received = m_commandSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        if (received > 0) {
            parseCommand(data);
            
            if (!m_fmsConnected) {
                m_fmsConnected = true;
                emit fmsConnectedChanged(true);
            }
        }
    }
}

void RobotFMSHandler::parseCommand(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return; // Ignore malformed packets
    }

    QJsonObject obj = doc.object();
    QString command = obj["command"].toString();
    
    emit fmsCommandReceived(command);

    if (command == "enable") {
        QString mode = obj["mode"].toString("Teleop");
        updateRobotState(true, mode);
    } else if (command == "disable") {
        updateRobotState(false, "Disabled");
    } else if (command == "estop") {
        emergencyStop();
    } else if (command == "mode") {
        QString mode = obj["mode"].toString();
        if (!mode.isEmpty()) {
            setRobotMode(mode);
        }
    }
}

void RobotFMSHandler::updateRobotState(bool enabled, const QString &mode)
{
    bool stateChanged = false;
    
    if (m_robotEnabled != enabled) {
        m_robotEnabled = enabled;
        emit robotEnabledChanged(enabled);
        stateChanged = true;
    }
    
    if (m_robotMode != mode) {
        m_robotMode = mode;
        emit robotModeChanged(mode);
        stateChanged = true;
    }
    
    if (stateChanged) {
        Logger::instance().log(QString("Robot state updated: %1, %2")
                              .arg(enabled ? "Enabled" : "Disabled")
                              .arg(mode), Logger::Info);
    }
}

void RobotFMSHandler::sendRobotStatus()
{
    QJsonObject status;
    status["type"] = "robotStatus";
    status["enabled"] = m_robotEnabled;
    status["mode"] = m_robotMode;
    status["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(status);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    m_commandSocket->writeDatagram(data, m_fmsAddress, m_statusPort);
}
