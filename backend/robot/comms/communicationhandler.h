#ifndef COMMUNICATIONHANDLER_H
#define COMMUNICATIONHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QStringList>
#include <memory>
#include "packets.h"

namespace FRCDriverStation {

class RobotState;
class ControllerHIDHandler;
class Logger;

/**
 * @brief Handles all communication with the roboRIO
 * 
 * This class manages:
 * - UDP control packet transmission (DS -> Robot)
 * - UDP status packet reception (Robot -> DS)  
 * - TCP console log streaming (Robot -> DS)
 * - Network connection management
 * - Network diagnostics (ping, packet loss, bandwidth)
 * - Robot command transmission (reboot, restart code)
 * - Log file downloading from the robot
 * - NetworkTables monitoring
 * 
 * Design principles:
 * - Single responsibility: Only handles robot communication
 * - Fail fast: Clear error reporting and timeout handling
 * - Observable: Emits signals for state changes
 * - Non-blocking: All operations are asynchronous
 */
class CommunicationHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit CommunicationHandler(RobotState *robotState, 
                                 ControllerHIDHandler *controllerHandler,
                                 std::shared_ptr<Logger> logger,
                                 QObject *parent = nullptr);
    ~CommunicationHandler();
    
public slots:
    void sendControlPacket();
    void sendRebootCommand();
    void sendRestartCodeCommand();
    void downloadLogs(const QString &destinationPath);
    void cancelLogDownload();
    void checkNetworkTablesConnection();

private slots:
    void readRobotPacket();
    void updateTeamNumber();
    void connectToConsole();
    void readConsoleData();
    void onConsoleConnected();
    void onConsoleDisconnected();
    void onConsoleError();
    void onLogDownloadFinished();
    void onLogDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onLogDownloadError();
    void sendPing();
    void updateNetworkStats();
    void requestAvailableLogFiles();
    void processPingResponse(const QByteArray &data);
    void updateConnectionStatus();

private:
    // Network utilities
    QHostAddress calculateRobotAddress(int teamNumber);
    void buildControlPacket(QByteArray &packet, quint8 requestType = 0);
    void parseStatusPacket(const QByteArray &data);
    void parseLogFileList(const QByteArray &data);
    void downloadNextLogFile();
    
    // Network sockets
    std::unique_ptr<QUdpSocket> m_udpSendSocket;
    std::unique_ptr<QUdpSocket> m_udpReceiveSocket;
    std::unique_ptr<QTcpSocket> m_tcpConsoleSocket;
    std::unique_ptr<QTcpSocket> m_networkTablesSocket;
    
    // Timers
    std::unique_ptr<QTimer> m_sendTimer;
    std::unique_ptr<QTimer> m_consoleReconnectTimer;
    std::unique_ptr<QTimer> m_watchdogTimer;
    std::unique_ptr<QTimer> m_pingTimer;
    std::unique_ptr<QTimer> m_networkStatsTimer;
    std::unique_ptr<QTimer> m_networkTablesTimer;
    
    // State references
    RobotState *m_robotState;
    ControllerHIDHandler *m_controllerHandler;
    std::shared_ptr<Logger> m_logger;
    
    // Network state
    QHostAddress m_robotAddress;
    quint16 m_packetCounter;
    qint64 m_lastPacketTime;
    bool m_robotConnected;
    bool m_consoleConnected;
    
    // Network diagnostics state
    QMap<qint64, qint64> m_pingTimestamps;
    quint32 m_packetsSent;
    quint32 m_packetsReceived;
    double m_totalLatency;
    quint32 m_latencyCount;
    qint64 m_lastBandwidthTime;
    qint64 m_lastBandwidthBytes;
    
    // Log download state
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QNetworkReply *m_logDownloadReply;
    std::unique_ptr<QFile> m_downloadFile;
    QString m_currentDownloadPath;
    QStringList m_pendingLogFiles;
    int m_currentFileIndex;
    int m_totalFiles;
};

} // namespace FRCDriverStation

#endif // COMMUNICATIONHANDLER_H
