import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import FRC.Backend 1.0

Dialog {
    id: logDownloadDialog
    title: "Download Robot Logs"
    modal: true
    width: 400
    height: 300
    
    property string selectedPath: ""
    
    background: Rectangle {
        color: "#2d2d2d"
        radius: 8
        border.color: "#555555"
        border.width: 1
    }
    
    FileDialog {
        id: folderDialog
        title: "Select Download Location"
        selectFolder: true
        folder: shortcuts.home
        onAccepted: {
            selectedPath = folderDialog.fileUrl.toString().replace("file://", "")
            pathField.text = selectedPath
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // Header
        Label {
            text: "Download WPILib Log Files"
            font.pixelSize: 18
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }
        
        Label {
            text: "Download robot log files (.wpilog) from the roboRIO to analyze robot performance and debug issues."
            color: "lightgray"
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#555555"
        }
        
        // Connection Status
        RowLayout {
            Layout.fillWidth: true
            
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: robotState.commsStatus.toLowerCase().includes("connected") ? "limegreen" : "red"
            }
            
            Label {
                text: robotState.commsStatus.toLowerCase().includes("connected") ? 
                      "Robot Connected - Ready to download logs" : 
                      "No Robot Connection - Connect to robot first"
                color: robotState.commsStatus.toLowerCase().includes("connected") ? "limegreen" : "red"
                font.pixelSize: 12
            }
            
            Item { Layout.fillWidth: true }
        }
        
        // Destination Path Selection
        GroupBox {
            title: "Download Location"
            Layout.fillWidth: true
            
            background: Rectangle {
                color: "#404040"
                radius: 4
                border.color: "#606060"
                border.width: 1
            }
            
            label: Label {
                text: parent.title
                color: "white"
                font.bold: true
            }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    TextField {
                        id: pathField
                        text: selectedPath || "No location selected"
                        color: selectedPath ? "white" : "gray"
                        font.pixelSize: 12
                        Layout.fillWidth: true
                        placeholderText: "Select folder..."
                    }
                    
                    Button {
                        text: "Browse..."
                        onClicked: folderDialog.open()
                        
                        background: Rectangle {
                            color: parent.pressed ? "#0078d4" : "#106ebe"
                            radius: 4
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
                
                Label {
                    text: "💡 Tip: Create a dedicated folder for robot logs to keep them organized"
                    color: "lightblue"
                    font.pixelSize: 10
                    font.italic: true
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }
            }
        }
        
        // Available Log Files
        GroupBox {
            title: "Available Log Files"
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            
            background: Rectangle {
                color: "#404040"
                radius: 4
                border.color: "#606060"
                border.width: 1
            }
            
            label: Label {
                text: parent.title
                color: "white"
                font.bold: true
            }
            
            ListView {
                anchors.fill: parent
                anchors.margins: 10
                
                model: robotState.availableLogFiles
                
                delegate: CheckBox {
                    text: modelData
                    checked: true
                }
            }
        }
        
        // Download Progress
        GroupBox {
            title: "Download Progress"
            Layout.fillWidth: true
            visible: robotState.logDownloadInProgress
            
            background: Rectangle {
                color: "#404040"
                radius: 4
                border.color: "#606060"
                border.width: 1
            }
            
            label: Label {
                text: parent.title
                color: "white"
                font.bold: true
            }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                Label {
                    text: robotState.logDownloadStatus
                    color: "white"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                
                ProgressBar {
                    Layout.fillWidth: true
                    value: robotState.logDownloadProgress / 100.0
                    
                    background: Rectangle {
                        color: "#606060"
                        radius: 4
                    }
                    
                    contentItem: Item {
                        Rectangle {
                            width: parent.width * parent.parent.value
                            height: parent.height
                            color: "#4CAF50"
                            radius: 4
                        }
                    }
                }
                
                Label {
                    text: robotState.logDownloadProgress + "%"
                    color: "white"
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
        
        Item { Layout.fillHeight: true }
        
        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                text: "Refresh File List"
                enabled: robotState.commsStatus.toLowerCase().includes("connected") && !robotState.logDownloadInProgress
                onClicked: {
                    // Request fresh file list
                    robotState.getAvailableLogFiles()
                }
                
                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? "#FF8F00" : "#FFA000") : "#666666"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : "gray"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: robotState.logDownloadInProgress ? "Cancel" : "Close"
                onClicked: {
                    if (robotState.logDownloadInProgress) {
                        robotState.cancelLogDownload()
                    } else {
                        logDownloadDialog.close()
                    }
                }
                
                background: Rectangle {
                    color: parent.pressed ? "#666666" : "#808080"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            Button {
                text: "Download Logs"
                enabled: robotState.commsStatus.toLowerCase().includes("connected") && 
                        pathField.text !== "" && 
                        !robotState.logDownloadInProgress &&
                        robotState.availableLogFiles.length > 0
                onClicked: {
                    robotState.downloadLogs(pathField.text)
                    logDownloadDialog.close()
                }
                
                background: Rectangle {
                    color: parent.enabled ? (parent.pressed ? "#4CAF50" : "#66BB6A") : "#666666"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "white" : "gray"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }
            }
        }
    }
    
    // Success/Error notifications
    Connections {
        target: robotState
        
        function onLogDownloadCompleted(destinationPath, success) {
            if (success) {
                successNotification.show("Logs downloaded successfully to:\n" + destinationPath)
            } else {
                errorNotification.show("Log download failed. Check robot connection and try again.")
            }
        }
    }
    
    // Success notification
    Rectangle {
        id: successNotification
        anchors.centerIn: parent
        width: 300
        height: 100
        color: "#4CAF50"
        radius: 8
        visible: false
        z: 1000
        
        function show(message) {
            successText.text = message
            visible = true
            hideTimer.start()
        }
        
        Label {
            id: successText
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        
        Timer {
            id: hideTimer
            interval: 3000
            onTriggered: successNotification.visible = false
        }
    }
    
    // Error notification
    Rectangle {
        id: errorNotification
        anchors.centerIn: parent
        width: 300
        height: 100
        color: "#F44336"
        radius: 8
        visible: false
        z: 1000
        
        function show(message) {
            errorText.text = message
            visible = true
            errorHideTimer.start()
        }
        
        Label {
            id: errorText
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }
        
        Timer {
            id: errorHideTimer
            interval: 4000
            onTriggered: errorNotification.visible = false
        }
    }
}
