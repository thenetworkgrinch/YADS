#include "packets.h"
#include "../../core/logger.h"
#include <QMutexLocker>
#include <QDebug>
#include <cstring>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QtEndian>

using namespace FRCDriverStation::Protocol;

// CRC-16 lookup table for FRC protocol
const quint16 RobotPackets::CRC_TABLE[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

bool RobotPackets::s_crcTableInitialized = false;

RobotPackets::RobotPackets(QObject *parent)
    : QObject(parent)
    , m_packetIndex(0)
    , m_teamNumber(0)
    , m_robotMode(Disabled)
    , m_enabled(false)
    , m_emergencyStop(false)
    , m_fmsAttached(false)
    , m_packetsSent(0)
    , m_packetsReceived(0)
    , m_packetsDropped(0)
    , m_statisticsStartTime(QDateTime::currentDateTime())
{
    Logger::instance().log(Logger::Info, "RobotPackets", "Initializing robot packet handler");
    
    if (!s_crcTableInitialized) {
        initializeCRCTable();
        s_crcTableInitialized = true;
    }
    
    Logger::instance().log(Logger::Info, "RobotPackets", "Robot packet handler initialized");
}

RobotPackets::~RobotPackets()
{
    Logger::instance().log(Logger::Info, "RobotPackets", "Robot packet handler destroyed");
}

QByteArray PacketBuilder::buildDSPacket(const DSToRobotHeader &header, 
                                       const QList<JoystickData> &joysticks)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Write header
    stream << header.packetIndex;
    stream << header.control;
    stream << header.request;
    stream << header.station;
    
    // Write joystick count (limited to 6)
    quint8 joystickCount = static_cast<quint8>(qMin(joysticks.size(), 6));
    stream << joystickCount;
    
    // Write joystick data
    for (int i = 0; i < 6; ++i) {
        if (i < joysticks.size()) {
            const JoystickData &js = joysticks[i];
            
            // Write axes (6 axes per joystick)
            for (int axis = 0; axis < 6; ++axis) {
                stream << js.axes.getAxis(axis);
            }
            
            // Write buttons (16 buttons as 2 bytes)
            stream << js.buttons.buttons;
            
            // Write POVs (4 POVs per joystick)
            for (int pov = 0; pov < 4; ++pov) {
                stream << js.povs.getPOV(pov);
            }
        } else {
            // Write neutral joystick data for empty slots
            JoystickData neutral;
            
            // Neutral axes
            for (int axis = 0; axis < 6; ++axis) {
                stream << 0.0f;
            }
            
            // No buttons pressed
            stream << quint16(0);
            
            // No POVs pressed
            for (int pov = 0; pov < 4; ++pov) {
                stream << qint16(-1);
            }
        }
    }
    
    // Calculate and append checksum
    quint16 checksum = calculateChecksum(packet);
    stream << checksum;
    
    return packet;
}

bool PacketBuilder::parseRobotPacket(const QByteArray &data,
                                    RobotToDSHeader &header,
                                    RobotDiagnostics &diagnostics,
                                    MatchTiming &timing)
{
    if (data.size() < 16) { // Minimum packet size
        return false;
    }
    
    // Validate checksum first
    if (!validateChecksum(data)) {
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Read header
    stream >> header.packetIndex;
    stream >> header.control;
    stream >> header.status;
    stream >> header.voltage;
    
    // Read diagnostics
    stream >> diagnostics.cpuUsage;
    stream >> diagnostics.ramUsage;
    stream >> diagnostics.diskUsage;
    stream >> diagnostics.canUtilization;
    stream >> diagnostics.canBusOffCount;
    stream >> diagnostics.robotCodeStatus;
    
    // Read timing
    stream >> timing.matchPhase;
    stream >> timing.matchTimeRemaining;
    
    return stream.status() == QDataStream::Ok;
}

bool PacketBuilder::validateChecksum(const QByteArray &data)
{
    if (data.size() < 2) {
        return false;
    }
    
    // Extract checksum from end of packet
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(data.size() - 2);
    
    quint16 receivedChecksum;
    stream >> receivedChecksum;
    
    // Calculate checksum of data without the checksum bytes
    QByteArray dataWithoutChecksum = data.left(data.size() - 2);
    quint16 calculatedChecksum = calculateChecksum(dataWithoutChecksum);
    
    return receivedChecksum == calculatedChecksum;
}

quint16 PacketBuilder::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    
    // Sum all bytes in the data
    for (int i = 0; i < data.size(); ++i) {
        sum += static_cast<quint8>(data[i]);
    }
    
    // Return 16-bit checksum
    return static_cast<quint16>(sum & 0xFFFF);
}

