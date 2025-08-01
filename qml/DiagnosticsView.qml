import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    ScrollView {
        anchors.fill: parent
        
        ColumnLayout {
            width: parent.width
            spacing: 10
            
            // Robot diagnostics
            GroupBox {
                title: "Robot Diagnostics"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 4
                    
                    Label { text: "CPU Usage:" }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: robotState.cpuUsage / 100.0
                    }
                    Label { text: `${robotState.cpuUsage}%` }
                    Rectangle { width: 1; height: 1; color: "transparent" }
                    
                    Label { text: "RAM Usage:" }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: robotState.ramUsage / 100.0
                    }
                    Label { text: `${robotState.ramUsage}%` }
                    Rectangle { width: 1; height: 1; color: "transparent" }
                    
                    Label { text: "Disk Usage:" }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: robotState.diskUsage / 100.0
                    }
                    Label { text: `${robotState.diskUsage}%` }
                    Rectangle { width: 1; height: 1; color: "transparent" }
                    
                    Label { text: "CAN Utilization:" }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: robotState.canUtil / 100.0
                    }
                    Label { text: `${robotState.canUtil.toFixed(1)}%` }
                    Rectangle { width: 1; height: 1; color: "transparent" }
                }
            }
            
            // Network diagnostics
            GroupBox {
                title: "Network Diagnostics"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Label { text: "Latency:" }
                    Label { text: `${robotState.networkLatency.toFixed(1)} ms` }
                    
                    Label { text: "Packet Loss:" }
                    Label { 
                        text: `${robotState.packetLoss.toFixed(1)}%`
                        color: robotState.packetLoss > 10 ? "red" : "green"
                    }
                    
                    Label { text: "Bandwidth:" }
                    Label { text: `${robotState.bandwidth.toFixed(1)} KB/s` }
                    
                    Label { text: "NetworkTables:" }
                    Label { 
                        text: robotState.networkTablesStatus
                        color: robotState.networkTablesConnected ? "green" : "red"
                    }
                    
                    Label { text: "Internet:" }
                    Label { 
                        text: networkManager.internetConnected ? "Connected" : "Disconnected"
                        color: networkManager.internetConnected ? "green" : "red"
                    }
                    
                    Label { text: "Primary Interface:" }
                    Label { text: networkManager.primaryInterface }
                    
                    Label { text: "Gateway:" }
                    Label { text: networkManager.gatewayAddress }
                }
            }
            
            // Battery diagnostics
            GroupBox {
                title: "Battery Diagnostics"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Label { text: "Current Voltage:" }
                    Label { 
                        text: `${batteryManager.currentVoltage.toFixed(2)}V`
                        color: {
                            if (batteryManager.currentVoltage < 6.0) return "red"
                            if (batteryManager.currentVoltage < 7.0) return "orange"
                            return "green"
                        }
                    }
                    
                    Label { text: "Battery Level:" }
                    Label { 
                        text: {
                            switch(batteryManager.batteryLevel) {
                                case 0: return "Critical"
                                case 1: return "Warning"
                                case 2: return "Normal"
                                default: return "Unknown"
                            }
                        }
                        color: {
                            switch(batteryManager.batteryLevel) {
                                case 0: return "red"
                                case 1: return "orange"
                                case 2: return "green"
                                default: return "gray"
                            }
                        }
                    }
                    
                    Label { text: "Critical Threshold:" }
                    SpinBox {
                        from: 0
                        to: 15
                        stepSize: 0.1
                        value: batteryManager.criticalThreshold * 10
                        onValueChanged: batteryManager.criticalThreshold = value / 10.0
                        textFromValue: function(value) { return (value / 10.0).toFixed(1) + "V" }
                    }
                    
                    Label { text: "Warning Threshold:" }
                    SpinBox {
                        from: 0
                        to: 15
                        stepSize: 0.1
                        value: batteryManager.warningThreshold * 10
                        onValueChanged: batteryManager.warningThreshold = value / 10.0
                        textFromValue: function(value) { return (value / 10.0).toFixed(1) + "V" }
                    }
                    
                    Label { text: "Auto-Disable:" }
                    CheckBox {
                        checked: batteryManager.autoDisableEnabled
                        onCheckedChanged: batteryManager.autoDisableEnabled = checked
                    }
                }
            }
            
            Item { Layout.fillHeight: true } // Spacer
        }
    }
}
