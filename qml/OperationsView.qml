import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        
        // Robot commands
        GroupBox {
            title: "Robot Commands"
            Layout.fillWidth: true
            
            RowLayout {
                anchors.fill: parent
                
                Button {
                    text: "Reboot Robot"
                    onClicked: robotState.rebootRobot()
                }
                
                Button {
                    text: "Restart Code"
                    onClicked: robotState.restartRobotCode()
                }
                
                Item { Layout.fillWidth: true }
            }
        }
        
        // Practice match controls (if enabled)
        GroupBox {
            title: "Practice Match"
            Layout.fillWidth: true
            visible: enablePracticeMatch
            
            ColumnLayout {
                anchors.fill: parent
                
                RowLayout {
                    Button {
                        text: practiceMatchManager.running ? "Stop Match" : "Start Match"
                        onClicked: {
                            if (practiceMatchManager.running) {
                                practiceMatchManager.stopMatch()
                            } else {
                                practiceMatchManager.startMatch()
                            }
                        }
                    }
                    
                    Button {
                        text: "Reset"
                        onClicked: practiceMatchManager.resetMatch()
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Label {
                        text: `${Math.floor(practiceMatchManager.timeRemaining / 60)}:${(practiceMatchManager.timeRemaining % 60).toString().padStart(2, '0')}`
                        font.pixelSize: 24
                        font.bold: true
                    }
                }
                
                RowLayout {
                    Label { text: "Phase:" }
                    Label { 
                        text: {
                            switch(practiceMatchManager.currentPhase) {
                                case 0: return "Pre-Match"
                                case 1: return "Autonomous"
                                case 2: return "Teleop"
                                case 3: return "Endgame"
                                case 4: return "Post-Match"
                                default: return "Unknown"
                            }
                        }
                        font.bold: true
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                GridLayout {
                    columns: 6
                    
                    Label { text: "Auto Time:" }
                    SpinBox {
                        from: 0
                        to: 300
                        value: practiceMatchManager.autonomousTime
                        onValueChanged: practiceMatchManager.autonomousTime = value
                    }
                    
                    Label { text: "Teleop Time:" }
                    SpinBox {
                        from: 0
                        to: 300
                        value: practiceMatchManager.teleopTime
                        onValueChanged: practiceMatchManager.teleopTime = value
                    }
                    
                    Label { text: "Endgame Time:" }
                    SpinBox {
                        from: 0
                        to: 60
                        value: practiceMatchManager.endgameTime
                        onValueChanged: practiceMatchManager.endgameTime = value
                    }
                }
            }
        }
        
        // Log download
        GroupBox {
            title: "Robot Logs"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                
                RowLayout {
                    Button {
                        text: "Download Logs"
                        onClicked: {
                            // In a real implementation, this would open a file dialog
                            robotState.downloadLogs("/tmp/robot_logs")
                        }
                    }
                    
                    Button {
                        text: "Cancel Download"
                        enabled: robotState.logDownloadProgress > 0 && robotState.logDownloadProgress < 100
                        onClicked: robotState.cancelLogDownload()
                    }
                    
                    Item { Layout.fillWidth: true }
                }
                
                ProgressBar {
                    Layout.fillWidth: true
                    value: robotState.logDownloadProgress / 100.0
                    visible: robotState.logDownloadProgress > 0
                }
                
                Label {
                    text: robotState.logDownloadStatus
                    Layout.fillWidth: true
                    visible: robotState.logDownloadStatus !== ""
                }
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer
    }
}