QByteArray RobotPackets::createDriverStationPacket(const DriverStationPacket& packet)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Write packet header
    stream << packet.packetIndex;
    stream << packet.generalData;
    stream << packet.dsDigitalIn;
    stream << packet.teamNumber;
    stream << packet.dsID_Alliance;
    stream << packet.dsID_Position;
    
    // Write joystick data
    for (int i = 0; i < 6; ++i) {
        const JoystickData& joy = packet.joysticks[i];
        
        // Write axes
        for (int j = 0; j < 6; ++j) {
            stream << joy.axes[j];
        }
        
        // Write buttons
        stream << joy.buttons;
        
        // Write POVs
        for (int j = 0; j < 4; ++j) {
            stream << joy.povs[j];
        }
    }
    
    // Add CRC
    data = addCRC(data);
    
    QMutexLocker locker(&m_mutex);
    m_packetsSent++;
    locker.unlock();
    
    emit packetSent(DriverStationToRobot, data.size());
    logPacketInfo("Sent", DriverStationToRobot, data.size());
    
    return data;
}

QByteArray RobotPackets::createJoystickPacket(const JoystickData joysticks[], int count)
{
    DriverStationPacket packet;
    packet.packetIndex = getNextPacketIndex();
    updateGeneralData(packet.generalData);
    packet.teamNumber = m_teamNumber;
    
    // Copy joystick data
    int maxJoysticks = qMin(count, 6);
    for (int i = 0; i < maxJoysticks; ++i) {
        packet.joysticks[i] = joysticks[i];
    }
    
    return createDriverStationPacket(packet);
}

QByteArray RobotPackets::createDisablePacket()
{
    DriverStationPacket packet;
    packet.packetIndex = getNextPacketIndex();
    packet.generalData = 0; // Disabled
    packet.teamNumber = m_teamNumber;
    
    QByteArray data = createDriverStationPacket(packet);
    
    Logger::instance().log(Logger::Info, "RobotPackets", "Created disable packet");
    return data;
}

QByteArray RobotPackets::createEmergencyStopPacket()
{
    DriverStationPacket packet;
    packet.packetIndex = getNextPacketIndex();
    updateGeneralData(packet.generalData);
    packet.generalData |= EmergencyStop; // Set emergency stop flag
    packet.teamNumber = m_teamNumber;
    
    QByteArray data = createDriverStationPacket(packet);
    
    Logger::instance().log(Logger::Warning, "RobotPackets", "Created emergency stop packet");
    return data;
}

QByteArray RobotPackets::createTimestampPacket()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Timestamp packet format
    stream << quint16(getNextPacketIndex());
    stream << quint8(TimestampData);
    stream << quint32(QDateTime::currentSecsSinceEpoch());
    stream << quint32(QDateTime::currentMSecsSinceEpoch() % 1000);
    
    data = addCRC(data);
    
    QMutexLocker locker(&m_mutex);
    m_packetsSent++;
    locker.unlock();
    
    emit packetSent(TimestampData, data.size());
    logPacketInfo("Sent", TimestampData, data.size());
    
    return data;
}

