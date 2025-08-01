#include "communicationhandler.h"
#include "../../robotstate.h"
#include "../../controllers/controllerhidhandler.h"
#include "../../core/logger.h"
#include "../../core/constants.h"
#include <QDataStream>
#include <QNetworkDatagram>
#include <QDebug>
#include <QElapsedTimer>
#include <QDir>
#include <QRegularExpression>

using namespace FRCDriverStation;
using namespace FRCDriverStation::Constants;
using namespace FRCDriverStation::Protocol;

CommunicationHandler::CommunicationHandler(RobotState *robotState, 
                                          ControllerHIDHandler *controllerHandler,
                                          std::shared_ptr<Logger> logger,
                                          QObject *parent)
    : QObject(parent)
    , m_udpSendSocket(std::make_unique<QUdpSocket>(this))
    , m_udpReceiveSocket(std::make_unique<QUdpSocket>(this))
    , m_tcpConsoleSocket(std::make_unique<QTcpSocket>(this))
    , m_networkTablesSocket(std::make_unique<QTcpSocket>(this))
    , m_sendTimer(std::make_unique<QTimer>(this))
    , m_consoleReconnectTimer(std::make_unique<QTimer>(this))
    , m_watchdogTimer(std::make_unique<QTimer>(this))
    , m_pingTimer(std::make_unique<QTimer>(this))
    , m_networkStatsTimer(std::make_unique<QTimer>(this))
    , m_networkTablesTimer(std::make_unique<QTimer>(this))
    , m_robotState(robotState)
    , m_controllerHandler(controllerHandler)
    , m_logger(logger)
    , m_packetCounter(0)
    , m_lastPacketTime(0)
    , m_robotConnected(false)
    , m_consoleConnected(false)
    , m_networkManager(std::make_unique<QNetworkAccessManager>(this))
    , m_logDownloadReply(nullptr)
    , m_currentFileIndex(0)
    , m_totalFiles(0)
{
    // Bind UDP receive socket
    m_udpReceiveSocket->bind(Network::ROBOT_TO_DS_PORT, QUdpSocket::ShareAddress);
    connect(m_udpReceiveSocket.get(), &QUdpSocket::readyRead, this, &CommunicationHandler::readRobotPacket);

    // Setup TCP console socket
    connect(m_tcpConsoleSocket.get(), &QTcpSocket::readyRead, this, &CommunicationHandler::readConsoleData);
    connect(m_tcpConsoleSocket.get(), &QTcpSocket::connected, this, &CommunicationHandler::onConsoleConnected);
    connect(m_tcpConsoleSocket.get(), &QTcpSocket::disconnected, this, &CommunicationHandler::onConsoleDisconnected);
    connect(m_tcpConsoleSocket.get(), QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &CommunicationHandler::onConsoleError);

    // Setup control packet timer (50Hz)
    m_sendTimer->setInterval(Network::HEARTBEAT_INTERVAL_MS);
    connect(m_sendTimer.get(), &QTimer::timeout, this, &CommunicationHandler::sendControlPacket);
    m_sendTimer->start();

    // Setup console reconnection timer
    m_consoleReconnectTimer->setSingleShot(true);
    connect(m_consoleReconnectTimer.get(), &QTimer::timeout, this, &CommunicationHandler::connectToConsole);

    // Setup watchdog timer
    m_watchdogTimer->setInterval(1000);
    connect(m_watchdogTimer.get(), &QTimer::timeout, this, &CommunicationHandler::updateConnectionStatus);
    m_watchdogTimer->start();

    // Setup network diagnostics timers
    m_pingTimer->setInterval(1000);
    connect(m_pingTimer.get(), &QTimer::timeout, this, &CommunicationHandler::sendPing);
    m_pingTimer->start();

    m_networkStatsTimer->setInterval(5000);
    connect(m_networkStatsTimer.get(), &QTimer::timeout, this, &CommunicationHandler::updateNetworkStats);
    m_networkStatsTimer->start();

    // Setup NetworkTables monitoring
    m_networkTablesTimer->setInterval(2000);
    connect(m_networkTablesTimer.get(), &QTimer::timeout, this, &CommunicationHandler::checkNetworkTablesConnection);
    m_networkTablesTimer->start();

    // Connect to robot state changes
    connect(m_robotState, &RobotState::teamNumberChanged, this, &CommunicationHandler::updateTeamNumber);
    connect(m_robotState, &RobotState::controlDataChanged, this, &CommunicationHandler::sendControlPacket);
    connect(m_robotState, &RobotState::robotRebootRequested, this, &CommunicationHandler::sendRebootCommand);
    connect(m_robotState, &RobotState::robotCodeRestartRequested, this, &CommunicationHandler::sendRestartCodeCommand);
    connect(m_robotState, &RobotState::logDownloadRequested, this, &CommunicationHandler::downloadLogs);

    // Initialize network diagnostics
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_totalLatency = 0.0;
    m_latencyCount = 0;
    m_lastBandwidthTime = QDateTime::currentMSecsSinceEpoch();
    m_lastBandwidthBytes = 0;

    // Initial setup
    updateTeamNumber();

    m_logger->info("Communication Handler", "Communication handler initialized");
}

