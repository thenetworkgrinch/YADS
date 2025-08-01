#ifndef PACKETS_H
#define PACKETS_H

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QDataStream>

namespace FRC {

/**
 * @brief FRC Driver Station packet types
 */
enum PacketType {
    CONTROL_PACKET = 0x10,
    STATUS_PACKET = 0x01,
    CONSOLE_PACKET = 0x02,
    HEARTBEAT_PACKET = 0x00,
    JOYSTICK_PACKET = 0x11
};

/**
 * @brief Robot operating modes
 */
enum RobotMode {
    DISABLED = 0,
    AUTONOMOUS = 1,
    TELEOP = 2,
    TEST = 3
};

/**
 * @brief Alliance colors
 */
enum Alliance {
    RED = 0,
    BLUE = 1
};

/**
 * @brief Alliance positions
 */
enum Position {
    POSITION_1 = 0,
    POSITION_2 = 1,
    POSITION_3 = 2
};

/**
 * @brief Control packet flags
 */
struct ControlFlags {
    bool enabled = false;
    bool autonomous = false;
    bool test = false;
    bool emergencyStop = false;
    bool dsAttached = true;
    bool fmsAttached = false;
    
    quint8 toByte() const {
        quint8 flags = 0;
        if (enabled) flags |= 0x01;
        if (autonomous) flags |= 0x02;
        if (test) flags |= 0x04;
        if (emergencyStop) flags |= 0x08;
        if (dsAttached) flags |= 0x10;
        if (fmsAttached) flags |= 0x20;
        return flags;
    }
    
    void fromByte(quint8 flags) {
        enabled = (flags & 0x01) != 0;
        autonomous = (flags & 0x02) != 0;
        test = (flags & 0x04) != 0;
        emergencyStop = (flags & 0x08) != 0;
        dsAttached = (flags & 0x10) != 0;
        fmsAttached = (flags & 0x20) != 0;
    }
};

/**
 * @brief Status packet flags
 */
struct StatusFlags {
    bool robotEnabled = false;
    bool robotConnected = false;
    bool robotCodeRunning = false;
    bool emergencyStop = false;
    bool brownout = false;
    
    void fromByte(quint8 flags) {
        robotEnabled = (flags & 0x01) != 0;
        robotConnected = (flags & 0x02) != 0;
        robotCodeRunning = (flags & 0x04) != 0;
        emergencyStop = (flags & 0x08) != 0;
        brownout = (flags & 0x10) != 0;
    }
};

/**
 * @brief Joystick data structure
 */
struct JoystickData {
    static const int MAX_AXES = 8;
    static const int MAX_BUTTONS = 32;
    static const int MAX_POVS = 4;
    
    float axes[MAX_AXES];
    quint32 buttons;
    qint16 povs[MAX_POVS];
    
    JoystickData() {
        clear();
    }
    
    void clear() {
        for (int i = 0; i < MAX_AXES; ++i) {
            axes[i] = 0.0f;
        }
        buttons = 0;
        for (int i = 0; i < MAX_POVS; ++i) {
            povs[i] = -1;
        }
    }
    
    QByteArray serialize() const {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Serialize axes
        for (int i = 0; i < MAX_AXES; ++i) {
            qint16 axisValue = static_cast<qint16>(axes[i] * 32767);
            stream << axisValue;
        }
        
        // Serialize buttons
        stream << buttons;
        
        // Serialize POVs
        for (int i = 0; i < MAX_POVS; ++i) {
            stream << povs[i];
        }
        
        return data;
    }
    
    bool deserialize(const QByteArray& data) {
        if (data.size() < (MAX_AXES * 2 + 4 + MAX_POVS * 2)) {
            return false;
        }
        
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Deserialize axes
        for (int i = 0; i < MAX_AXES; ++i) {
            qint16 axisValue;
            stream >> axisValue;
            axes[i] = static_cast<float>(axisValue) / 32767.0f;
        }
        
        // Deserialize buttons
        stream >> buttons;
        
        // Deserialize POVs
        for (int i = 0; i < MAX_POVS; ++i) {
            stream >> povs[i];
        }
        
        return true;
    }
};

/**
 * @brief Control packet sent from DS to robot
 */
struct ControlPacket {
    static const int MAX_JOYSTICKS = 6;
    
    quint16 sequenceNumber = 0;
    ControlFlags flags;
    Alliance alliance = RED;
    Position position = POSITION_1;
    JoystickData joysticks[MAX_JOYSTICKS];
    
    QByteArray serialize() const {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Packet header
        stream << static_cast<quint8>(CONTROL_PACKET);
        stream << static_cast<quint8>(0x00); // Reserved
        
        // Sequence number
        stream << sequenceNumber;
        
        // Control flags
        stream << flags.toByte();
        
        // Alliance and position
        stream << static_cast<quint8>(alliance);
        stream << static_cast<quint8>(position);
        
        // Reserved byte
        stream << static_cast<quint8>(0x00);
        
        // Joystick data
        for (int i = 0; i < MAX_JOYSTICKS; ++i) {
            QByteArray joystickData = joysticks[i].serialize();
            data.append(joystickData);
        }
        
        return data;
    }
    
