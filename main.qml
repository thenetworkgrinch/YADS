import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import FRCDriverStation 1.0

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: "Yet Another Driver Station"
    
    property bool darkMode: false
    
    // Global keyboard shortcuts
    Shortcut {
        sequence: "Space"
        onActivated: {
            if (robotState.enabled) {
                robotState.disableRobot()
            } else {
                robotState.enableRobot()
            }
        }
    }
    
    Shortcut {
        sequence: "Enter"
        onActivated: robotState.emergencyStopRobot()
    }
    
    Shortcut {
        sequence: "Escape"
        onActivated: robotState.clearEmergencyStop()
    }
    
    // Main layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header Section
        Rectangle {
            id: header
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: darkMode ? "#2b2b2b" : "#1976D2"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                
                Text {
                    text: "Yet Another Driver Station"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                }
                
                Item { Layout.fillWidth: true }
                
                Switch {
                    id: darkModeSwitch
                    text: "Dark Mode"
                    checked: darkMode
                    onCheckedChanged: darkMode = checked
                }
            }
        }
        
        // Connection Configuration Section
        Rectangle {
            id: connectionSection
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: darkMode ? "#2d2d2d" : "#f5f5f5"
            border.color: robotState.robotConnected ? "#4CAF50" : (darkMode ? "#555" : "#ddd")
            border.width: 2
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 10
                
                // Connection Mode Toggle
                RowLayout {
                    spacing: 20
                    
                    Label {
                        text: "Connection Mode:"
                        color: darkMode ? "white" : "black"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    RadioButton {
                        id: teamNumberMode
                        text: "Team Number"
                        checked: robotState.connectionMode === 0
                        onCheckedChanged: {
                            if (checked) {
                                robotState.setConnectionMode(0)
                            }
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: darkMode ? "white" : "black"
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    RadioButton {
                        id: ipAddressMode
                        text: "IP Address"
                        checked: robotState.connectionMode === 1
                        onCheckedChanged: {
                            if (checked) {
                                robotState.setConnectionMode(1)
                            }
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: darkMode ? "white" : "black"
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                // Input Section
                RowLayout {
                    spacing: 15
                    
                    // Team Number Input
                    RowLayout {
                        visible: teamNumberMode.checked
                        spacing: 10
                        
                        Label {
                            text: "Team Number:"
                            color: darkMode ? "white" : "black"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        TextField {
                            id: teamNumberInput
                            Layout.preferredWidth: 120
                            text: robotState.teamNumber > 0 ? robotState.teamNumber.toString() : ""
                            placeholderText: "0000"
                            font.pixelSize: 16
                            horizontalAlignment: TextInput.AlignHCenter
                            validator: IntValidator { bottom: 1; top: 9999 }
                            
                            background: Rectangle {
                                color: darkMode ? "#404040" : "white"
                                border.color: parent.activeFocus ? "#4CAF50" : (darkMode ? "#666" : "#ccc")
                                border.width: 2
                                radius: 4
                            }
                            
                            color: darkMode ? "white" : "black"
                            selectionColor: "#4CAF50"
                            selectedTextColor: "white"
                            
                            onTextChanged: {
                                if (text.length > 0) {
                                    const teamNum = parseInt(text)
                                    if (teamNum > 0 && teamNum <= 9999) {
                                        robotState.setTeamNumber(teamNum)
                                    }
                                }
                            }
                            
                            onEditingFinished: {
                                if (text.length === 0) {
                                    robotState.setTeamNumber(0)
                                }
                            }
                        }
                    }
                    
                    // IP Address Input
                    RowLayout {
                        visible: ipAddressMode.checked
                        spacing: 10
                        
                        Label {
                            text: "Robot IP Address:"
                            color: darkMode ? "white" : "black"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        TextField {
                            id: ipAddressInput
                            Layout.preferredWidth: 180
                            text: robotState.robotIpAddress
                            placeholderText: "10.XX.XX.2"
                            font.pixelSize: 16
                            
                            background: Rectangle {
                                color: darkMode ? "#404040" : "white"
                                border.color: parent.activeFocus ? "#4CAF50" : (darkMode ? "#666" : "#ccc")
                                border.width: 2
                                radius: 4
                            }
                            
                            color: darkMode ? "white" : "black"
                            selectionColor: "#4CAF50"
                            selectedTextColor: "white"
                            
                            // Simple IP validation regex
                            validator: RegExpValidator {
                                regExp: /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
                            }
                            
                            onTextChanged: {
                                if (acceptableInput) {
                                    robotState.setRobotIpAddress(text)
                                }
                            }
                        }
                    }
                    
                    // Status Indicator
                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        color: {
                            if (teamNumberMode.checked && robotState.teamNumber === 0) return "#f44336"
                            if (ipAddressMode.checked && robotState.robotIpAddress === "") return "#f44336"
                            if (robotState.robotConnected) return "#4CAF50"
                            return "#FF9800"
                        }
                        
                        SequentialAnimation on opacity {
                            running: !robotState.robotConnected && 
                                    ((teamNumberMode.checked && robotState.teamNumber > 0) ||
                                     (ipAddressMode.checked && robotState.robotIpAddress !== ""))
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 500 }
                            NumberAnimation { to: 1.0; duration: 500 }
                        }
                    }
                    
                    // Status Text
                    Label {
                        text: {
                            if (teamNumberMode.checked) {
                                if (robotState.teamNumber === 0) return "No team number set"
                                if (robotState.robotConnected) return `Connected to Team ${robotState.teamNumber}`
                                return `Team ${robotState.teamNumber} - Not connected`
                            } else {
                                if (robotState.robotIpAddress === "") return "No IP address set"
                                if (robotState.robotConnected) return `Connected to ${robotState.robotIpAddress}`
                                return `${robotState.robotIpAddress} - Not connected`
                            }
                        }
                        color: {
                            if ((teamNumberMode.checked && robotState.teamNumber === 0) ||
                                (ipAddressMode.checked && robotState.robotIpAddress === "")) return "#f44336"
                            if (robotState.robotConnected) return "#4CAF50"
                            return "#FF9800"
                        }
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    // Connection Button
                    Button {
                        text: robotState.robotConnected ? "Disconnect" : "Connect"
                        enabled: (teamNumberMode.checked && robotState.teamNumber > 0) ||
                                (ipAddressMode.checked && robotState.robotIpAddress !== "")
                        
                        onClicked: {
                            if (robotState.robotConnected) {
                                robotState.disconnectFromRobot()
                            } else {
                                robotState.connectToRobot()
                            }
                        }
                        
                        background: Rectangle {
                            color: {
                                if (!parent.enabled) return darkMode ? "#666" : "#ccc"
                                if (robotState.robotConnected) {
                                    return parent.pressed ? "#d32f2f" : "#f44336"
                                } else {
                                    return parent.pressed ? "#388E3C" : "#4CAF50"
                                }
                            }
                            radius: 6
                            border.color: darkMode ? "#555" : "#ddd"
                            border.width: 1
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "white" : (darkMode ? "#999" : "#666")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }
                }
            }
        }
        
        // Main Content Area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            
            // Left sidebar
            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: darkMode ? "#2b2b2b" : "#f5f5f5"
                border.color: darkMode ? "#404040" : "#d0d0d0"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    
                    // Robot control
                    GroupBox {
                        title: "Robot Control"
                        Layout.fillWidth: true
                        
                        background: Rectangle {
                            color: darkMode ? "#333" : "white"
                            border.color: darkMode ? "#555" : "#ddd"
                            border.width: 1
                            radius: 4
                        }
                        
                        label: Label {
                            text: parent.title
                            color: darkMode ? "white" : "black"
                            font.bold: true
                        }
                        
                        ColumnLayout {
                            anchors.fill: parent
                            
                            Button {
                                text: robotState.enabled ? "DISABLE" : "ENABLE"
                                Layout.fillWidth: true
                                enabled: !robotState.emergencyStop && robotState.robotConnected
                                onClicked: {
                                    if (robotState.enabled) {
                                        robotState.disableRobot()
                                    } else {
                                        robotState.enableRobot()
                                    }
                                }
                                
                                background: Rectangle {
                                    color: {
                                        if (!parent.enabled) return darkMode ? "#666" : "#ccc"
                                        return robotState.enabled ? "#ff4444" : "#44ff44"
                                    }
                                    border.color: "#888888"
                                    border.width: 1
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: parent.enabled ? "white" : (darkMode ? "#999" : "#666")
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.bold: true
                                }
                            }
                            
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["Teleoperated", "Autonomous", "Test"]
                                currentIndex: robotState.robotMode === 0 ? 0 : robotState.robotMode - 1
                                onCurrentIndexChanged: {
                                    if (currentIndex === 0) robotState.robotMode = 2  // Teleoperated
                                    else robotState.robotMode = currentIndex
                                }
                                
                                background: Rectangle {
                                    color: darkMode ? "#404040" : "white"
                                    border.color: darkMode ? "#666" : "#ccc"
                                    border.width: 1
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.displayText
                                    color: darkMode ? "white" : "black"
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 10
                                }
                            }
                            
                            Button {
                                text: robotState.emergencyStop ? "CLEAR E-STOP" : "EMERGENCY STOP"
                                Layout.fillWidth: true
                                onClicked: {
                                    if (robotState.emergencyStop) {
                                        robotState.clearEmergencyStop()
                                    } else {
                                        robotState.emergencyStopRobot()
                                    }
                                }
                                
                                background: Rectangle {
                                    color: robotState.emergencyStop ? "#ffaa00" : "#ff0000"
                                    border.color: "#888888"
                                    border.width: 1
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.bold: true
                                }
                            }
                        }
                    }
                    
                    // Status indicators
                    GroupBox {
                        title: "Status"
                        Layout.fillWidth: true
                        
                        background: Rectangle {
                            color: darkMode ? "#333" : "white"
                            border.color: darkMode ? "#555" : "#ddd"
                            border.width: 1
                            radius: 4
                        }
                        
                        label: Label {
                            text: parent.title
                            color: darkMode ? "white" : "black"
                            font.bold: true
                        }
                        
                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            
                            Label { 
                                text: "Communications:"
                                color: darkMode ? "white" : "black"
                            }
                            Label { 
                                text: robotState.commsStatus
                                color: robotState.commsStatus.includes("Connected") ? "green" : "red"
                            }
                            
                            Label { 
                                text: "Robot Code:"
                                color: darkMode ? "white" : "black"
                            }
                            Label { 
                                text: robotState.robotCodeStatus
                                color: robotState.robotCodeStatus.includes("Robot Code") ? "green" : "red"
                            }
                            
                            Label { 
                                text: "Controllers:"
                                color: darkMode ? "white" : "black"
                            }
                            Label { 
                                text: robotState.joystickStatus
                                color: robotState.joystickStatus.includes("Bound") ? "green" : "orange"
                            }
                            
                            Label { 
                                text: "Battery:"
                                color: darkMode ? "white" : "black"
                            }
                            Label { 
                                text: batteryManager.batteryStatus
                                color: {
                                    if (batteryManager.batteryStatus.includes("CRITICAL")) return "red"
                                    if (batteryManager.batteryStatus.includes("Warning")) return "orange"
                                    return "green"
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true } // Spacer
                }
            }
            
            // Main content area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: darkMode ? "#1e1e1e" : "#ffffff"
                
                TabView {
                    anchors.fill: parent
                    anchors.margins: 10
                    
                    Tab {
                        title: "Operations"
                        OperationsView {
                            anchors.fill: parent
                        }
                    }
                    
                    Tab {
                        title: "Diagnostics"
                        DiagnosticsView {
                            anchors.fill: parent
                        }
                    }
                    
                    Tab {
                        title: "Controllers"
                        ControllersView {
                            anchors.fill: parent
                        }
                    }
                    
                    Tab {
                        title: "Charts"
                        ChartsView {
                            anchors.fill: parent
                        }
                    }
                    
                    Tab {
                        title: "Console"
                        ScrollView {
                            anchors.fill: parent
                            
                            TextArea {
                                text: robotState.consoleOutput
                                readOnly: true
                                selectByMouse: true
                                wrapMode: TextArea.Wrap
                                font.family: "Consolas, Monaco, monospace"
                                font.pixelSize: 12
                                color: darkMode ? "white" : "black"
                                
                                background: Rectangle {
                                    color: darkMode ? "#1e1e1e" : "#ffffff"
                                    border.color: darkMode ? "#555" : "#cccccc"
                                    border.width: 1
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Status bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: darkMode ? "#2b2b2b" : "#f0f0f0"
            border.color: darkMode ? "#404040" : "#d0d0d0"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 5
                
                Label {
                    text: teamNumberMode.checked ? `Team ${robotState.teamNumber}` : robotState.robotIpAddress
                    font.bold: true
                    color: darkMode ? "white" : "black"
                }
                
                Rectangle {
                    width: 1
                    height: 20
                    color: darkMode ? "#555" : "#cccccc"
                }
                
                Label {
                    text: robotState.enabled ? "ENABLED" : "DISABLED"
                    color: robotState.enabled ? "green" : "red"
                    font.bold: true
                }
                
                Rectangle {
                    width: 1
                    height: 20
                    color: darkMode ? "#555" : "#cccccc"
                }
                
                Label {
                    text: `${robotState.robotVoltage.toFixed(2)}V`
                    color: {
                        if (robotState.robotVoltage < 6.0) return "red"
                        if (robotState.robotVoltage < 7.0) return "orange"
                        return "green"
                    }
                }
                
                Item { Layout.fillWidth: true } // Spacer
                
                Label {
                    text: `Latency: ${robotState.networkLatency.toFixed(1)}ms`
                    color: darkMode ? "white" : "black"
                }
                
                Rectangle {
                    width: 1
                    height: 20
                    color: darkMode ? "#555" : "#cccccc"
                }
                
                Label {
                    text: `Loss: ${robotState.packetLoss.toFixed(1)}%`
                    color: robotState.packetLoss > 10 ? "red" : "green"
                }
            }
        }
    }
}
