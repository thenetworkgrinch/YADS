#ifndef PACKETS_H
#define PACKETS_H

#include <QByteArray>
#include <QList>
#include <QtGlobal>

namespace FRCDriverStation {
namespace Protocol {

// Protocol constants
namespace Constants {
    constexpr quint16 DS_TO_ROBOT_PORT = 1110;
    constexpr quint16 ROBOT_TO_DS_PORT = 1150;
    constexpr quint16 ROBOT_CONSOLE_PORT = 6666;
    constexpr int PACKET_TIMEOUT_MS = 2000;
}

// Control flags for DS to Robot packets
namespace ControlFlags {
    constexpr quint8 ENABLED = 0x01;
    constexpr quint8 AUTONOMOUS = 0x02;
    constexpr quint8 TEST_MODE = 0x04;
    constexpr quint8 EMERGENCY_STOP = 0x08;
    constexpr quint8 FMS_ATTACHED = 0x10;
    constexpr quint8 DS_ATTACHED = 0x20;
}

// Request types
namespace RequestType {
    constexpr quint8 NORMAL = 0x00;
    constexpr quint8 REBOOT = 0x01;
    constexpr quint8 RESTART_CODE = 0x02;
}

// Joystick data structures
struct JoystickAxes {
    float axes[6] = {0.0f}; // Up to 6 axes per joystick
    
    void setAxis(int index, float value) {
        if (index >= 0 && index < 6) {
            axes[index] = qBound(-1.0f, value, 1.0f);
        }
    }
    
    float getAxis(int index) const {
        return (index >= 0 && index < 6) ? axes[index] : 0.0f;
    }
};

struct JoystickButtons {
    quint16 buttons = 0; // Up to 16 buttons per joystick
    
    void setButton(int index, bool pressed) {
        if (index >= 0 && index < 16) {
            if (pressed) {
                buttons |= (1 << index);
            } else {
                buttons &= ~(1 << index);
            }
        }
    }
    
    bool getButton(int index) const {
        return (index >= 0 && index < 16) ? ((buttons >> index) & 1) != 0 : false;
    }
};

struct JoystickPOVs {
    qint16 povs[4] = {-1, -1, -1, -1}; // Up to 4 POVs per joystick, -1 = not pressed
    
    void setPOV(int index, qint16 angle) {
        if (index >= 0 && index < 4) {
            povs[index] = angle;
        }
    }
    
    qint16 getPOV(int index) const {
        return (index >= 0 && index < 4) ? povs[index] : -1;
    }
};

struct JoystickData {
    JoystickAxes axes;
    JoystickButtons buttons;
    JoystickPOVs povs;
    
    JoystickData() = default;
    
    // Create neutral joystick data
    static JoystickData neutral() {
        return JoystickData();
    }
};

// DS to Robot packet header
struct DSToRobotHeader {
    quint16 packetIndex = 0;
    quint8 control = 0;
    quint8 request = RequestType::NORMAL;
    quint8 station = 0; // Alliance station (0-5: Red1, Red2, Red3, Blue1, Blue2, Blue3)
    
    // Helper methods
    bool isEnabled() const { return (control & ControlFlags::ENABLED) != 0; }
    bool isAutonomous() const { return (control & ControlFlags::AUTONOMOUS) != 0; }
    bool isTest() const { return (control & ControlFlags::TEST_MODE) != 0; }
    bool isEmergencyStop() const { return (control & ControlFlags::EMERGENCY_STOP) != 0; }
    bool isFMSAttached() const { return (control & ControlFlags::FMS_ATTACHED) != 0; }
    
    void setEnabled(bool enabled) {
        if (enabled) control |= ControlFlags::ENABLED;
        else control &= ~ControlFlags::ENABLED;
    }
    
    void setAutonomous(bool autonomous) {
        if (autonomous) control |= ControlFlags::AUTONOMOUS;
        else control &= ~ControlFlags::AUTONOMOUS;
    }
    
