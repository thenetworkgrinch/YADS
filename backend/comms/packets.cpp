#include "packets.h"
#include <QDataStream>
#include <QIODevice>
#include <QCryptographicHash>
#include <QDebug>

namespace FRC {

// ControlPacket implementation
QByteArray ControlPacket::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Header
    stream << static_cast<quint8>(CONTROL_PACKET);
    stream << sequenceNumber;
    stream << flags.toByte();
    stream << static_cast<quint8>(alliance);
    stream << static_cast<quint8>(position);
    
    // Joystick data (simplified - first joystick only for now)
    const JoystickData& joy = joysticks[0];
    
    // Axes (first 6 axes)
    for (int i = 0; i < 6 && i < JoystickData::MAX_AXES; ++i) {
        stream << joy.axes[i].value;
    }
    
    // Buttons
    stream << joy.buttons.buttons;
    
    // POVs (first POV only)
    stream << joy.povs[0].angle;
    
    // Calculate checksum
    quint16 checksum = calculatePacketChecksum(data);
    stream << checksum;
    
    return data;
}

bool ControlPacket::deserialize(const QByteArray& data)
{
    if (data.size() < PACKET_SIZE + 2) { // +2 for checksum
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 packetType;
    stream >> packetType;
    
    if (packetType != CONTROL_PACKET) {
        return false;
    }
    
    stream >> sequenceNumber;
    
    quint8 flagsByte;
    stream >> flagsByte;
    flags.fromByte(flagsByte);
    
    quint8 allianceByte, positionByte;
    stream >> allianceByte >> positionByte;
    alliance = static_cast<Alliance>(allianceByte);
    position = static_cast<Position>(positionByte);
    
    // Read joystick data
    JoystickData& joy = joysticks[0];
    
    // Axes
    for (int i = 0; i < 6 && i < JoystickData::MAX_AXES; ++i) {
        qint8 axisValue;
        stream >> axisValue;
        joy.axes[i] = JoystickAxis(axisValue);
    }
    
    // Buttons
    quint32 buttonData;
    stream >> buttonData;
    joy.buttons = JoystickButtons(buttonData);
    
    // POV
    qint16 povAngle;
    stream >> povAngle;
    joy.povs[0] = JoystickPOV(povAngle);
    
    // Verify checksum
    quint16 providedChecksum;
    stream >> providedChecksum;
    quint16 calculatedChecksum = calculatePacketChecksum(data.left(data.size() - 2));
    
    return providedChecksum == calculatedChecksum;
}

// StatusPacket implementation
QByteArray StatusPacket::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(STATUS_PACKET);
    stream << sequenceNumber;
    stream << flags.toByte();
    stream << static_cast<quint16>(batteryVoltage * 100); // Convert to centivolt
    stream << canUtilization;
    
    // Calculate checksum
    quint16 checksum = calculatePacketChecksum(data);
    stream << checksum;
    
    return data;
}

bool StatusPacket::deserialize(const QByteArray& data)
{
    if (data.size() < PACKET_SIZE + 2) { // +2 for checksum
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 packetType;
    stream >> packetType;
    
    if (packetType != STATUS_PACKET) {
        return false;
    }
    
    stream >> sequenceNumber;
    
    quint8 flagsByte;
    stream >> flagsByte;
    flags.fromByte(flagsByte);
    
    quint16 voltageRaw;
    stream >> voltageRaw;
    batteryVoltage = voltageRaw / 100.0; // Convert from centivolt
    
    stream >> canUtilization;
    
    // Verify checksum
    quint16 providedChecksum;
    stream >> providedChecksum;
    quint16 calculatedChecksum = calculatePacketChecksum(data.left(data.size() - 2));
    
    return providedChecksum == calculatedChecksum;
}

// ConsolePacket implementation
QByteArray ConsolePacket::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(CONSOLE_PACKET);
    stream << sequenceNumber;
    stream << static_cast<quint64>(timestamp.toMSecsSinceEpoch());
    
    QByteArray messageBytes = message.toUtf8();
    stream << static_cast<quint16>(messageBytes.size());
    stream.writeRawData(messageBytes.constData(), messageBytes.size());
    
    // Calculate checksum
    quint16 checksum = calculatePacketChecksum(data);
    stream << checksum;
    
    return data;
}