CommunicationHandler::~CommunicationHandler() {
    if (m_tcpConsoleSocket->isOpen()) {
        m_tcpConsoleSocket->disconnectFromHost();
    }
    
    if (m_logDownloadReply) {
        m_logDownloadReply->abort();
    }
    
    m_logger->info("Communication Handler", "Communication handler destroyed");
}

QHostAddress CommunicationHandler::calculateRobotAddress(int teamNumber) {
    if (teamNumber <= 0 || teamNumber > 9999) {
        return QHostAddress();
    }
    
    QString teamStr = QString::number(teamNumber).rightJustified(4, '0');
    QString ipPart1 = teamStr.mid(0, 2);
    QString ipPart2 = teamStr.mid(2, 2);
    
    return QHostAddress(QString("10.%1.%2.2").arg(ipPart1).arg(ipPart2));
}

void CommunicationHandler::updateTeamNumber() {
    int teamNumber = m_robotState->teamNumber();
    m_robotAddress = calculateRobotAddress(teamNumber);
    
    if (m_robotAddress.isNull()) {
        m_robotState->updateCommsStatus("Invalid Team #");
        return;
    }
    
    m_robotState->updateCommsStatus("No Comms");
    m_robotConnected = false;
    
    // Reset network stats
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_totalLatency = 0.0;
    m_latencyCount = 0;
    
    // Reconnect console
    if (m_tcpConsoleSocket->state() != QAbstractSocket::UnconnectedState) {
        m_tcpConsoleSocket->disconnectFromHost();
    }
    connectToConsole();
    
    m_logger->info("Communication", "Team number updated", 
                  QString("Team %1, Robot IP: %2").arg(teamNumber).arg(m_robotAddress.toString()));
}

void CommunicationHandler::connectToConsole() {
    if (!m_robotAddress.isNull() && m_tcpConsoleSocket->state() == QAbstractSocket::UnconnectedState) {
        m_tcpConsoleSocket->connectToHost(m_robotAddress, Network::ROBOT_CONSOLE_PORT);
    }
}

void CommunicationHandler::onConsoleConnected() {
    m_consoleConnected = true;
    m_logger->info("Console", "Console connected", m_robotAddress.toString());
}

void CommunicationHandler::onConsoleDisconnected() {
    m_consoleConnected = false;
    m_consoleReconnectTimer->start(3000);
    m_logger->debug("Console", "Console disconnected, will retry in 3 seconds");
}

void CommunicationHandler::onConsoleError() {
    m_consoleConnected = false;
    m_logger->debug("Console", "Console connection error", m_tcpConsoleSocket->errorString());
}

void CommunicationHandler::readConsoleData() {
    QByteArray data = m_tcpConsoleSocket->readAll();
    m_robotState->appendConsoleMessage(QString::fromLatin1(data));
}

