import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FRC.Backend 1.0

RowLayout {
    spacing: 8
    
    // Battery Status
    Rectangle {
        property string status: "Battery: " + robotState.robotVoltage.toFixed(2) + "V"
        property color statusColor: {
            if (robotState.batteryCriticalActive) return "#f44336"
            if (robotState.batteryWarningActive) return "#ff9800"
            if (robotState.robotVoltage > robotState.batteryWarningThreshold) return "limegreen"
            if (robotState.robotVoltage > robotState.batteryCriticalThreshold) return "orange"
            return "red"
        }
        
        width: 80
        height: 40
        color: "#1e1e1e"
        border.color: Qt.darker(statusColor, 1.2)
        border.width: robotState.batteryWarningActive || robotState.batteryCriticalActive ? 2 : 1
        radius: 4
        
        ToolTip.visible: mouseArea.containsMouse
        ToolTip.text: status
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: batterySettingsDialog.open()
            cursorShape: Qt.PointingHandCursor
        }
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            
            RowLayout {
                spacing: 3
                Layout.alignment: Qt.AlignHCenter
                
                Label {
                    text: "🔋"
                    font.pixelSize: 12
                }
                
                Label {
                    text: "Battery"
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
            
            Label {
                text: robotState.robotVoltage.toFixed(1) + "V"
                color: statusColor
                font.pixelSize: 11
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
    
    // RAM Usage
    Rectangle {
        property string status: "RAM Usage: " + robotState.ramUsage + "%"
        property color statusColor: {
            if (robotState.ramUsage >= 90) return "#f44336"
            if (robotState.ramUsage >= 80) return "#ff9800"
            if (robotState.ramUsage >= 60) return "#ffc107"
            return "limegreen"
        }
        
        width: 80
        height: 40
        color: "#1e1e1e"
        border.color: Qt.darker(statusColor, 1.2)
        border.width: 1
        radius: 4
        
        ToolTip.visible: mouseArea.containsMouse
        ToolTip.text: status
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: systemDetailsDialog.open()
            cursorShape: Qt.PointingHandCursor
        }
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            
            RowLayout {
                spacing: 3
                Layout.alignment: Qt.AlignHCenter
                
                Label {
                    text: "🧠"
                    font.pixelSize: 12
                }
                
                Label {
                    text: "RAM"
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
            
            Label {
                text: robotState.ramUsage + "%"
                color: statusColor
                font.pixelSize: 11
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
    
    // Storage Usage
    Rectangle {
        property string status: "Disk Usage: " + robotState.diskUsage + "%"
        property color statusColor: {
            if (robotState.diskUsage >= 95) return "#f44336"
            if (robotState.diskUsage >= 85) return "#ff9800"
            if (robotState.diskUsage >= 75) return "#ffc107"
            return "limegreen"
        }
        
        width: 80
        height: 40
        color: "#1e1e1e"
        border.color: Qt.darker(statusColor, 1.2)
        border.width: 1
        radius: 4
        
        ToolTip.visible: mouseArea.containsMouse
        ToolTip.text: status
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: systemDetailsDialog.open()
            cursorShape: Qt.PointingHandCursor
        }
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            
            RowLayout {
                spacing: 3
                Layout.alignment: Qt.AlignHCenter
                
                Label {
                    text: "💾"
                    font.pixelSize: 12
                }
                
                Label {
                    text: "Disk"
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
            
            Label {
                text: robotState.diskUsage + "%"
                color: statusColor
                font.pixelSize: 11
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
    
    // CPU Usage
    Rectangle {
        property string status: "CPU Usage: " + robotState.cpuUsage + "%"
        property color statusColor: {
            if (robotState.cpuUsage >= 90) return "#f44336"
            if (robotState.cpuUsage >= 80) return "#ff9800"
            if (robotState.cpuUsage >= 60) return "#ffc107"
            return "limegreen"
        }
        
        width: 80
        height: 40
        color: "#1e1e1e"
        border.color: Qt.darker(statusColor, 1.2)
        border.width: 1
        radius: 4
        
        ToolTip.visible: mouseArea.containsMouse
        ToolTip.text: status
        
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: systemDetailsDialog.open()
            cursorShape: Qt.PointingHandCursor
        }
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            
            RowLayout {
                spacing: 3
                Layout.alignment: Qt.AlignHCenter
                
                Label {
                    text: "⚡"
                    font.pixelSize: 12
                }
                
                Label {
                    text: "CPU"
                    color: "white"
                    font.pixelSize: 9
                    font.bold: true
                }
            }
            
            Label {
                text: robotState.cpuUsage + "%"
                color: statusColor
                font.pixelSize: 11
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