    void setTest(bool test) {
        if (test) control |= ControlFlags::TEST_MODE;
        else control &= ~ControlFlags::TEST_MODE;
    }
    
    void setEmergencyStop(bool estop) {
        if (estop) control |= ControlFlags::EMERGENCY_STOP;
        else control &= ~ControlFlags::EMERGENCY_STOP;
    }
    
    void setFMSAttached(bool attached) {
        if (attached) control |= ControlFlags::FMS_ATTACHED;
        else control &= ~ControlFlags::FMS_ATTACHED;
    }
};

// Robot to DS packet header
struct RobotToDSHeader {
    quint16 packetIndex = 0;
    quint8 control = 0;
    quint8 status = 0;
    quint16 voltage = 0; // Battery voltage in millivolts
    
    // Helper methods
    double getVoltage() const { return voltage / 1000.0; }
    void setVoltage(double volts) { voltage = static_cast<quint16>(volts * 1000); }
    
    bool isEnabled() const { return (control & ControlFlags::ENABLED) != 0; }
    bool isAutonomous() const { return (control & ControlFlags::AUTONOMOUS) != 0; }
    bool isTest() const { return (control & ControlFlags::TEST_MODE) != 0; }
    bool isEmergencyStop() const { return (control & ControlFlags::EMERGENCY_STOP) != 0; }
};

// Robot diagnostics data
struct RobotDiagnostics {
    quint8 cpuUsage = 0;        // CPU usage percentage (0-100)
    quint8 ramUsage = 0;        // RAM usage percentage (0-100)
    quint8 diskUsage = 0;       // Disk usage percentage (0-100)
    quint16 canUtilization = 0; // CAN utilization in 0.1% units
    quint8 canBusOffCount = 0;  // Number of CAN bus-off events
    quint8 robotCodeStatus = 0; // 0 = no code, 1 = code running
    
    // Helper methods
    double getCanUtilPercent() const { return canUtilization / 10.0; }
    void setCanUtilPercent(double percent) { canUtilization = static_cast<quint16>(percent * 10); }
};

// Match timing information
struct MatchTiming {
    quint8 matchPhase = 0;          // 0=pre, 1=auto, 2=teleop, 3=endgame, 4=post
    quint16 matchTimeRemaining = 0; // Time remaining in seconds
    
    enum Phase {
        PreMatch = 0,
        Autonomous = 1,
        Teleop = 2,
        Endgame = 3,
        PostMatch = 4
    };
};

/**
 * @brief Packet builder and parser utilities
 * 
 * Provides methods to build and parse FRC protocol packets.
 * All methods are static and thread-safe.
 */
class PacketBuilder
{
public:
    /**
     * @brief Build a complete DS to Robot packet
     * @param header Packet header with control information
     * @param joysticks List of joystick data (up to 6 joysticks)
     * @return Complete packet as byte array
     */
    static QByteArray buildDSPacket(const DSToRobotHeader &header, 
                                   const QList<JoystickData> &joysticks);
    
    /**
     * @brief Parse a Robot to DS packet
     * @param data Raw packet data
     * @param header Output header structure
     * @param diagnostics Output diagnostics structure
     * @param timing Output timing structure
     * @return true if packet was parsed successfully
     */
    static bool parseRobotPacket(const QByteArray &data,
                                RobotToDSHeader &header,
                                RobotDiagnostics &diagnostics,
                                MatchTiming &timing);
    
    /**
     * @brief Validate packet checksum
     * @param data Packet data including checksum
     * @return true if checksum is valid
     */
    static bool validateChecksum(const QByteArray &data);
    
    /**
     * @brief Calculate packet checksum
     * @param data Packet data without checksum
     * @return Calculated checksum
     */
    static quint16 calculateChecksum(const QByteArray &data);

private:
    PacketBuilder() = delete; // Static class only
};

} // namespace Protocol
} // namespace FRCDriverStation

#endif // PACKETS_H