bool RobotPackets::parseRobotStatusPacket(const QByteArray& data, RobotStatusPacket& packet)
{
    if (data.size() < 12) {
        Logger::instance().log(Logger::Warning, "RobotPackets", "Robot status packet too small");
        return false;
    }
    
    if (!verifyCRC(data)) {
        Logger::instance().log(Logger::Warning, "RobotPackets", "Robot status packet CRC invalid");
        QMutexLocker locker(&m_mutex);
        m_packetsDropped++;
        locker.unlock();
        emit packetDropped("Invalid CRC");
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream >> packet.packetIndex;
    stream >> packet.generalData;
    stream >> packet.robotDigitalOut;
    stream >> packet.batteryVoltage;
    stream >> packet.canUtilization;
    stream >> packet.wifiDB;
    stream >> packet.wifiMB;
    
    QMutexLocker locker(&m_mutex);
    m_packetsReceived++;
    locker.unlock();
    
    emit packetReceived(RobotToDriverStation, data.size());
    emit robotStatusUpdated(packet);
    logPacketInfo("Received", RobotToDriverStation, data.size());
    
    return true;
}

bool RobotPackets::parseIncomingPacket(const QByteArray& data, PacketType& type)
{
    if (data.size() < 3) {
        Logger::instance().log(Logger::Warning, "RobotPackets", "Incoming packet too small");
        return false;
    }
    
    if (!verifyCRC(data)) {
        Logger::instance().log(Logger::Warning, "RobotPackets", "Incoming packet CRC invalid");
        QMutexLocker locker(&m_mutex);
        m_packetsDropped++;
        locker.unlock();
        emit packetDropped("Invalid CRC");
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint16 packetIndex;
    quint8 packetType;
    
    stream >> packetIndex >> packetType;
    type = static_cast<PacketType>(packetType);
    
    if (!isValidPacketSize(type, data.size())) {
        Logger::instance().log(Logger::Warning, "RobotPackets", 
                              QString("Invalid packet size for type %1: %2 bytes")
                              .arg(packetType).arg(data.size()));
        QMutexLocker locker(&m_mutex);
        m_packetsDropped++;
        locker.unlock();
        emit packetDropped("Invalid packet size");
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    m_packetsReceived++;
    locker.unlock();
    
    emit packetReceived(type, data.size());
    logPacketInfo("Received", type, data.size());
    
    return true;
}

bool RobotPackets::validatePacket(const QByteArray& data)
{
    if (data.size() < 4) {
        return false;
    }
    
    return verifyCRC(data);
}

quint16 RobotPackets::calculateCRC(const QByteArray& data)
{
    quint16 crc = 0xFFFF;
    
    for (int i = 0; i < data.size(); ++i) {
        quint8 byte = static_cast<quint8>(data[i]);
        crc = (crc << 8) ^ CRC_TABLE[((crc >> 8) ^ byte) & 0xFF];
    }
    
    return crc;
}

bool RobotPackets::verifyCRC(const QByteArray& data)
{
    if (data.size() < 2) {
        return false;
    }
    
    // Extract CRC from end of packet
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.skipRawData(data.size() - 2);
    
    quint16 receivedCRC;
    stream >> receivedCRC;
    
    // Calculate CRC for data without the CRC bytes
    QByteArray dataWithoutCRC = data.left(data.size() - 2);
    quint16 calculatedCRC = calculateCRC(dataWithoutCRC);
    
    return receivedCRC == calculatedCRC;
}

QByteArray RobotPackets::addCRC(const QByteArray& data)
{
    QByteArray result = data;
    quint16 crc = calculateCRC(data);
    
    QDataStream stream(&result, QIODevice::WriteOnly | QIODevice::Append);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << crc;
    
    return result;
}

quint16 RobotPackets::getNextPacketIndex()
{
    QMutexLocker locker(&m_mutex);
    return ++m_packetIndex;
}

void RobotPackets::resetStatistics()
{
    QMutexLocker locker(&m_mutex);
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_packetsDropped = 0;
    m_statisticsStartTime = QDateTime::currentDateTime();
    
    Logger::instance().log(Logger::Info, "RobotPackets", "Statistics reset");
}

double RobotPackets::getPacketLossRate() const
{
    QMutexLocker locker(&m_mutex);
    
    quint32 totalPackets = m_packetsSent + m_packetsReceived;
    if (totalPackets == 0) {
        return 0.0;
    }
    
    return (static_cast<double>(m_packetsDropped) / totalPackets) * 100.0;
}

void RobotPackets::setTeamNumber(quint16 teamNumber)
{
    QMutexLocker locker(&m_mutex);
    if (m_teamNumber != teamNumber) {
        m_teamNumber = teamNumber;
        Logger::instance().log(Logger::Info, "RobotPackets", 
                              QString("Team number set to %1").arg(teamNumber));
    }
}

void RobotPackets::setRobotMode(RobotMode mode)
{
    QMutexLocker locker(&m_mutex);
    if (m_robotMode != mode) {
        m_robotMode = mode;
        Logger::instance().log(Logger::Info, "RobotPackets", 
                              QString("Robot mode set to %1").arg(static_cast<int>(mode)));
    }
}

void RobotPackets::setEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    if (m_enabled != enabled) {
        m_enabled = enabled;
        Logger::instance().log(Logger::Info, "RobotPackets", 
                              QString("Robot %1").arg(enabled ? "enabled" : "disabled"));
    }
}

void RobotPackets::setEmergencyStop(bool emergencyStop)
{
    QMutexLocker locker(&m_mutex);
    if (m_emergencyStop != emergencyStop) {
        m_emergencyStop = emergencyStop;
        Logger::instance().log(Logger::Warning, "RobotPackets", 
                              QString("Emergency stop %1").arg(emergencyStop ? "activated" : "cleared"));
    }
}

void RobotPackets::setFMSAttached(bool attached)
{
    QMutexLocker locker(&m_mutex);
    if (m_fmsAttached != attached) {
        m_fmsAttached = attached;
        Logger::instance().log(Logger::Info, "RobotPackets", 
                              QString("FMS %1").arg(attached ? "attached" : "detached"));
    }
}

void RobotPackets::updateGeneralData(quint8& generalData)
{
    generalData = 0;
    
    if (m_enabled) {
        generalData |= Enabled;
    }
    
    if (m_robotMode == Autonomous) {
        generalData |= Autonomous_Flag;
    } else if (m_robotMode == Test) {
        generalData |= Test_Flag;
    }
    
    if (m_emergencyStop) {
        generalData |= EmergencyStop;
        generalData &= ~Enabled; // Also disable
    }
    
    if (m_fmsAttached) {
        generalData |= FMSAttached;
    }
    
    generalData |= DSAttached; // Driver station is always attached
}

bool RobotPackets::isValidPacketSize(PacketType type, int size)
{
    switch (type) {
        case DriverStationToRobot:
            return size >= 1024; // Full DS packet
        case RobotToDriverStation:
            return size >= 12; // Minimum robot status packet
        case JoystickData:
            return size >= 64; // Joystick data packet
        case TimestampData:
            return size >= 12; // Timestamp packet
        case DisableData:
            return size >= 8; // Disable packet
        case TaggedData:
            return size >= 4; // Tagged data packet
        default:
            return false;
    }
}

void RobotPackets::logPacketInfo(const QString& direction, PacketType type, int size)
{
    QString typeName;
    switch (type) {
        case DriverStationToRobot: typeName = "DS->Robot"; break;
        case RobotToDriverStation: typeName = "Robot->DS"; break;
        case JoystickData: typeName = "Joystick"; break;
        case TimestampData: typeName = "Timestamp"; break;
        case DisableData: typeName = "Disable"; break;
        case TaggedData: typeName = "Tagged"; break;
        default: typeName = "Unknown"; break;
    }
    
    Logger::instance().log(Logger::Debug, "RobotPackets", 
                          QString("%1 %2 packet (%3 bytes)")
                          .arg(direction)
                          .arg(typeName)
                          .arg(size));
}

void RobotPackets::initializeCRCTable()
{
    // CRC table is already initialized as a static const array
    // This function exists for consistency but doesn't need to do anything
    Logger::instance().log(Logger::Debug, "RobotPackets", "CRC table initialized");
}

namespace RobotPackets {

// ControlPacket implementation
ControlPacket::ControlPacket()
    : packetIndex(0)
    , generalData(0)
    , mode(0)
    , request(0)
    , crc(0)
{
    // Initialize joystick data
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            joysticks[i].axes[j] = 127; // Neutral position
        }
        joysticks[i].buttons = 0;
        for (int j = 0; j < 4; ++j) {
            joysticks[i].povs[j] = 255; // No POV pressed
        }
    }
}

void ControlPacket::setEnabled(bool enabled)
{
    if (enabled) {
        generalData |= ControlFlags::ENABLED;
    } else {
        generalData &= ~ControlFlags::ENABLED;
    }
}

void ControlPacket::setMode(quint8 newMode)
{
    mode = newMode;
    
    // Clear mode flags
    generalData &= ~(ControlFlags::AUTONOMOUS | ControlFlags::TEST);
    
    // Set appropriate mode flag
    if (newMode == 1) { // Autonomous
        generalData |= ControlFlags::AUTONOMOUS;
    } else if (newMode == 2) { // Test
        generalData |= ControlFlags::TEST;
    }
    // Teleop is default (no flag)
}

void ControlPacket::setEmergencyStop(bool estop)
{
    if (estop) {
        generalData |= ControlFlags::EMERGENCY_STOP;
        generalData &= ~ControlFlags::ENABLED; // Also disable
    } else {
        generalData &= ~ControlFlags::EMERGENCY_STOP;
    }
}

void ControlPacket::setFMSAttached(bool attached)
{
    if (attached) {
        generalData |= ControlFlags::FMS_ATTACHED;
    } else {
        generalData &= ~ControlFlags::FMS_ATTACHED;
    }
}

void ControlPacket::calculateCRC()
{
    QByteArray data = toByteArray();
    // Remove the last 4 bytes (CRC field) before calculating
    data.chop(4);
    crc = PacketBuilder::calculateCRC32(data);
}

QByteArray ControlPacket::toByteArray() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << packetIndex;
    stream << generalData;
    stream << mode;
    stream << request;
    
    // Write joystick data
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            stream << joysticks[i].axes[j];
        }
        stream << joysticks[i].buttons;
        for (int j = 0; j < 4; ++j) {
            stream << joysticks[i].povs[j];
        }
    }
    
    stream << crc;
    
    return data;
}