void CommunicationHandler::sendPing() {
    if (m_robotAddress.isNull() || !m_robotConnected) return;
    
    // Create a simple ping packet with timestamp
    QByteArray pingPacket;
    QDataStream stream(&pingPacket, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    stream << static_cast<quint32>(0xDEADBEEF); // Ping marker
    stream << timestamp;
    
    m_udpSendSocket->writeDatagram(pingPacket, m_robotAddress, Network::DS_TO_ROBOT_PORT + 1);
    m_pingTimestamps[timestamp] = QDateTime::currentMSecsSinceEpoch();
    
    // Clean up old ping timestamps (older than 5 seconds)
    auto it = m_pingTimestamps.begin();
    while (it != m_pingTimestamps.end()) {
        if (QDateTime::currentMSecsSinceEpoch() - it.value() > 5000) {
            it = m_pingTimestamps.erase(it);
        } else {
            ++it;
        }
    }
}

void CommunicationHandler::processPingResponse(const QByteArray &data) {
    if (data.size() < 12) return; // Not enough data for ping response
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint32 marker;
    qint64 timestamp;
    stream >> marker >> timestamp;
    
    if (marker == 0xDEADBEEF && m_pingTimestamps.contains(timestamp)) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        double latency = now - m_pingTimestamps[timestamp];
        
        m_totalLatency += latency;
        m_latencyCount++;
        
        // Update average latency
        double avgLatency = m_totalLatency / m_latencyCount;
        m_robotState->updateNetworkLatency(avgLatency);
        
        m_pingTimestamps.remove(timestamp);
    }
}

void CommunicationHandler::updateNetworkStats() {
    if (!m_robotConnected) {
        m_robotState->updatePacketLoss(100.0);
        m_robotState->updateBandwidth(0.0);
        return;
    }
    
    // Calculate packet loss
    if (m_packetsSent > 0) {
        double lossRate = 100.0 * (1.0 - (double)m_packetsReceived / (double)m_packetsSent);
        m_robotState->updatePacketLoss(qMax(0.0, lossRate));
    }
    
    // Calculate bandwidth (rough estimate based on packet size and frequency)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 timeDiff = now - m_lastBandwidthTime;
    
    if (timeDiff > 0) {
        // Estimate bytes per second (control packets are ~100 bytes each at 50Hz)
        qint64 estimatedBytes = m_packetsReceived * 100; // Rough packet size
        double bandwidth = (estimatedBytes * 1000.0) / timeDiff; // bytes per second
        m_robotState->updateBandwidth(bandwidth / 1024.0); // Convert to KB/s
        
        m_lastBandwidthTime = now;
        m_lastBandwidthBytes = estimatedBytes;
    }
    
    // Reset counters periodically to prevent overflow
    if (m_packetsSent > 10000) {
        m_packetsSent /= 2;
        m_packetsReceived /= 2;
    }
}

void CommunicationHandler::checkNetworkTablesConnection() {
    if (m_robotAddress.isNull() || !m_robotConnected) {
        m_robotState->updateNetworkTablesStatus(false, "No Robot Connection");
        return;
    }
    
    // NetworkTables typically runs on port 1735
    if (m_networkTablesSocket->state() == QAbstractSocket::UnconnectedState) {
        m_networkTablesSocket->connectToHost(m_robotAddress, Network::NETWORKTABLES_PORT);
        
        if (m_networkTablesSocket->waitForConnected(1000)) {
            m_robotState->updateNetworkTablesStatus(true, "Connected");
            m_networkTablesSocket->disconnectFromHost();
        } else {
            m_robotState->updateNetworkTablesStatus(false, "Not Available");
        }
    }
}

void CommunicationHandler::sendRebootCommand() {
    if (m_robotAddress.isNull()) return;
    
    QByteArray packet;
    buildControlPacket(packet, RequestType::REBOOT);
    m_udpSendSocket->writeDatagram(packet, m_robotAddress, Network::DS_TO_ROBOT_PORT);
    
    m_logger->info("Robot Command", "Robot reboot command sent");
}

void CommunicationHandler::sendRestartCodeCommand() {
    if (m_robotAddress.isNull()) return;
    
    QByteArray packet;
    buildControlPacket(packet, RequestType::RESTART_CODE);
    m_udpSendSocket->writeDatagram(packet, m_robotAddress, Network::DS_TO_ROBOT_PORT);
    
    m_logger->info("Robot Command", "Robot code restart command sent");
}

void CommunicationHandler::sendControlPacket() {
    if (m_robotAddress.isNull()) return;
    
    QByteArray packet;
    buildControlPacket(packet);
    m_udpSendSocket->writeDatagram(packet, m_robotAddress, Network::DS_TO_ROBOT_PORT);
    
    m_packetsSent++;
}

