#ifndef ROBOT_COMMS_PACKETS_H
#define ROBOT_COMMS_PACKETS_H

#include <QObject>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QHostAddress>
#include <QMutex>
#include "../../core/constants.h"

/**
 * @brief Handles FRC robot communication packets
 * 
 * This class manages the creation, parsing, and validation of FRC protocol packets
 * used for robot communication, including driver station to robot and robot to
 * driver station packets.
 */
class RobotPackets : public QObject
{
    Q_OBJECT

public:
    // Packet types
    enum PacketType {
        DriverStationToRobot = 0x00,
        RobotToDriverStation = 0x01,
        JoystickData = 0x02,
        TimestampData = 0x03,
        DisableData = 0x04,
        TaggedData = 0x05
    };
    Q_ENUM(PacketType)

    // Robot modes
    enum RobotMode {
        Disabled = 0x00,
        Autonomous = 0x01,
        Teleop = 0x02,
        Test = 0x03
    };
    Q_ENUM(RobotMode)

    // Control flags
    enum ControlFlags {
        Enabled = 0x01,
        Autonomous_Flag = 0x02,
        Test_Flag = 0x04,
        EmergencyStop = 0x08,
        FMSAttached = 0x10,
        DSAttached = 0x20
    };
    Q_FLAGS(ControlFlags)

    // Joystick data structure
    struct JoystickData {
        quint8 axes[6];          // 6 axes, values 0-255
        quint16 buttons;         // 16 buttons as bitmask
        quint8 povs[4];          // 4 POV hats, values 0-8 (8 = not pressed)
        
        JoystickData() {
            memset(axes, 128, sizeof(axes)); // Center position
            buttons = 0;
            memset(povs, 8, sizeof(povs)); // Not pressed
        }
    };

    // Driver station packet structure
    struct DriverStationPacket {
        quint16 packetIndex;
        quint8 generalData;
        quint8 dsDigitalIn;
        quint16 teamNumber;
        quint8 dsID_Alliance;
        quint8 dsID_Position;
        JoystickData joysticks[6]; // Up to 6 joysticks
        quint16 crc;
        
        DriverStationPacket() {
            packetIndex = 0;
            generalData = 0;
            dsDigitalIn = 0;
            teamNumber = 0;
            dsID_Alliance = 0;
            dsID_Position = 0;
            crc = 0;
        }
    };

    // Robot status packet structure
    struct RobotStatusPacket {
        quint16 packetIndex;
        quint8 generalData;
        quint8 robotDigitalOut;
        quint16 batteryVoltage; // In millivolts
        quint8 canUtilization;
        quint8 wifiDB;
        quint8 wifiMB;
        quint16 crc;
        
        RobotStatusPacket() {
            packetIndex = 0;
            generalData = 0;
            robotDigitalOut = 0;
            batteryVoltage = 0;
            canUtilization = 0;
            wifiDB = 0;
            wifiMB = 0;
            crc = 0;
        }
    };

    explicit RobotPackets(QObject *parent = nullptr);
    ~RobotPackets();

    // Packet creation methods
    QByteArray createDriverStationPacket(const DriverStationPacket& packet);
    QByteArray createJoystickPacket(const JoystickData joysticks[], int count);
    QByteArray createDisablePacket();
    QByteArray createEmergencyStopPacket();
    QByteArray createTimestampPacket();

    // Packet parsing methods
    bool parseRobotStatusPacket(const QByteArray& data, RobotStatusPacket& packet);
    bool parseIncomingPacket(const QByteArray& data, PacketType& type);
    bool validatePacket(const QByteArray& data);

    // Utility methods
    static quint16 calculateCRC(const QByteArray& data);
    static bool verifyCRC(const QByteArray& data);
    static QByteArray addCRC(const QByteArray& data);

    // Getters
    quint16 getNextPacketIndex();
    quint32 getPacketsSent() const { return m_packetsSent; }
    quint32 getPacketsReceived() const { return m_packetsReceived; }
    quint32 getPacketsDropped() const { return m_packetsDropped; }

    // Statistics
    void resetStatistics();
    double getPacketLossRate() const;

public slots:
    void setTeamNumber(quint16 teamNumber);
    void setRobotMode(RobotMode mode);
    void setEnabled(bool enabled);
    void setEmergencyStop(bool emergencyStop);
    void setFMSAttached(bool attached);

signals:
    void packetSent(PacketType type, int size);
    void packetReceived(PacketType type, int size);
    void packetDropped(const QString& reason);
    void robotStatusUpdated(const RobotStatusPacket& status);

private:
    // Packet state
    quint16 m_packetIndex;
    quint16 m_teamNumber;
    RobotMode m_robotMode;
    bool m_enabled;
    bool m_emergencyStop;
    bool m_fmsAttached;
    
    // Statistics
    quint32 m_packetsSent;
    quint32 m_packetsReceived;
    quint32 m_packetsDropped;
    QDateTime m_statisticsStartTime;
    
    // Thread safety
    mutable QMutex m_mutex;
    
    // Helper methods
    void updateGeneralData(quint8& generalData);
    bool isValidPacketSize(PacketType type, int size);
    void logPacketInfo(const QString& direction, PacketType type, int size);
    
    // CRC calculation table
    static const quint16 CRC_TABLE[256];
    static void initializeCRCTable();
    static bool s_crcTableInitialized;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RobotPackets::ControlFlags)

#endif // ROBOT_COMMS_PACKETS_H
