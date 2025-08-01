#ifndef ROBOT_PACKETS_H
#define ROBOT_PACKETS_H

#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>
#include <QDateTime>

/**
 * @brief Robot communication packet definitions and utilities
 * 
 * This file contains all the packet structures and utilities for
 * communicating with FRC robots using the standard protocol.
 */

namespace RobotPackets {

// Packet types
enum PacketType : quint8 {
    ROBOT_CONTROL = 0x00,
    ROBOT_STATUS = 0x01,
    JOYSTICK_DATA = 0x02,
    TIMEZONE_DATA = 0x03,
    DISABLE_FAULT = 0x04,
    RAIL_FAULT = 0x05,
    REBOOT = 0x06,
    VERSION = 0x07,
    ERROR_DATA = 0x08,
    PRINT_DATA = 0x09,
    NETCONSOLE = 0x0A
};

// Control packet flags
enum ControlFlags : quint8 {
    ENABLED = 0x01,
    AUTONOMOUS = 0x02,
    TEST = 0x04,
    EMERGENCY_STOP = 0x08,
    FMS_ATTACHED = 0x10,
    DS_ATTACHED = 0x20
};

// Status packet flags
enum StatusFlags : quint8 {
    ROBOT_ENABLED = 0x01,
    ROBOT_AUTO = 0x02,
    ROBOT_TEST = 0x04,
    ROBOT_ESTOP = 0x08,
    BROWNOUT = 0x10,
    CODE_READY = 0x20
};

#pragma pack(push, 1)

/**
 * @brief Robot control packet structure
 */
struct ControlPacket {
    quint16 packetIndex;
    quint8 generalData;
    quint8 mode;
    quint16 request;
    
    // Joystick data (6 joysticks max)
    struct JoystickData {
        quint8 axes[6];
        quint16 buttons;
        quint8 povs[4];
    } joysticks[6];
    
    // CRC
    quint32 crc;
    
    ControlPacket();
    void setEnabled(bool enabled);
    void setMode(quint8 mode);
    void setEmergencyStop(bool estop);
    void setFMSAttached(bool attached);
    void calculateCRC();
    QByteArray toByteArray() const;
};

/**
 * @brief Robot status packet structure
 */
struct StatusPacket {
    quint8 generalData;
    quint8 mode;
    quint16 batteryVoltage; // In millivolts
    quint16 request;
    quint32 pcmId;
    quint8 pdpId;
    quint8 pcmVersion;
    quint8 pdpVersion;
    quint32 robotCodeVersion;
    
    StatusPacket();
    bool isEnabled() const;
    bool isAutonomous() const;
    bool isTest() const;
    bool isEmergencyStop() const;
    bool isBrownout() const;
    bool isCodeReady() const;
    double getBatteryVoltage() const;
    void fromByteArray(const QByteArray &data);
};

/**
 * @brief Joystick data packet
 */
struct JoystickPacket {
    quint8 joystickId;
    quint8 axisCount;
    quint8 axes[12];
    quint8 buttonCount;
    quint16 buttons;
    quint8 povCount;
    quint16 povs[4];
    
    JoystickPacket();
    void setAxis(int index, double value);
    void setButton(int index, bool pressed);
    void setPOV(int index, int angle);
    QByteArray toByteArray() const;
};

#pragma pack(pop)

/**
 * @brief Packet builder and parser utilities
 */
class PacketBuilder {
public:
    static QByteArray buildControlPacket(const ControlPacket &packet);
    static QByteArray buildJoystickPacket(const JoystickPacket &packet);
    static QByteArray buildTimezonePacket(const QTimeZone &timezone);
    static QByteArray buildRebootPacket();
    static QByteArray buildVersionRequestPacket();
    
    static bool parseStatusPacket(const QByteArray &data, StatusPacket &packet);
    static bool parseErrorPacket(const QByteArray &data, QString &error);
    static bool parsePrintPacket(const QByteArray &data, QString &message);
    
    static quint32 calculateCRC32(const QByteArray &data);
    static bool verifyCRC32(const QByteArray &data, quint32 expectedCRC);
};

/**
 * @brief Network utilities for robot communication
 */
class NetworkUtils {
public:
    static QHostAddress getRobotAddress(int teamNumber);
    static QList<QHostAddress> getAllRobotAddresses(int teamNumber);
    static quint16 getRobotPort();
    static quint16 getDriverStationPort();
    
    static bool isValidTeamNumber(int teamNumber);
    static QString formatTeamNumber(int teamNumber);
};

} // namespace RobotPackets

#endif // ROBOT_PACKETS_H
