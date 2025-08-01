import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        
        // Chart selection
        RowLayout {
            Layout.fillWidth: true
            
            Label { text: "Chart Type:" }
            
            ComboBox {
                id: chartTypeCombo
                model: ["Battery Voltage", "CPU Usage", "Network Latency", "CAN Utilization"]
                currentIndex: 0
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: "Clear"
                onClicked: {
                    // Clear chart data
                }
            }
        }
        
        // Chart area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            border.color: "#cccccc"
            border.width: 1
            color: "#ffffff"
            
            // Placeholder for actual chart implementation
            Label {
                anchors.centerIn: parent
                text: "Chart: " + chartTypeCombo.currentText
                font.pixelSize: 24
                color: "#cccccc"
            }
            
            // Simple voltage chart simulation
            Canvas {
                id: chartCanvas
                anchors.fill: parent
                anchors.margins: 20
                
                property var voltageHistory: []
                
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        if (chartTypeCombo.currentIndex === 0) { // Battery Voltage
                            chartCanvas.voltageHistory.push(robotState.robotVoltage)
                            if (chartCanvas.voltageHistory.length > 60) {
                                chartCanvas.voltageHistory.shift()
                            }
                            chartCanvas.requestPaint()
                        }
                    }
                }
                
                onPaint: {
                    if (chartTypeCombo.currentIndex !== 0 || voltageHistory.length < 2) return
                    
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    
                    // Draw grid
                    ctx.strokeStyle = "#eeeeee"
                    ctx.lineWidth = 1
                    
                    for (var i = 0; i <= 10; i++) {
                        var y = (height / 10) * i
                        ctx.beginPath()
                        ctx.moveTo(0, y)
                        ctx.lineTo(width, y)
                        ctx.stroke()
                    }
                    
                    for (var j = 0; j <= 12; j++) {
                        var x = (width / 12) * j
                        ctx.beginPath()
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, height)
                        ctx.stroke()
                    }
                    
                    // Draw voltage line
                    ctx.strokeStyle = "#4CAF50"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    
                    for (var k = 0; k < voltageHistory.length; k++) {
                        var voltage = voltageHistory[k]
                        var x = (width / (voltageHistory.length - 1)) * k
                        var y = height - ((voltage / 15.0) * height) // Scale 0-15V to canvas height
                        
                        if (k === 0) {
                            ctx.moveTo(x, y)
                        } else {
                            ctx.lineTo(x, y)
                        }
                    }
                    
                    ctx.stroke()
                }
            }
        }
        
        // Chart legend/info
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            border.color: "#cccccc"
            border.width: 1
            color: "#f8f8f8"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Column {
                    Label {
                        text: "Current Value:"
                        font.bold: true
                    }
                    Label {
                        text: {
                            switch(chartTypeCombo.currentIndex) {
                                case 0: return `${robotState.robotVoltage.toFixed(2)}V`
                                case 1: return `${robotState.cpuUsage}%`
                                case 2: return `${robotState.networkLatency.toFixed(1)}ms`
                                case 3: return `${robotState.canUtil.toFixed(1)}%`
                                default: return "N/A"
                            }
                        }
                        font.pixelSize: 18
                        color: {
                            switch(chartTypeCombo.currentIndex) {
                                case 0: return robotState.robotVoltage < 7.0 ? "red" : "green"
                                case 1: return robotState.cpuUsage > 80 ? "red" : "green"
                                case 2: return robotState.networkLatency > 50 ? "red" : "green"
                                case 3: return robotState.canUtil > 80 ? "red" : "green"
                                default: return "black"
                            }
                        }
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Column {
                    Label {
                        text: "Time Range:"
                        font.bold: true
                    }
                    Label {
                        text: "Last 60 seconds"
                    }
                }
            }
        }
    }
}