bool ConsolePacket::deserialize(const QByteArray& data)
{
    if (data.size() < 13) { // Minimum size: type(1) + seq(2) + timestamp(8) + messageLength(2) + checksum(2)
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 packetType;
    stream >> packetType;
    
    if (packetType != CONSOLE_PACKET) {
        return false;
    }
    
    stream >> sequenceNumber;
    
    quint64 timestampMs;
    stream >> timestampMs;
    timestamp = QDateTime::fromMSecsSinceEpoch(timestampMs);
    
    quint16 messageLength;
    stream >> messageLength;
    
    if (data.size() < 15 + messageLength) { // +2 for checksum
        return false;
    }
    
    QByteArray messageBytes(messageLength, 0);
    stream.readRawData(messageBytes.data(), messageLength);
    message = QString::fromUtf8(messageBytes);
    
    // Verify checksum
    quint16 providedChecksum;
    stream >> providedChecksum;
    quint16 calculatedChecksum = calculatePacketChecksum(data.left(data.size() - 2));
    
    return providedChecksum == calculatedChecksum;
}

// HeartbeatPacket implementation
QByteArray HeartbeatPacket::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(HEARTBEAT_PACKET);
    stream << sequenceNumber;
    stream << timestamp;
    
    // Calculate checksum
    quint16 checksum = calculatePacketChecksum(data);
    stream << checksum;
    
    return data;
}

bool HeartbeatPacket::deserialize(const QByteArray& data)
{
    if (data.size() < PACKET_SIZE + 2) { // +2 for checksum
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 packetType;
    stream >> packetType;
    
    if (packetType != HEARTBEAT_PACKET) {
        return false;
    }
    
    stream >> sequenceNumber;
    stream >> timestamp;
    
    // Verify checksum
    quint16 providedChecksum;
    stream >> providedChecksum;
    quint16 calculatedChecksum = calculatePacketChecksum(data.left(data.size() - 2));
    
    return providedChecksum == calculatedChecksum;
}

// PacketFactory implementation
PacketFactory::PacketFactory(QObject* parent) : QObject(parent)
{
}

ControlPacket PacketFactory::createControlPacket(bool enabled, RobotMode mode, 
                                               Alliance alliance, Position position)
{
    ControlPacket packet;
    packet.flags.enabled = enabled;
    packet.flags.autonomous = (mode == AUTONOMOUS);
    packet.flags.test = (mode == TEST);
    packet.flags.dsAttached = true;
    packet.alliance = alliance;
    packet.position = position;
    
    return packet;
}

StatusPacket PacketFactory::createStatusPacket(bool enabled, double batteryVoltage)
{
    StatusPacket packet;
    packet.flags.robotEnabled = enabled;
    packet.flags.codeReady = true;
    packet.batteryVoltage = batteryVoltage;
    
    return packet;
}

ConsolePacket PacketFactory::createConsolePacket(const QString& message)
{
    return ConsolePacket(message);
}

HeartbeatPacket PacketFactory::createHeartbeatPacket()
{
    HeartbeatPacket packet;
    packet.timestamp = static_cast<quint16>(QDateTime::currentMSecsSinceEpoch() & 0xFFFF);
    return packet;
}

QByteArray PacketFactory::serializeControlPacket(const ControlPacket& packet)
{
    return packet.serialize();
}

QByteArray PacketFactory::serializeStatusPacket(const StatusPacket& packet)
{
    return packet.serialize();
}

QByteArray PacketFactory::serializeConsolePacket(const ConsolePacket& packet)
{
    return packet.serialize();
}

QByteArray PacketFactory::serializeHeartbeatPacket(const HeartbeatPacket& packet)
{
    return packet.serialize();
}

bool PacketFactory::deserializeControlPacket(const QByteArray& data, ControlPacket& packet)
{
    return packet.deserialize(data);
}

bool PacketFactory::deserializeStatusPacket(const QByteArray& data, StatusPacket& packet)
{
    return packet.deserialize(data);
}

bool PacketFactory::deserializeConsolePacket(const QByteArray& data, ConsolePacket& packet)
{
    return packet.deserialize(data);
}

bool PacketFactory::deserializeHeartbeatPacket(const QByteArray& data, HeartbeatPacket& packet)
{
    return packet.deserialize(data);
}

PacketType PacketFactory::getPacketType(const QByteArray& data)
{
    return FRC::getPacketType(data);
}

bool PacketFactory::isValidPacket(const QByteArray& data)
{
    PacketType type = getPacketType(data);
    
    switch (type) {
        case CONTROL_PACKET:
            return data.size() >= ControlPacket::PACKET_SIZE + 2;
        case STATUS_PACKET:
            return data.size() >= StatusPacket::PACKET_SIZE + 2;
        case HEARTBEAT_PACKET:
            return data.size() >= HeartbeatPacket::PACKET_SIZE + 2;
        case CONSOLE_PACKET:
            return data.size() >= 15; // Minimum console packet size including checksum
        default:
            return false;
    }
}

QString PacketFactory::packetTypeToString(PacketType type)
{
    return FRC::packetTypeToString(type);
}

QString PacketFactory::robotModeToString(RobotMode mode)
{
    return FRC::robotModeToString(mode);
}

QString PacketFactory::allianceToString(Alliance alliance)
{
    return FRC::allianceToString(alliance);
}

QString PacketFactory::positionToString(Position position)
{
    return FRC::positionToString(position);
}

quint16 PacketFactory::calculateChecksum(const QByteArray& data)
{
    return FRC::calculatePacketChecksum(data);
}

bool PacketFactory::verifyChecksum(const QByteArray& data)
{
    return FRC::validatePacketChecksum(data);
}

} // namespace FRC

