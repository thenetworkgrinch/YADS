#include "packets.h"
#include "../../core/logger.h"
#include <QMutexLocker>
#include <QDebug>
#include <cstring>

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
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
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
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
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
