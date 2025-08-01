#include "packets.h"
#include <QDataStream>
#include <QDebug>

using namespace FRCDriverStation::Protocol;

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
