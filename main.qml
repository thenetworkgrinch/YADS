import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: "Yet Another Driver Station"
    
    property bool darkMode: true
    property alias robotState: robotStateItem
    
    // Color scheme
    property color backgroundColor: darkMode ? "#1e1e1e" : "#ffffff"
    property color surfaceColor: darkMode ? "#2d2d2d" : "#f5f5f5"
    property color primaryColor: "#4CAF50"
    property color secondaryColor: "#2196F3"
    property color errorColor: "#f44336"
    property color warningColor: "#FF9800"
    property color textPrimaryColor: darkMode ? "#ffffff" : "#000000"
    property color textSecondaryColor: darkMode ? "#b0b0b0" : "#666666"
    property color borderColor: darkMode ? "#555555" : "#cccccc"
    
    // Robot state object
    QtObject {
        id: robotStateItem
        // This would be connected to the C++ RobotState object
        property bool robotEnabled: false
        property bool robotConnected: false
        property bool emergencyStop: false
        property int teamNumber: 0
        property string robotIpAddress: ""
        property int connectionMode: 0 // 0 = Team Number, 1 = IP Address
        property string robotMode: "Disabled"
        property real batteryVoltage: 0.0
        property int pingLatency: -1
        property bool globalShortcutsEnabled: true
        property string commsStatus: "Disconnected"
        property string robotCodeStatus: "No Robot Code"
        property string joystickStatus: "No Controllers"
        property real robotVoltage: 0.0
        property real networkLatency: 0.0
        property real packetLoss: 0.0
        property string consoleOutput: ""
    }
    
    color: backgroundColor
    
    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Header with team number/IP input and connection controls
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: surfaceColor
            border.color: borderColor
            border.width: 1
            radius: 8
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15
                
                // Connection mode toggle
                Column {
                    spacing: 5
                    
                    Text {
                        text: "Connection Mode"
                        color: textSecondaryColor
                        font.pixelSize: 12
                    }
                    
                    Row {
                        spacing: 10
                        
                        RadioButton {
                            id: teamNumberRadio
                            text: "Team Number"
                            checked: robotState.connectionMode === 0
                            onCheckedChanged: {
                                if (checked) {
                                    robotState.connectionMode = 0
                                }
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: textPrimaryColor
                                leftPadding: parent.indicator.width + parent.spacing
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: ipAddressRadio
                            text: "IP Address"
                            checked: robotState.connectionMode === 1
                            onCheckedChanged: {
                                if (checked) {
                                    robotState.connectionMode = 1
                                }
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: textPrimaryColor
                                leftPadding: parent.indicator.width + parent.spacing
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
                
                // Team number input (visible when team number mode is selected)
                Column {
                    visible: robotState.connectionMode === 0
                    spacing: 5
                    
                    Text {
                        text: "Team Number"
                        color: textSecondaryColor
                        font.pixelSize: 12
                    }
                    
                    TextField {
                        id: teamNumberField
                        width: 120
                        height: 35
                        text: robotState.teamNumber > 0 ? robotState.teamNumber.toString() : ""
                        placeholderText: "0000"
                        validator: IntValidator { bottom: 1; top: 9999 }
                        selectByMouse: true
                        
                        background: Rectangle {
                            color: darkMode ? "#3d3d3d" : "#ffffff"
                            border.color: parent.activeFocus ? primaryColor : borderColor
                            border.width: parent.activeFocus ? 2 : 1
                            radius: 4
                        }
                        
                        color: textPrimaryColor
                        
                        onTextChanged: {
                            if (text.length > 0) {
                                let num = parseInt(text)
                                if (!isNaN(num) && num >= 1 && num <= 9999) {
                                    robotState.teamNumber = num
                                }
                            }
                        }
                    }
                }
                
                // IP address input (visible when IP address mode is selected)
                Column {
                    visible: robotState.connectionMode === 1
                    spacing: 5
                    
                    Text {
                        text: "Robot IP Address"
                        color: textSecondaryColor
                        font.pixelSize: 12
                    }
                    
                    TextField {
                        id: ipAddressField
                        width: 150
                        height: 35
                        text: robotState.robotIpAddress
                        placeholderText: "10.0.0.2"
                        selectByMouse: true
                        
                        // IP address validation regex
                        validator: RegularExpressionValidator {
                            regularExpression: /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
                        }
                        
                        background: Rectangle {
                            color: darkMode ? "#3d3d3d" : "#ffffff"
                            border.color: parent.activeFocus ? primaryColor : 
                                         (parent.acceptableInput ? borderColor : errorColor)
                            border.width: parent.activeFocus ? 2 : 1
                            radius: 4
                        }
                        
                        color: textPrimaryColor
                        
                        onTextChanged: {
                            if (acceptableInput) {
                                robotState.robotIpAddress = text
                            }
                        }
                    }
                }
                
                // Connection status indicator
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: {
                        if (robotState.robotConnected) return primaryColor
                        if (robotState.commsStatus === "Connecting...") return warningColor
                        return errorColor
                    }
                    
                    SequentialAnimation on opacity {
                        running: robotState.commsStatus === "Connecting..."
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 500 }
                        NumberAnimation { to: 1.0; duration: 500 }
                    }
                }
                
                Text {
                    text: robotState.commsStatus
                    color: textPrimaryColor
                    font.pixelSize: 14
                    font.bold: true
                }
                
                Item { Layout.fillWidth: true } // Spacer
                
                // Connection controls
                Row {
                    spacing: 10
                    
                    Button {
                        text: robotState.robotConnected ? "Disconnect" : "Connect"
                        enabled: (robotState.connectionMode === 0 && robotState.teamNumber > 0) ||
                                (robotState.connectionMode === 1 && robotState.robotIpAddress.length > 0)
                        
                        background: Rectangle {
                            color: parent.pressed ? Qt.darker(secondaryColor, 1.2) :
                                   parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor
                            radius: 4
                            opacity: parent.enabled ? 1.0 : 0.5
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                            font.bold: true
                        }
                        
                        onClicked: {
                            if (robotState.robotConnected) {
                                // Disconnect logic would be implemented in C++
                                console.log("Disconnecting from robot")
                            } else {
                                // Connect logic would be implemented in C++
                                console.log("Connecting to robot")
                            }
                        }
                    }
                    
                    Button {
                        text: "Restart"
                        
                        background: Rectangle {
                            color: parent.pressed ? Qt.darker(warningColor, 1.2) :
                                   parent.hovered ? Qt.lighter(warningColor, 1.1) : warningColor
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 12
                            font.bold: true
                        }
                        
                        onClicked: {
                            console.log("Restarting communication")
                        }
                    }
                }
            }
        }
        
        // Main content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
            
            // Left panel - Robot control and status
            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                color: surfaceColor
                border.color: borderColor
                border.width: 1
                radius: 8
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15
                    
                    // Robot control section
                    Column {
                        Layout.fillWidth: true
                        spacing: 10
                        
                        Text {
                            text: "Robot Control"
                            color: textPrimaryColor
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        // Emergency stop button
                        Button {
                            width: parent.width
                            height: 50
                            text: robotState.emergencyStop ? "EMERGENCY STOP ACTIVE" : "EMERGENCY STOP"
                            enabled: robotState.robotConnected
                            
                            background: Rectangle {
                                color: robotState.emergencyStop ? Qt.darker(errorColor, 1.2) :
                                       (parent.pressed ? Qt.darker(errorColor, 1.2) :
                                        parent.hovered ? Qt.lighter(errorColor, 1.1) : errorColor)
                                radius: 4
                                opacity: parent.enabled ? 1.0 : 0.5
                                
                                // Pulsing animation when emergency stop is active
                                SequentialAnimation on opacity {
                                    running: robotState.emergencyStop
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.6; duration: 500 }
                                    NumberAnimation { to: 1.0; duration: 500 }
                                }
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 14
                                font.bold: true
                            }
                            
                            onClicked: {
                                if (robotState.emergencyStop) {
                                    // Clear emergency stop
                                    robotState.emergencyStop = false
                                    console.log("Emergency stop cleared")
                                } else {
                                    // Activate emergency stop
                                    robotState.emergencyStop = true
                                    robotState.robotEnabled = false
                                    console.log("Emergency stop activated")
                                }
                            }
                        }
                        
                        // Enable/Disable buttons
                        Row {
                            width: parent.width
                            spacing: 10
                            
                            Button {
                                width: (parent.width - parent.spacing) / 2
                                height: 40
                                text: "Enable"
                                enabled: robotState.robotConnected && !robotState.emergencyStop && !robotState.robotEnabled
                                
                                background: Rectangle {
                                    color: parent.pressed ? Qt.darker(primaryColor, 1.2) :
                                           parent.hovered ? Qt.lighter(primaryColor, 1.1) : primaryColor
                                    radius: 4
                                    opacity: parent.enabled ? 1.0 : 0.5
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                
                                onClicked: {
                                    robotState.robotEnabled = true
                                    console.log("Robot enabled")
                                }
                            }
                            
                            Button {
                                width: (parent.width - parent.spacing) / 2
                                height: 40
                                text: "Disable"
                                enabled: robotState.robotConnected && robotState.robotEnabled
                                
                                background: Rectangle {
                                    color: parent.pressed ? Qt.darker(warningColor, 1.2) :
                                           parent.hovered ? Qt.lighter(warningColor, 1.1) : warningColor
                                    radius: 4
                                    opacity: parent.enabled ? 1.0 : 0.5
                                }
                                
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                                
                                onClicked: {
                                    robotState.robotEnabled = false
                                    console.log("Robot disabled")
                                }
                            }
                        }
                    }
                    
                    // Status indicators
                    Column {
                        Layout.fillWidth: true
                        spacing: 10
                        
                        Text {
                            text: "Status"
                            color: textPrimaryColor
                            font.pixelSize: 16
                            font.bold: true
                        }
                        
                        // Status items
                        Repeater {
                            model: [
                                { label: "Communications", value: robotState.commsStatus },
                                { label: "Robot Code", value: robotState.robotCodeStatus },
                                { label: "Joysticks", value: robotState.joystickStatus },
                                { label: "Mode", value: robotState.robotMode },
                                { label: "Battery", value: robotState.batteryVoltage.toFixed(2) + "V" },
                                { label: "Ping", value: robotState.pingLatency >= 0 ? robotState.pingLatency + "ms" : "N/A" }
                            ]
                            
                            Row {
                                width: parent.width
                                spacing: 10
                                
                                Text {
                                    width: 100
                                    text: modelData.label + ":"
                                    color: textSecondaryColor
                                    font.pixelSize: 12
                                }
                                
                                Text {
                                    text: modelData.value
                                    color: textPrimaryColor
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true } // Spacer
                    
                    // Global shortcuts toggle
                    Row {
                        Layout.fillWidth: true
                        spacing: 10
                        
                        CheckBox {
                            id: globalShortcutsCheckbox
                            checked: robotState.globalShortcutsEnabled
                            onCheckedChanged: {
                                robotState.globalShortcutsEnabled = checked
                            }
                            
                            contentItem: Text {
                                text: "Global Shortcuts"
                                color: textPrimaryColor
                                leftPadding: parent.indicator.width + parent.spacing
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                        }
                    }
                    
                    // Shortcuts help text
                    Column {
                        Layout.fillWidth: true
                        spacing: 2
                        visible: robotState.globalShortcutsEnabled
                        
                        Text {
                            text: "Shortcuts:"
                            color: textSecondaryColor
                            font.pixelSize: 10
                        }
                        
                        Text {
                            text: "Space - Emergency Stop"
                            color: textSecondaryColor
                            font.pixelSize: 10
                        }
                        
                        Text {
                            text: "Enter - Disable Robot"
                            color: textSecondaryColor
                            font.pixelSize: 10
                        }
                        
                        Text {
                            text: "Ctrl+E - Enable Robot"
                            color: textSecondaryColor
                            font.pixelSize: 10
                        }
                    }
                }
            }
            
            // Right panel - Console and additional info
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: surfaceColor
                border.color: borderColor
                border.width: 1
                radius: 8
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15
                    
                    Text {
                        text: "Console Output"
                        color: textPrimaryColor
                        font.pixelSize: 16
                        font.bold: true
                    }
                    
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        TextArea {
                            id: consoleTextArea
                            text: robotState.consoleOutput
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextArea.Wrap
                            
                            background: Rectangle {
                                color: darkMode ? "#1a1a1a" : "#f8f8f8"
                                border.color: borderColor
                                border.width: 1
                                radius: 4
                            }
                            
                            color: textPrimaryColor
                            font.family: "Consolas, Monaco, monospace"
                            font.pixelSize: 11
                        }
                    }
                    
                    // Theme toggle
                    Row {
                        spacing: 10
                        
                        Button {
                            text: darkMode ? "Light Mode" : "Dark Mode"
                            
                            background: Rectangle {
                                color: parent.pressed ? Qt.darker(secondaryColor, 1.2) :
                                       parent.hovered ? Qt.lighter(secondaryColor, 1.1) : secondaryColor
                                radius: 4
                            }
                            
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 12
                            }
                            
                            onClicked: {
                                darkMode = !darkMode
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Status bar
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 25
        color: Qt.darker(surfaceColor, 1.1)
        border.color: borderColor
        border.width: 1
        
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 20
            
            Text {
                text: "Yet Another Driver Station v2025.1.0"
                color: textSecondaryColor
                font.pixelSize: 10
            }
            
            Text {
                text: robotState.robotConnected ? 
                      (robotState.connectionMode === 0 ? 
                       "Team " + robotState.teamNumber : 
                       robotState.robotIpAddress) : 
                      "Not Connected"
                color: textSecondaryColor
                font.pixelSize: 10
            }
        }
        
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            
            Text {
                text: new Date().toLocaleTimeString()
                color: textSecondaryColor
                font.pixelSize: 10
                
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: parent.text = new Date().toLocaleTimeString()
                }
            }
        }
    }
}
