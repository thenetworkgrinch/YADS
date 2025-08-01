#ifndef ROBOT_FMS_HANDLER_H
#define ROBOT_FMS_HANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>

/**
 * @brief Handles FMS communication specifically for robot control
 * 
 * This is a specialized FMS handler that focuses on robot control
 * aspects of FMS communication, separate from the main FMS handler.
 */
class RobotFMSHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool fmsConnected READ isFMSConnected NOTIFY fmsConnectedChanged)
    Q_PROPERTY(bool robotEnabled READ isRobotEnabled NOTIFY robotEnabledChanged)
    Q_PROPERTY(QString robotMode READ robotMode NOTIFY robotModeChanged)

public:
    explicit RobotFMSHandler(QObject *parent = nullptr);
    ~RobotFMSHandler();

    bool isFMSConnected() const { return m_fmsConnected; }
    bool isRobotEnabled() const { return m_robotEnabled; }
    QString robotMode() const { return m_robotMode; }

public slots:
    void enableRobot();
    void disableRobot();
    void setRobotMode(const QString &mode);
    void emergencyStop();

signals:
    void fmsConnectedChanged(bool connected);
    void robotEnabledChanged(bool enabled);
    void robotModeChanged(const QString &mode);
    void fmsCommandReceived(const QString &command);

private slots:
    void processFMSCommand();
    void sendRobotStatus();

private:
    void parseCommand(const QByteArray &data);
    void updateRobotState(bool enabled, const QString &mode);

    QUdpSocket *m_commandSocket;
    QTimer *m_statusTimer;
    
    bool m_fmsConnected;
    bool m_robotEnabled;
    QString m_robotMode;
    
    QHostAddress m_fmsAddress;
    quint16 m_commandPort;
    quint16 m_statusPort;
};

#endif // ROBOT_FMS_HANDLER_H
