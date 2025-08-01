import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    
    property bool connected: false
    property string status: "Disconnected"
    property int entryCount: 0
    property bool glassAvailable: true
    property bool glassRunning: false
    
    signal launchGlassRequested()
    signal closeGlassRequested()
    
    width: 200
    height: 60
    color: connected ? "#2d5a2d" : "#5a2d2d"
    border.color: connected ? "#4a8a4a" : "#8a4a4a"
    border.width: 1
    radius: 4
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8
        
        // Status indicator
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: connected ? "#4CAF50" : "#F44336"
            
            SequentialAnimation on opacity {
                running: !connected
                loops: Animation.Infinite
                NumberAnimation { to: 0.3; duration: 1000 }
                NumberAnimation { to: 1.0; duration: 1000 }
            }
        }
        
        // Status text
        Column {
            Layout.fillWidth: true
            spacing: 2
            
            Text {
                text: "NT:"
                font.pixelSize: 12
                font.bold: true
                color: "white"
            }
            
            Text {
                text: root.status
                font.pixelSize: 10
                color: connected ? "#90EE90" : "#FFB6C1"
            }
            
            Text {
                text: connected ? qsTr("%1 entries").arg(entryCount) : ""
                font.pixelSize: 9
                color: "#CCCCCC"
                visible: connected
            }
        }
        
        // Glass launch button
        Button {
            id: glassButton
            text: glassRunning ? "Close" : "Glass"
            enabled: glassAvailable
            
            background: Rectangle {
                color: glassButton.pressed ? "#555555" : 
                       glassButton.hovered ? "#444444" : "#333333"
                border.color: glassRunning ? "#4CAF50" : "#666666"
                border.width: 1
                radius: 3
            }
            
            contentItem: Text {
                text: glassButton.text
                font.pixelSize: 10
                color: glassButton.enabled ? "white" : "#888888"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            
            onClicked: {
                if (glassRunning) {
                    root.closeGlassRequested()
                } else {
                    root.launchGlassRequested()
                }
            }
            
            ToolTip.visible: hovered
            ToolTip.text: glassRunning ? 
                         qsTr("Close Glass NetworkTables viewer") : 
                         qsTr("Launch Glass NetworkTables viewer")
        }
    }
    
    // Connection status tooltip
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        
        ToolTip.visible: containsMouse
        ToolTip.text: connected ? 
                     qsTr("NetworkTables connected\n%1 entries available\nClick Glass to view data").arg(entryCount) :
                     qsTr("NetworkTables disconnected\nWaiting for robot connection")
    }
}