void CommunicationHandler::buildControlPacket(QByteArray &packet, quint8 requestType) {
    // Create header
    DSToRobotHeader header;
    header.packetIndex = m_packetCounter++;
    
    // Set control flags
    header.control = 0;
    if (m_robotState->robotMode() == RobotState::Test) 
        header.control |= ControlFlags::TEST_MODE;
    if (m_robotState->robotMode() == RobotState::Autonomous) 
        header.control |= ControlFlags::AUTONOMOUS;
    
    // CRITICAL: Only set enabled flag when robot should be enabled
    if (m_robotState->enabled()) 
        header.control |= ControlFlags::ENABLED;
    
    // Set FMS flag if FMS is attached
    if (m_robotState->fmsConnected()) 
        header.control |= ControlFlags::FMS_ATTACHED;
    
    header.request = requestType != 0 ? requestType : RequestType::NORMAL;
    header.station = static_cast<quint8>(m_robotState->station());

    // Collect controller data from SLOTS
    QList<JoystickData> joysticks;
    for (int slot = 0; slot < Controllers::MAX_CONTROLLER_SLOTS; ++slot) {
        ControllerHIDDevice *device = m_controllerHandler->getControllerInSlot(slot);
        if (device && device->isConnected()) {
            JoystickData jsData;
            
            // Pack axes
            for (int axis = 0; axis < Controllers::MAX_AXES_PER_CONTROLLER; ++axis) {
                if (axis < device->getAxisCount()) {
                    jsData.axes.setAxis(axis, device->getAxisValue(axis));
                } else {
                    jsData.axes.setAxis(axis, 0.0); // Neutral for missing axes
                }
            }
            
            // Pack buttons
            for (int btn = 0; btn < Controllers::MAX_BUTTONS_PER_CONTROLLER; ++btn) {
                if (btn < device->getButtonCount()) {
                    jsData.buttons.setButton(btn, device->getButtonValue(btn));
                } else {
                    jsData.buttons.setButton(btn, false); // Not pressed for missing buttons
                }
            }
            
            // Pack POVs
            for (int pov = 0; pov < Controllers::MAX_POVS_PER_CONTROLLER; ++pov) {
                if (pov < device->getPOVCount()) {
                    jsData.povs.setPOV(pov, device->getPOVValue(pov));
                } else {
                    jsData.povs.setPOV(pov, -1); // Not pressed for missing POVs
                }
            }
            
            joysticks.append(jsData);
        } else {
            // Empty slot - send neutral controller data
            joysticks.append(JoystickData::neutral());
        }
    }

    // Build complete packet using PacketBuilder
    packet = PacketBuilder::buildDSPacket(header, joysticks);
    
    // Update controller status based on bound controllers
    QList<ControllerHIDDevice*> boundControllers = m_controllerHandler->getAllBoundControllers();
    if (boundControllers.isEmpty()) {
        m_robotState->updateJoystickStatus("No Controllers");
    } else {
        // Show count and types of bound controllers
        QStringList controllerTypes;
        for (ControllerHIDDevice *device : boundControllers) {
            QString type = device->name();
            if (type.contains("Xbox", Qt::CaseInsensitive)) {
                type = "Xbox";
            } else if (type.contains("PlayStation", Qt::CaseInsensitive) || type.contains("PS", Qt::CaseInsensitive)) {
                type = "PS";
            } else if (type.contains("HOTAS", Qt::CaseInsensitive)) {
                type = "HOTAS";
            } else if (type.contains("Flight", Qt::CaseInsensitive) || type.contains("Stick", Qt::CaseInsensitive)) {
                type = "Stick";
            } else {
                type = "Generic";
            }
            controllerTypes.append(type);
        }
        
        QString status = QString("%1 Bound").arg(boundControllers.count());
        if (controllerTypes.size() <= 3) {
            status += QString(" (%1)").arg(controllerTypes.join(", "));
        }
        m_robotState->updateJoystickStatus(status);
    }
}

void CommunicationHandler::readRobotPacket() {
    while (m_udpReceiveSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpReceiveSocket->receiveDatagram();
        
        // Check if this is a ping response
        if (datagram.data().size() >= 12) {
            QDataStream stream(datagram.data());
            stream.setByteOrder(QDataStream::BigEndian);
            quint32 marker;
            stream >> marker;
            
            if (marker == 0xDEADBEEF) {
                processPingResponse(datagram.data());
                continue; // Don't process as regular robot packet
            }
        }
        
        parseStatusPacket(datagram.data());
        m_lastPacketTime = QDateTime::currentMSecsSinceEpoch();
        m_robotConnected = true;
        m_packetsReceived++;
    }
}