    bool deserialize(const QByteArray& data) {
        if (data.size() < 8) {
            return false;
        }
        
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Check packet type
        quint8 packetType;
        stream >> packetType;
        if (packetType != CONTROL_PACKET) {
            return false;
        }
        
        // Skip reserved byte
        quint8 reserved;
        stream >> reserved;
        
        // Sequence number
        stream >> sequenceNumber;
        
        // Control flags
        quint8 flagsByte;
        stream >> flagsByte;
        flags.fromByte(flagsByte);
        
        // Alliance and position
        quint8 allianceByte, positionByte;
        stream >> allianceByte >> positionByte;
        alliance = static_cast<Alliance>(allianceByte);
        position = static_cast<Position>(positionByte);
        
        // Skip reserved byte
        stream >> reserved;
        
        // Joystick data
        int remainingBytes = data.size() - 8;
        int joystickDataSize = sizeof(JoystickData);
        int joystickCount = qMin(MAX_JOYSTICKS, remainingBytes / joystickDataSize);
        
        for (int i = 0; i < joystickCount; ++i) {
            QByteArray joystickData = data.mid(8 + i * joystickDataSize, joystickDataSize);
            joysticks[i].deserialize(joystickData);
        }
        
        return true;
    }
};

/**
 * @brief Status packet sent from robot to DS
 */
struct StatusPacket {
    quint16 sequenceNumber = 0;
    StatusFlags flags;
    double batteryVoltage = 0.0;
    double cpuUsage = 0.0;
    double memoryUsage = 0.0;
    quint32 canUtilization = 0;
    
    QByteArray serialize() const {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Packet header
        stream << static_cast<quint8>(STATUS_PACKET);
        stream << static_cast<quint8>(0x00); // Reserved
        
        // Sequence number
        stream << sequenceNumber;
        
        // Status flags
        quint8 flagsByte = 0;
        if (flags.robotEnabled) flagsByte |= 0x01;
        if (flags.robotConnected) flagsByte |= 0x02;
        if (flags.robotCodeRunning) flagsByte |= 0x04;
        if (flags.emergencyStop) flagsByte |= 0x08;
        if (flags.brownout) flagsByte |= 0x10;
        stream << flagsByte;
        
        // Battery voltage (16-bit fixed point, 8.8 format)
        quint16 voltageFixed = static_cast<quint16>(batteryVoltage * 256);
        stream << voltageFixed;
        
        // CPU usage (8-bit percentage)
        stream << static_cast<quint8>(cpuUsage);
        
        // Memory usage (8-bit percentage)
        stream << static_cast<quint8>(memoryUsage);
        
        // CAN utilization
        stream << canUtilization;
        
        return data;
    }
    
    bool deserialize(const QByteArray& data) {
        if (data.size() < 12) {
            return false;
        }
        
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Check packet type
        quint8 packetType;
        stream >> packetType;
        if (packetType != STATUS_PACKET) {
            return false;
        }
        
        // Skip reserved byte
        quint8 reserved;
        stream >> reserved;
        
        // Sequence number
        stream >> sequenceNumber;
        
        // Status flags
        quint8 flagsByte;
        stream >> flagsByte;
        flags.fromByte(flagsByte);
        
        // Battery voltage
        quint16 voltageFixed;
        stream >> voltageFixed;
        batteryVoltage = static_cast<double>(voltageFixed) / 256.0;
        
        // CPU usage
        quint8 cpuByte;
        stream >> cpuByte;
        cpuUsage = static_cast<double>(cpuByte);
        
        // Memory usage
        quint8 memoryByte;
        stream >> memoryByte;
        memoryUsage = static_cast<double>(memoryByte);
        
        // CAN utilization
        stream >> canUtilization;
        
        return true;
    }
};

/**
 * @brief Console packet for robot output
 */
struct ConsolePacket {
    QString message;
    QDateTime timestamp;
    
    QByteArray serialize() const {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Packet header
        stream << static_cast<quint8>(CONSOLE_PACKET);
        
        // Timestamp
        stream << static_cast<quint64>(timestamp.toMSecsSinceEpoch());
        
        // Message length
        QByteArray messageBytes = message.toUtf8();
        stream << static_cast<quint16>(messageBytes.size());
        
        // Message data
        data.append(messageBytes);
        
        return data;
    }
    
    bool deserialize(const QByteArray& data) {
        if (data.size() < 11) {
            return false;
        }
        
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Check packet type
        quint8 packetType;
        stream >> packetType;
        if (packetType != CONSOLE_PACKET) {
            return false;
        }
        
        // Timestamp
        quint64 timestampMs;
        stream >> timestampMs;
        timestamp = QDateTime::fromMSecsSinceEpoch(timestampMs);
        
        // Message length
        quint16 messageLength;
        stream >> messageLength;
        
        // Message data
        if (data.size() < 11 + messageLength) {
            return false;
        }
        
        QByteArray messageBytes = data.mid(11, messageLength);
        message = QString::fromUtf8(messageBytes);
        
        return true;
    }
};

/**
 * @brief Heartbeat packet for connection monitoring
 */
struct HeartbeatPacket {
    quint32 timestamp = 0;
    quint16 sequenceNumber = 0;
    
    QByteArray serialize() const {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Packet header
        stream << static_cast<quint8>(HEARTBEAT_PACKET);
        stream << static_cast<quint8>(0x00); // Reserved
        
        // Timestamp
        stream << timestamp;
        
        // Sequence number
        stream << sequenceNumber;
        
        return data;
    }
    
    bool deserialize(const QByteArray& data) {
        if (data.size() < 8) {
            return false;
        }
        
        QDataStream stream(data);
        stream.setByteOrder(QDataStream::BigEndian);
        
        // Check packet type
        quint8 packetType;
        stream >> packetType;
        if (packetType != HEARTBEAT_PACKET) {
            return false;
        }
        
        // Skip reserved byte
        quint8 reserved;
        stream >> reserved;
        
        // Timestamp
        stream >> timestamp;
        
        // Sequence number
        stream >> sequenceNumber;
        
        return true;
    }
};

} // namespace FRC

#endif // PACKETS_H
