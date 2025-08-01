import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FRC.Backend 1.0

Dialog {
    id: systemDetailsDialog
    title: "System Details"
    modal: true
    width: 500
    height: 400
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 10
        
        ColumnLayout {
            width: parent.width
            spacing: 10
            
            GroupBox {
                title: "Application"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Label { text: "Version:" }
                    Label { text: "2024.1.0" }
                    
                    Label { text: "Build Date:" }
                    Label { text: "2024-01-15" }
                    
                    Label { text: "Qt Version:" }
                    Label { text: "5.15.2" }
                }
            }
            
            GroupBox {
                title: "System"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Label { text: "OS:" }
                    Label { text: Qt.platform.os }
                    
                    Label { text: "Architecture:" }
                    Label { text: "x64" }
                }
            }
            
            GroupBox {
                title: "Features"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Label { text: "FMS Support:" }
                    Label { text: enableFMSSupport ? "Enabled" : "Disabled" }
                    
                    Label { text: "Glass Integration:" }
                    Label { text: enableGlassIntegration ? "Enabled" : "Disabled" }
                    
                    Label { text: "Dashboard Management:" }
                    Label { text: enableDashboardManagement ? "Enabled" : "Disabled" }
                    
                    Label { text: "Practice Match:" }
                    Label { text: enablePracticeMatch ? "Enabled" : "Disabled" }
                }
            }
            
            // System Status Cards
            GridLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                columns: 2
                columnSpacing: 15
                rowSpacing: 15
                
                // Battery Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#404040"
                    radius: 8
                    border.color: "#606060"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        RowLayout {
                            Label {
                                text: "🔋"
                                font.pixelSize: 20
                            }
                            
                            Label {
                                text: "Battery"
                                font.pixelSize: 16
                                font.bold: true
                                color: "white"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Label {
                                text: robotState.robotVoltage.toFixed(2) + "V"
                                font.pixelSize: 18
                                font.bold: true
                                color: {
                                    if (robotState.batteryCriticalActive) return "#f44336"
                                    if (robotState.batteryWarningActive) return "#ff9800"
                                    if (robotState.robotVoltage > robotState.batteryWarningThreshold) return "limegreen"
                                    return "white"
                                }
                            }
                        }
                        
                        Label {
                            text: "Status: " + robotState.batteryStatus
                            color: "lightgray"
                            font.pixelSize: 12
                        }
                        
                        ProgressBar {
                            Layout.fillWidth: true
                            value: Math.max(0, Math.min(1, (robotState.robotVoltage - 9.0) / (13.0 - 9.0)))
                            
                            background: Rectangle {
                                color: "#606060"
                                radius: 4
                            }
                            
                            contentItem: Item {
                                Rectangle {
                                    width: parent.width * parent.parent.value
                                    height: parent.height
                                    color: {
                                        if (robotState.batteryCriticalActive) return "#f44336"
                                        if (robotState.batteryWarningActive) return "#ff9800"
                                        if (robotState.robotVoltage > robotState.batteryWarningThreshold) return "#4CAF50"
                                        return "#ff9800"
                                    }
                                    radius: 4
                                }
                            }
                        }
                        
                        Label {
                            text: robotState.robotVoltage > 0.1 ? 
                                  "Range: 9.0V - 13.0V" : 
                                  "No battery data available"
                            color: "gray"
                            font.pixelSize: 10
                        }
                    }
                }
                
                // RAM Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#404040"
                    radius: 8
                    border.color: "#606060"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        RowLayout {
                            Label {
                                text: "🧠"
                                font.pixelSize: 20
                            }
                            
                            Label {
                                text: "Memory (RAM)"
                                font.pixelSize: 16
                                font.bold: true
                                color: "white"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Label {
                                text: robotState.ramUsage + "%"
                                font.pixelSize: 18
                                font.bold: true
                                color: {
                                    if (robotState.ramUsage >= 90) return "#f44336"
                                    if (robotState.ramUsage >= 80) return "#ff9800"
                                    if (robotState.ramUsage >= 60) return "#ffc107"
                                    return "#4CAF50"
                                }
                            }
                        }
                        
                        Label {
                            text: "Status: " + {
                                if (robotState.ramUsage >= 90) return "Critical"
                                if (robotState.ramUsage >= 80) return "Warning"
                                if (robotState.ramUsage >= 60) return "Moderate"
                                return "Normal"
                            }
                            color: "lightgray"
                            font.pixelSize: 12
                        }
                        
                        ProgressBar {
                            Layout.fillWidth: true
                            value: robotState.ramUsage / 100.0
                            
                            background: Rectangle {
                                color: "#606060"
                                radius: 4
                            }
                            
                            contentItem: Item {
                                Rectangle {
                                    width: parent.width * parent.parent.value
                                    height: parent.height
                                    color: {
                                        if (robotState.ramUsage >= 90) return "#f44336"
                                        if (robotState.ramUsage >= 80) return "#ff9800"
                                        if (robotState.ramUsage >= 60) return "#ffc107"
                                        return "#4CAF50"
                                    }
                                    radius: 4
                                }
                            }
                        }
                        
                        Label {
                            text: robotState.ramUsage > 0 ? 
                                  "Recommended: Keep below 80%" : 
                                  "No memory data available"
                            color: "gray"
                            font.pixelSize: 10
                        }
                    }
                }
                
                // Storage Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#404040"
                    radius: 8
                    border.color: "#606060"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        RowLayout {
                            Label {
                                text: "💾"
                                font.pixelSize: 20
                            }
                            
                            Label {
                                text: "Storage"
                                font.pixelSize: 16
                                font.bold: true
                                color: "white"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Label {
                                text: robotState.diskUsage + "%"
                                font.pixelSize: 18
                                font.bold: true
                                color: {
                                    if (robotState.diskUsage >= 95) return "#f44336"
                                    if (robotState.diskUsage >= 85) return "#ff9800"
                                    if (robotState.diskUsage >= 75) return "#ffc107"
                                    return "#4CAF50"
                                }
                            }
                        }
                        
                        Label {
                            text: "Status: " + {
                                if (robotState.diskUsage >= 95) return "Nearly Full"
                                if (robotState.diskUsage >= 85) return "Low Space"
                                if (robotState.diskUsage >= 75) return "Moderate"
                                return "Plenty of Space"
                            }
                            color: "lightgray"
                            font.pixelSize: 12
                        }
                        
                        ProgressBar {
                            Layout.fillWidth: true
                            value: robotState.diskUsage / 100.0
                            
                            background: Rectangle {
                                color: "#606060"
                                radius: 4
                            }
                            
                            contentItem: Item {
                                Rectangle {
                                    width: parent.width * parent.parent.value
                                    height: parent.height
                                    color: {
                                        if (robotState.diskUsage >= 95) return "#f44336"
                                        if (robotState.diskUsage >= 85) return "#ff9800"
                                        if (robotState.diskUsage >= 75) return "#ffc107"
                                        return "#4CAF50"
                                    }
                                    radius: 4
                                }
                            }
                        }
                        
                        Label {
                            text: robotState.diskUsage > 0 ? 
                                  "Recommended: Keep below 85%" : 
                                  "No storage data available"
                            color: "gray"
                            font.pixelSize: 10
                        }
                    }
                }
                
                // CPU Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#404040"
                    radius: 8
                    border.color: "#606060"
                    border.width: 1
                    
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 10
                        
                        RowLayout {
                            Label {
                                text: "⚡"
                                font.pixelSize: 20
                            }
                            
                            Label {
                                text: "CPU Usage"
                                font.pixelSize: 16
                                font.bold: true
                                color: "white"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Label {
                                text: robotState.cpuUsage + "%"
                                font.pixelSize: 18
                                font.bold: true
                                color: {
                                    if (robotState.cpuUsage >= 90) return "#f44336"
                                    if (robotState.cpuUsage >= 80) return "#ff9800"
                                    if (robotState.cpuUsage >= 60) return "#ffc107"
                                    return "#4CAF50"
                                }
                            }
                        }
                        
                        Label {
                            text: "Status: " + {
                                if (robotState.cpuUsage >= 90) return "Overloaded"
                                if (robotState.cpuUsage >= 80) return "High Load"
                                if (robotState.cpuUsage >= 60) return "Moderate Load"
                                return "Normal Load"
                            }
                            color: "lightgray"
                            font.pixelSize: 12
                        }
                        
                        ProgressBar {
                            Layout.fillWidth: true
                            value: robotState.cpuUsage / 100.0
                            
                            background: Rectangle {
                                color: "#606060"
                                radius: 4
                            }
                            
                            contentItem: Item {
                                Rectangle {
                                    width: parent.width * parent.parent.value
                                    height: parent.height
                                    color: {
                                        if (robotState.cpuUsage >= 90) return "#f44336"
                                        if (robotState.cpuUsage >= 80) return "#ff9800"
                                        if (robotState.cpuUsage >= 60) return "#ffc107"
                                        return "#4CAF50"
                                    }
                                    radius: 4
                                }
                            }
                        }
                        
                        Label {
                            text: robotState.cpuUsage > 0 ? 
                                  "Recommended: Keep below 80%" : 
                                  "No CPU data available"
                            color: "gray"
                            font.pixelSize: 10
                        }
                    }
                }
            }
            
            // System Recommendations
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                color: "#404040"
                radius: 8
                border.color: "#606060"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 5
                    
                    Label {
                        text: "💡 System Recommendations"
                        font.pixelSize: 14
                        font.bold: true
                        color: "lightblue"
                    }
                    
                    Label {
                        text: {
                            var recommendations = []
                            if (robotState.batteryCriticalActive || robotState.batteryWarningActive) {
                                recommendations.push("• Replace or charge robot battery")
                            }
                            if (robotState.ramUsage >= 80) {
                                recommendations.push("• Restart robot code to free memory")
                            }
                            if (robotState.diskUsage >= 85) {
                                recommendations.push("• Free up storage space by removing old logs")
                            }
                            if (robotState.cpuUsage >= 80) {
                                recommendations.push("• Check for runaway processes or optimize robot code")
                            }
                            if (recommendations.length === 0) {
                                return "✅ All systems operating normally"
                            }
                            return recommendations.join("\n")
                        }
                        color: "white"
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
    
    Button {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        text: "Close"
        onClicked: systemDetailsDialog.close()
    }
}