// Debug operators
QDebug operator<<(QDebug debug, const FRC::ControlFlags& flags)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ControlFlags("
                    << "enabled=" << flags.enabled
                    << ", autonomous=" << flags.autonomous
                    << ", test=" << flags.test
                    << ", emergencyStop=" << flags.emergencyStop
                    << ", fmsAttached=" << flags.fmsAttached
                    << ", dsAttached=" << flags.dsAttached
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const FRC::StatusFlags& flags)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "StatusFlags("
                    << "robotEnabled=" << flags.robotEnabled
                    << ", autonomous=" << flags.autonomous
                    << ", test=" << flags.test
                    << ", emergencyStop=" << flags.emergencyStop
                    << ", brownout=" << flags.brownout
                    << ", codeReady=" << flags.codeReady
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const FRC::JoystickData& joystick)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "JoystickData("
                    << "axes=[" << joystick.axes[0].value << "," << joystick.axes[1].value << ",...]"
                    << ", buttons=0x" << Qt::hex << joystick.buttons.buttons
                    << ", pov=" << joystick.povs[0].angle
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const FRC::ControlPacket& packet)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ControlPacket("
                    << "seq=" << packet.sequenceNumber
                    << ", flags=" << packet.flags
                    << ", alliance=" << FRC::PacketFactory::allianceToString(packet.alliance)
                    << ", position=" << FRC::PacketFactory::positionToString(packet.position)
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const FRC::StatusPacket& packet)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "StatusPacket("
                    << "seq=" << packet.sequenceNumber
                    << ", flags=" << packet.flags
                    << ", battery=" << packet.batteryVoltage << "V"
                    << ", canUtil=" << packet.canUtilization << "%"
                    << ")";
    return debug;
}

// Utility functions
bool validatePacketChecksum(const QByteArray& packet) {
    // Simple validation - check minimum size
    return packet.size() >= 2;
}

quint16 calculatePacketChecksum(const QByteArray& packet) {
    quint16 checksum = 0;
    for (int i = 0; i < packet.size(); ++i) {
        checksum += static_cast<quint8>(packet[i]);
    }
    return checksum;
}

PacketType getPacketType(const QByteArray& packet) {
    if (packet.isEmpty()) {
        return HEARTBEAT_PACKET; // Default
    }
    
    quint8 type = static_cast<quint8>(packet[0]);
    switch (type) {
        case CONTROL_PACKET:
        case STATUS_PACKET:
        case CONSOLE_PACKET:
        case HEARTBEAT_PACKET:
        case JOYSTICK_PACKET:
            return static_cast<PacketType>(type);
        default:
            return HEARTBEAT_PACKET; // Default fallback
    }
}

QString packetTypeToString(PacketType type) {
    switch (type) {
        case CONTROL_PACKET: return "Control";
        case STATUS_PACKET: return "Status";
        case CONSOLE_PACKET: return "Console";
        case HEARTBEAT_PACKET: return "Heartbeat";
        case JOYSTICK_PACKET: return "Joystick";
        default: return "Unknown";
    }
}

QString robotModeToString(RobotMode mode) {
    switch (mode) {
        case DISABLED: return "Disabled";
        case AUTONOMOUS: return "Autonomous";
        case TELEOP: return "Teleop";
        case TEST: return "Test";
        default: return "Unknown";
    }
}

QString allianceToString(Alliance alliance) {
    switch (alliance) {
        case RED: return "Red";
        case BLUE: return "Blue";
        default: return "Unknown";
    }
}

QString positionToString(Position position) {
    switch (position) {
        case POSITION_1: return "1";
        case POSITION_2: return "2";
        case POSITION_3: return "3";
        default: return "Unknown";
    }
}

} // namespace FRC