void CommunicationHandler::parseStatusPacket(const QByteArray &data) {
    RobotToDSHeader header;
    RobotDiagnostics diagnostics;
    MatchTiming timing;
    
    if (PacketBuilder::parseRobotPacket(data, header, diagnostics, timing)) {
        // Update robot state with received data
        m_robotState->updateRobotVoltage(header.getVoltage());
        m_robotState->updateCpuUsage(diagnostics.cpuUsage);
        m_robotState->updateRamUsage(diagnostics.ramUsage);
        m_robotState->updateDiskUsage(diagnostics.diskUsage);
        m_robotState->updateCanUtil(diagnostics.getCanUtilPercent());
        m_robotState->updateCanBusOff(diagnostics.canBusOffCount);
        m_robotState->updateRobotCodeStatus(diagnostics.robotCodeStatus ? "Robot Code" : "No Code");
        m_robotState->updateMatchTime(timing.matchTimeRemaining);
        
        // Update match phase based on timing and FMS status
        if (m_robotState->fmsAttached()) {
            RobotState::MatchPhase phase = static_cast<RobotState::MatchPhase>(timing.matchPhase);
            m_robotState->updateMatchPhase(phase);
        }
        
        m_robotState->updateCommsStatus("Robot Connected");
    }
}

void CommunicationHandler::updateConnectionStatus() {
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // Check if we haven't received a packet in over 2 seconds
    if (m_robotConnected && (currentTime - m_lastPacketTime) > Network::PACKET_TIMEOUT_MS) {
        m_robotConnected = false;
        m_robotState->updateCommsStatus("No Comms");
        m_robotState->updateRobotCodeStatus("No Code");
        m_robotState->updateNetworkLatency(0.0);
        m_robotState->updatePacketLoss(100.0);
        m_robotState->updateBandwidth(0.0);
    }
}

void CommunicationHandler::downloadLogs(const QString &destinationPath) {
    if (m_logDownloadReply) {
        m_logger->warning("Log Download", "Log download already in progress");
        return;
    }
    
    m_currentDownloadPath = destinationPath;
    
    // Create destination directory if it doesn't exist
    QDir dir;
    if (!dir.mkpath(destinationPath)) {
        m_robotState->updateLogDownloadStatus("Error: Cannot create destination directory");
        m_robotState->onLogDownloadCompleted(destinationPath, false);
        return;
    }
    
    // First, request list of available log files
    requestAvailableLogFiles();
}

void CommunicationHandler::requestAvailableLogFiles() {
    if (m_robotAddress.isNull()) {
        m_robotState->updateLogDownloadStatus("Error: No robot address");
        m_robotState->onLogDownloadCompleted(m_currentDownloadPath, false);
        return;
    }
    
    m_robotState->updateLogDownloadStatus("Requesting log file list...");
    
    // WPILib logs are typically served via HTTP on port 5800
    QString url = QString("http://%1:5800/logs/").arg(m_robotAddress.toString());
    
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "FRC-DriverStation");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    
    m_logDownloadReply = m_networkManager->get(request);
    
    connect(m_logDownloadReply, &QNetworkReply::finished, this, [this]() {
        if (m_logDownloadReply->error() == QNetworkReply::NoError) {
            parseLogFileList(m_logDownloadReply->readAll());
        } else {
            m_robotState->updateLogDownloadStatus("Error: Cannot connect to robot log server");
            m_robotState->onLogDownloadCompleted(m_currentDownloadPath, false);
        }
        m_logDownloadReply->deleteLater();
        m_logDownloadReply = nullptr;
    });
    
    // Set timeout for the request
    QTimer::singleShot(10000, this, [this]() {
        if (m_logDownloadReply) {
            m_logDownloadReply->abort();
        }
    });
}