// StatusPacket implementation
StatusPacket::StatusPacket()
    : generalData(0)
    , mode(0)
    , batteryVoltage(0)
    , request(0)
    , pcmId(0)
    , pdpId(0)
    , pcmVersion(0)
    , pdpVersion(0)
    , robotCodeVersion(0)
{
}

bool StatusPacket::isEnabled() const
{
    return generalData & StatusFlags::ROBOT_ENABLED;
}

bool StatusPacket::isAutonomous() const
{
    return generalData & StatusFlags::ROBOT_AUTO;
}

bool StatusPacket::isTest() const
{
    return generalData & StatusFlags::ROBOT_TEST;
}

bool StatusPacket::isEmergencyStop() const
{
    return generalData & StatusFlags::ROBOT_ESTOP;
}

bool StatusPacket::isBrownout() const
{
    return generalData & StatusFlags::BROWNOUT;
}

bool StatusPacket::isCodeReady() const
{
    return generalData & StatusFlags::CODE_READY;
}

double StatusPacket::getBatteryVoltage() const
{
    return batteryVoltage / 1000.0; // Convert millivolts to volts
}

void StatusPacket::fromByteArray(const QByteArray &data)
{
    if (data.size() < static_cast<int>(sizeof(StatusPacket))) {
        return;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream >> generalData;
    stream >> mode;
    stream >> batteryVoltage;
    stream >> request;
    stream >> pcmId;
    stream >> pdpId;
    stream >> pcmVersion;
    stream >> pdpVersion;
    stream >> robotCodeVersion;
}

// JoystickPacket implementation
JoystickPacket::JoystickPacket()
    : joystickId(0)
    , axisCount(0)
    , buttonCount(0)
    , buttons(0)
    , povCount(0)
{
    for (int i = 0; i < 12; ++i) {
        axes[i] = 127; // Neutral
    }
    for (int i = 0; i < 4; ++i) {
        povs[i] = 65535; // No POV
    }
}

void JoystickPacket::setAxis(int index, double value)
{
    if (index >= 0 && index < 12) {
        // Convert from -1.0 to 1.0 range to 0-255
        axes[index] = static_cast<quint8>((value + 1.0) * 127.5);
        axisCount = qMax(axisCount, static_cast<quint8>(index + 1));
    }
}

void JoystickPacket::setButton(int index, bool pressed)
{
    if (index >= 0 && index < 16) {
        if (pressed) {
            buttons |= (1 << index);
        } else {
            buttons &= ~(1 << index);
        }
        buttonCount = qMax(buttonCount, static_cast<quint8>(index + 1));
    }
}

void JoystickPacket::setPOV(int index, int angle)
{
    if (index >= 0 && index < 4) {
        if (angle >= 0 && angle < 360) {
            povs[index] = static_cast<quint16>(angle);
        } else {
            povs[index] = 65535; // No POV
        }
        povCount = qMax(povCount, static_cast<quint8>(index + 1));
    }
}

QByteArray JoystickPacket::toByteArray() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << joystickId;
    stream << axisCount;
    for (int i = 0; i < axisCount; ++i) {
        stream << axes[i];
    }
    stream << buttonCount;
    stream << buttons;
    stream << povCount;
    for (int i = 0; i < povCount; ++i) {
        stream << povs[i];
    }
    
    return data;
}

