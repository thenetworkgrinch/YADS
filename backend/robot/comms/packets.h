#ifndef ROBOT_PACKETS_H
#define ROBOT_PACKETS_H

#include <QByteArray>
#include <QDataStream>
#include <QHostAddress>

namespace RobotPackets {

// Packet types
enum PacketType {
    ControlPacket = 0x00,
    StatusPacket = 0x01,
    JoystickPacket = 0x02,
    DisablePacket = 0x03
};

// Control packet structure
struct ControlData {
    quint16 packetNumber;
    quint8 controlByte;
    quint8 requestByte;
    quint16 teamNumber;
    quint8 alliance;
    quint8 position;
    
    // Control byte flags
    bool isEnabled() const { return (controlByte & 0x01) != 0; }
    bool isAutonomous() const { return (controlByte & 0x02) != 0; }
    bool isTest() const { return (controlByte & 0x04) != 0; }
    bool isEmergencyStop() const { return (controlByte & 0x80) != 0; }
    
    void setEnabled(bool enabled) {
        if (enabled) controlByte |= 0x01;
        else controlByte &= ~0x01;
    }
    
    void setAutonomous(bool autonomous) {
        if (autonomous) controlByte |= 0x02;
        else controlByte &= ~0x02;
    }
    
    void setTest(bool test) {
        if (test) controlByte |= 0x04;
        else controlByte &= ~0x04;
    }
    
    void setEmergencyStop(bool emergencyStop) {
        if (emergencyStop) controlByte |= 0x80;
        else controlByte &= ~0x80;
    }
};

// Status packet structure
struct StatusData {
    quint16 packetNumber;
    quint8 statusByte;
    quint8 batteryHigh;
    quint8 batteryLow;
    quint8 brownoutProtection;
    quint8 reserved1;
    quint8 reserved2;
    quint8 reserved3;
    
    // Battery voltage in 0.1V increments
    int batteryVoltage() const {
        return (batteryHigh << 8) | batteryLow;
    }
    
    void setBatteryVoltage(int voltage) {
        batteryHigh = (voltage >> 8) & 0xFF;
        batteryLow = voltage & 0xFF;
    }
    
    // Status byte flags
    bool isRobotEnabled() const { return (statusByte & 0x01) != 0; }
    bool isAutonomousMode() const { return (statusByte & 0x02) != 0; }
    bool isTestMode() const { return (statusByte & 0x04) != 0; }
    bool isEmergencyStop() const { return (statusByte & 0x80) != 0; }
    bool isBrownout() const { return (brownoutProtection & 0x01) != 0; }
};

// Joystick data structure
struct JoystickData {
    quint8 axes[6];      // 6 analog axes
    quint16 buttons;     // 16 digital buttons
    quint8 povs[4];      // 4 POV hats
    
    // Helper methods
    float getAxis(int index) const {
        if (index < 0 || index >= 6) return 0.0f;
        return (axes[index] - 128) / 127.0f; // Convert to -1.0 to 1.0
    }
    
    void setAxis(int index, float value) {
        if (index < 0 || index >= 6) return;
        axes[index] = static_cast<quint8>((value * 127.0f) + 128);
    }
    
    bool getButton(int index) const {
        if (index < 0 || index >= 16) return false;
        return (buttons & (1 << index)) != 0;
    }
    
    void setButton(int index, bool pressed) {
        if (index < 0 || index >= 16) return;
        if (pressed) buttons |= (1 << index);
        else buttons &= ~(1 << index);
    }
    
    int getPOV(int index) const {
        if (index < 0 || index >= 4) return -1;
        return povs[index] == 0xFF ? -1 : povs[index] * 45; // Convert to degrees
    }
    
    void setPOV(int index, int angle) {
        if (index < 0 || index >= 4) return;
        povs[index] = (angle < 0) ? 0xFF : static_cast<quint8>(angle / 45);
    }
};

// Packet creation functions
QByteArray createControlPacket(const ControlData &data);
QByteArray createStatusPacket(const StatusData &data);
QByteArray createJoystickPacket(const JoystickData &data);
QByteArray createDisablePacket();

// Packet parsing functions
bool parseControlPacket(const QByteArray &packet, ControlData &data);
bool parseStatusPacket(const QByteArray &packet, StatusData &data);
bool parseJoystickPacket(const QByteArray &packet, JoystickData &data);

// Utility functions
bool isValidPacket(const QByteArray &packet);
PacketType getPacketType(const QByteArray &packet);
quint16 calculateChecksum(const QByteArray &data);
bool verifyChecksum(const QByteArray &packet);

// Network utilities
QHostAddress getTeamAddress(int teamNumber, int device = 2);
bool isValidTeamNumber(int teamNumber);
QString formatTeamAddress(int teamNumber, int device = 2);

} // namespace RobotPackets

#endif // ROBOT_PACKETS_H