void CommunicationHandler::parseLogFileList(const QByteArray &data) {
    QString html = QString::fromUtf8(data);
    QStringList logFiles;
    
    // Parse HTML directory listing for .wpilog files
    QRegularExpression regex(R"(<a href="([^"]*\.wpilog)"[^>]*>)");
    QRegularExpressionMatchIterator matches = regex.globalMatch(html);
    
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString filename = match.captured(1);
        if (!filename.isEmpty()) {
            logFiles.append(filename);
        }
    }
    
    if (logFiles.isEmpty()) {
        m_robotState->updateLogDownloadStatus("No log files found on robot");
        m_robotState->onLogDownloadCompleted(m_currentDownloadPath, false);
        return;
    }
    
    // Update available log files
    m_robotState->updateAvailableLogFiles(logFiles);
    
    // Start downloading all log files
    m_pendingLogFiles = logFiles;
    m_currentFileIndex = 0;
    m_totalFiles = logFiles.size();
    
    downloadNextLogFile();
}

void CommunicationHandler::downloadNextLogFile() {
    if (m_currentFileIndex >= m_pendingLogFiles.size()) {
        // All files downloaded successfully
        m_robotState->updateLogDownloadStatus("All log files downloaded successfully");
        m_robotState->onLogDownloadCompleted(m_currentDownloadPath, true);
        return;
    }
    
    QString filename = m_pendingLogFiles[m_currentFileIndex];
    QString url = QString("http://%1:5800/logs/%2").arg(m_robotAddress.toString()).arg(filename);
    QString localPath = QDir(m_currentDownloadPath).absoluteFilePath(filename);
    
    m_robotState->updateLogDownloadStatus(QString("Downloading %1 (%2/%3)...")
                                         .arg(filename)
                                         .arg(m_currentFileIndex + 1)
                                         .arg(m_totalFiles));
    
    // Create and open the local file
    m_downloadFile = std::make_unique<QFile>(localPath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        m_robotState->updateLogDownloadStatus("Error: Cannot create local file");
        m_robotState->onLogDownloadCompleted(m_currentDownloadPath, false);
        return;
    }
    
    // Start the download
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "FRC-DriverStation");
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    
    m_logDownloadReply = m_networkManager->get(request);
    
    connect(m_logDownloadReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadFile) {
            m_downloadFile->write(m_logDownloadReply->readAll());
        }
    });
    
    connect(m_logDownloadReply, &QNetworkReply::downloadProgress, 
            this, &CommunicationHandler::onLogDownloadProgress);
    
    connect(m_logDownloadReply, &QNetworkReply::finished, 
            this, &CommunicationHandler::onLogDownloadFinished);
    
    connect(m_logDownloadReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &CommunicationHandler::onLogDownloadError);
}

void CommunicationHandler::onLogDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        // Calculate overall progress across all files
        int fileProgress = (bytesReceived * 100) / bytesTotal;
        int overallProgress = ((m_currentFileIndex * 100) + fileProgress) / m_totalFiles;
        m_robotState->updateLogDownloadProgress(overallProgress);
    }
}

void CommunicationHandler::onLogDownloadFinished() {
    if (m_downloadFile) {
        m_downloadFile->close();
        m_downloadFile.reset();
    }
    
    if (m_logDownloadReply->error() == QNetworkReply::NoError) {
        // File downloaded successfully, move to next file
        m_currentFileIndex++;
        m_logDownloadReply->deleteLater();
        m_logDownloadReply = nullptr;
        
        // Download next file
        downloadNextLogFile();
    } else {
        onLogDownloadError();
    }
}

void CommunicationHandler::onLogDownloadError() {
    QString error = m_logDownloadReply ? m_logDownloadReply->errorString() : "Unknown error";
    m_robotState->updateLogDownloadStatus(QString("Download error: %1").arg(error));
    
    if (m_downloadFile) {
        m_downloadFile->close();
        m_downloadFile.reset();
    }
    
    if (m_logDownloadReply) {
        m_logDownloadReply->deleteLater();
        m_logDownloadReply = nullptr;
    }
    
    m_robotState->onLogDownloadCompleted(m_currentDownloadPath, false);
}

void CommunicationHandler::cancelLogDownload() {
    if (m_logDownloadReply) {
        m_logDownloadReply->abort();
        m_logDownloadReply->deleteLater();
        m_logDownloadReply = nullptr;
    }
    
    if (m_downloadFile) {
        m_downloadFile->close();
        m_downloadFile.reset();
    }
    
    m_robotState->updateLogDownloadStatus("Download cancelled");
}