// PacketBuilder implementation
QByteArray PacketBuilder::buildControlPacket(const ControlPacket &packet)
{
    return packet.toByteArray();
}

QByteArray PacketBuilder::buildJoystickPacket(const JoystickPacket &packet)
{
    return packet.toByteArray();
}

QByteArray PacketBuilder::buildTimezonePacket(const QTimeZone &timezone)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(PacketType::TIMEZONE_DATA);
    
    QByteArray tzData = timezone.id();
    stream << static_cast<quint16>(tzData.size());
    stream.writeRawData(tzData.data(), tzData.size());
    
    return data;
}

QByteArray PacketBuilder::buildRebootPacket()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(PacketType::REBOOT);
    stream << static_cast<quint32>(0x12345678); // Magic number
    
    return data;
}

QByteArray PacketBuilder::buildVersionRequestPacket()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << static_cast<quint8>(PacketType::VERSION);
    
    return data;
}

bool PacketBuilder::parseStatusPacket(const QByteArray &data, StatusPacket &packet)
{
    if (data.size() < static_cast<int>(sizeof(StatusPacket))) {
        return false;
    }
    
    packet.fromByteArray(data);
    return true;
}

bool PacketBuilder::parseErrorPacket(const QByteArray &data, QString &error)
{
    if (data.size() < 3) {
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 type;
    stream >> type;
    
    if (type != PacketType::ERROR_DATA) {
        return false;
    }
    
    quint16 length;
    stream >> length;
    
    if (data.size() < 3 + length) {
        return false;
    }
    
    QByteArray errorData(length, 0);
    stream.readRawData(errorData.data(), length);
    error = QString::fromUtf8(errorData);
    
    return true;
}

bool PacketBuilder::parsePrintPacket(const QByteArray &data, QString &message)
{
    if (data.size() < 3) {
        return false;
    }
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 type;
    stream >> type;
    
    if (type != PacketType::PRINT_DATA) {
        return false;
    }
    
    quint16 length;
    stream >> length;
    
    if (data.size() < 3 + length) {
        return false;
    }
    
    QByteArray messageData(length, 0);
    stream.readRawData(messageData.data(), length);
    message = QString::fromUtf8(messageData);
    
    return true;
}

quint32 PacketBuilder::calculateCRC32(const QByteArray &data)
{
    // Simple CRC32 implementation
    static const quint32 crcTable[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        // ... (full CRC table would be here)
    };
    
    quint32 crc = 0xFFFFFFFF;
    for (int i = 0; i < data.size(); ++i) {
        quint8 byte = static_cast<quint8>(data[i]);
        crc = crcTable[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

bool PacketBuilder::verifyCRC32(const QByteArray &data, quint32 expectedCRC)
{
    return calculateCRC32(data) == expectedCRC;
}

// NetworkUtils implementation
QHostAddress NetworkUtils::getRobotAddress(int teamNumber)
{
    if (!isValidTeamNumber(teamNumber)) {
        return QHostAddress();
    }
    
    // Primary robot address: 10.TE.AM.2
    int team = teamNumber;
    int te = team / 100;
    int am = team % 100;
    
    return QHostAddress(QString("10.%1.%2.2").arg(te).arg(am));
}

QList<QHostAddress> NetworkUtils::getAllRobotAddresses(int teamNumber)
{
    QList<QHostAddress> addresses;
    
    if (!isValidTeamNumber(teamNumber)) {
        return addresses;
    }
    
    int team = teamNumber;
    int te = team / 100;
    int am = team % 100;
    
    // Primary competition address
    addresses << QHostAddress(QString("10.%1.%2.2").arg(te).arg(am));
    
    // USB connection
    addresses << QHostAddress("172.22.11.2");
    
    // Ethernet connection
    addresses << QHostAddress("192.168.1.2");
    
    // mDNS address (not an IP, but included for completeness)
    // addresses << QHostAddress(QString("roboRIO-%1-FRC.local").arg(team));
    
    return addresses;
}

quint16 NetworkUtils::getRobotPort()
{
    return 1110;
}

quint16 NetworkUtils::getDriverStationPort()
{
    return 1150;
}

bool NetworkUtils::isValidTeamNumber(int teamNumber)
{
    return teamNumber >= 1 && teamNumber <= 9999;
}

QString NetworkUtils::formatTeamNumber(int teamNumber)
{
    return QString::number(teamNumber);
}

QByteArray createControlPacket(const ControlData &data)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << data.packetNumber;
    stream << data.controlByte;
    stream << data.requestByte;
    stream << data.teamNumber;
    stream << data.alliance;
    stream << data.position;
    
    // Add checksum
    quint16 checksum = calculateChecksum(packet);
    stream << checksum;
    
    return packet;
}

QByteArray createStatusPacket(const StatusData &data)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream << data.packetNumber;
    stream << data.statusByte;
    stream << data.batteryHigh;
    stream << data.batteryLow;
    stream << data.brownoutProtection;
    stream << data.reserved1;
    stream << data.reserved2;
    stream << data.reserved3;
    
    // Add checksum
    quint16 checksum = calculateChecksum(packet);
    stream << checksum;
    
    return packet;
}

QByteArray createJoystickPacket(const JoystickData &data)
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Write axes
    for (int i = 0; i < 6; ++i) {
        stream << data.axes[i];
    }
    
    // Write buttons
    stream << data.buttons;
    
    // Write POVs
    for (int i = 0; i < 4; ++i) {
        stream << data.povs[i];
    }
    
    // Add checksum
    quint16 checksum = calculateChecksum(packet);
    stream << checksum;
    
    return packet;
}

QByteArray createDisablePacket()
{
    ControlData data = {};
    data.packetNumber = 0;
    data.controlByte = 0x00; // Disabled
    data.requestByte = 0x00;
    data.teamNumber = 0;
    data.alliance = 0;
    data.position = 0;
    
    return createControlPacket(data);
}

bool parseControlPacket(const QByteArray &packet, ControlData &data)
{
    if (packet.size() < 10) { // 8 bytes + 2 byte checksum
        return false;
    }
    
    if (!verifyChecksum(packet)) {
        return false;
    }
    
    QDataStream stream(packet);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream >> data.packetNumber;
    stream >> data.controlByte;
    stream >> data.requestByte;
    stream >> data.teamNumber;
    stream >> data.alliance;
    stream >> data.position;
    
    return true;
}

bool parseStatusPacket(const QByteArray &packet, StatusData &data)
{
    if (packet.size() < 10) { // 8 bytes + 2 byte checksum
        return false;
    }
    
    if (!verifyChecksum(packet)) {
        return false;
    }
    
    QDataStream stream(packet);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream >> data.packetNumber;
    stream >> data.statusByte;
    stream >> data.batteryHigh;
    stream >> data.batteryLow;
    stream >> data.brownoutProtection;
    stream >> data.reserved1;
    stream >> data.reserved2;
    stream >> data.reserved3;
    
    return true;
}

bool parseJoystickPacket(const QByteArray &packet, JoystickData &data)
{
    if (packet.size() < 14) { // 12 bytes + 2 byte checksum
        return false;
    }
    
    if (!verifyChecksum(packet)) {
        return false;
    }
    
    QDataStream stream(packet);
    stream.setByteOrder(QDataStream::BigEndian);
    
    // Read axes
    for (int i = 0; i < 6; ++i) {
        stream >> data.axes[i];
    }
    
    // Read buttons
    stream >> data.buttons;
    
    // Read POVs
    for (int i = 0; i < 4; ++i) {
        stream >> data.povs[i];
    }
    
    return true;
}

bool isValidPacket(const QByteArray &packet)
{
    return packet.size() >= 4 && verifyChecksum(packet);
}

PacketType getPacketType(const QByteArray &packet)
{
    if (packet.size() < 2) {
        return ControlPacket; // Default
    }
    
    // Packet type is typically in the second byte or determined by size/content
    if (packet.size() == 10) {
        return packet[1] & 0x80 ? StatusPacket : ControlPacket;
    } else if (packet.size() == 14) {
        return JoystickPacket;
    } else if (packet.size() == 8) {
        return DisablePacket;
    }
    
    return ControlPacket;
}

quint16 calculateChecksum(const QByteArray &data)
{
    quint16 checksum = 0;
    for (int i = 0; i < data.size(); ++i) {
        checksum += static_cast<quint8>(data[i]);
    }
    return checksum;
}

bool verifyChecksum(const QByteArray &packet)
{
    if (packet.size() < 2) {
        return false;
    }
    
    // Extract checksum from last 2 bytes
    QDataStream stream(packet.right(2));
    stream.setByteOrder(QDataStream::BigEndian);
    quint16 receivedChecksum;
    stream >> receivedChecksum;
    
    // Calculate checksum of data (excluding checksum bytes)
    QByteArray data = packet.left(packet.size() - 2);
    quint16 calculatedChecksum = calculateChecksum(data);
    
    return receivedChecksum == calculatedChecksum;
}

QHostAddress getTeamAddress(int teamNumber, int device)
{
    if (!isValidTeamNumber(teamNumber)) {
        return QHostAddress();
    }
    
    int firstOctet = teamNumber / 100;
    int secondOctet = teamNumber % 100;
    
    return QHostAddress(QString("10.%1.%2.%3").arg(firstOctet).arg(secondOctet).arg(device));
}

bool isValidTeamNumber(int teamNumber)
{
    return teamNumber > 0 && teamNumber <= 9999;
}

QString formatTeamAddress(int teamNumber, int device)
{
    QHostAddress addr = getTeamAddress(teamNumber, device);
    return addr.isNull() ? QString() : addr.toString();
}

} // namespace RobotPackets
