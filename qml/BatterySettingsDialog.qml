import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import FRC.Backend 1.0

Dialog {
    id: batterySettingsDialog
    title: "Battery Settings"
    modal: true
    width: 400
    height: 300
    
    background: Rectangle {
        color: "#2d2d2d"
        radius: 8
        border.color: "#555555"
        border.width: 1
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // Header
        Label {
            text: "Battery Monitoring Configuration"
            font.pixelSize: 18
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }
        
        // Current Status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#404040"
            radius: 8
            border.color: "#606060"
            border.width: 1
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                
                Label {
                    text: "🔋"
                    font.pixelSize: 24
                }
                
                ColumnLayout {
                    spacing: 2
                    
                    Label {
                        text: "Current Battery: " + robotState.robotVoltage.toFixed(2) + "V"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    Label {
                        text: "Status: " + robotState.batteryStatus
                        color: {
                            if (robotState.batteryCriticalActive) return "#f44336"
                            if (robotState.batteryWarningActive) return "#ff9800"
                            return "limegreen"
                        }
                        font.pixelSize: 12
                    }
                }
                
                Item { Layout.fillWidth: true }
            }
        }
        
        // Voltage Thresholds
        GroupBox {
            title: "Voltage Thresholds"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                
                Label { text: "Critical Threshold:" }
                SpinBox {
                    id: criticalThresholdSpinBox
                    from: 0
                    to: 150
                    stepSize: 1
                    value: batteryManager.criticalThreshold * 10
                    onValueChanged: batteryManager.criticalThreshold = value / 10.0
                    textFromValue: function(value) { return (value / 10.0).toFixed(1) + "V" }
                    valueFromText: function(text) { return parseFloat(text) * 10 }
                    
                    background: Rectangle {
                        color: "#606060"
                        radius: 4
                    }
                    
                    contentItem: TextInput {
                        text: parent.textFromValue(parent.value, parent.locale)
                        font: parent.font
                        color: "white"
                        selectionColor: "#0078d4"
                        selectedTextColor: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        readOnly: !parent.editable
                        validator: parent.validator
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                }
                
                Label { text: "Warning Threshold:" }
                SpinBox {
                    id: warningThresholdSpinBox
                    from: 0
                    to: 150
                    stepSize: 1
                    value: batteryManager.warningThreshold * 10
                    onValueChanged: batteryManager.warningThreshold = value / 10.0
                    textFromValue: function(value) { return (value / 10.0).toFixed(1) + "V" }
                    valueFromText: function(text) { return parseFloat(text) * 10 }
                    
                    background: Rectangle {
                        color: "#606060"
                        radius: 4
                    }
                    
                    contentItem: TextInput {
                        text: parent.textFromValue(parent.value, parent.locale)
                        font: parent.font
                        color: "white"
                        selectionColor: "#0078d4"
                        selectedTextColor: "white"
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        readOnly: !parent.editable
                        validator: parent.validator
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                }
            }
        }
        
        // Protection Settings
        GroupBox {
            title: "Protection Settings"
            Layout.fillWidth: true
            
            CheckBox {
                id: autoDisableCheckBox
                text: "Automatically disable robot on critical voltage"
                checked: batteryManager.autoDisableEnabled
                onCheckedChanged: batteryManager.autoDisableEnabled = checked
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    leftPadding: parent.indicator.width + parent.spacing
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        
        Item { Layout.fillHeight: true }
        
        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            
            Button {
                text: "Reset to Defaults"
                onClicked: {
                    batteryManager.criticalThreshold = 6.0
                    batteryManager.warningThreshold = 7.0
                    batteryManager.autoDisableEnabled = true
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
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "Close"
                onClicked: batterySettingsDialog.close()
                
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
        }
    }
}
