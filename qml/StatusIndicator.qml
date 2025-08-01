import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FRC.Backend 1.0

RowLayout {
    spacing: 8
    
    // Communications Status
    Row {
        property string label: "Communications"
        property string status: robotState.commsStatus
        property color statusColor: {
            if (status.toLowerCase().includes("connected")) return "limegreen"
            if (status.toLowerCase().includes("no comms")) return "red"
            return "orange"
        }
        
        spacing: 5
        
        Text {
            text: label
            anchors.verticalCenter: parent.verticalCenter
        }
        
        SystemStatusIndicator {
            status: parent.status
            statusColor: parent.statusColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    // Robot Code Status
    Row {
        property string label: "Robot Code"
        property string status: robotState.robotCodeStatus
        property color statusColor: {
            if (status.toLowerCase().includes("robot code")) return "limegreen"
            return "red"
        }
        
        spacing: 5
        
        Text {
            text: label
            anchors.verticalCenter: parent.verticalCenter
        }
        
        SystemStatusIndicator {
            status: parent.status
            statusColor: parent.statusColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    // Controllers Status
    Row {
        property string label: "Controllers"
        property string status: robotState.joystickStatus
        property color statusColor: {
            if (status.toLowerCase().includes("bound")) return "limegreen"
            return "red"
        }
        
        spacing: 5
        
        Text {
            text: label
            anchors.verticalCenter: parent.verticalCenter
        }
        
        SystemStatusIndicator {
            status: parent.status
            statusColor: parent.statusColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
