import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15

Frame {
    background: Rectangle { color: "transparent" }
    
    property bool robotConnected: robotState.commsStatus.toLowerCase().includes("connected")
    property bool isPaused: false
    property int maxDataPoints: 300
    property real timeRange: 60000 // 60 seconds
    
    // Data storage for network charts
    property var latencyData: []
    property var packetLossData: []
    property var bandwidthData: []
    
    // Connect to network diagnostic signals
    Connections {
        target: robotState
        
        function onNetworkLatencyChanged(latency, timestamp) {
            if (!isPaused && robotConnected) {
                addDataPoint(latencyData, latencySeries, timestamp, latency)
                updateChart(latencyChart, latencyData)
            }
        }
        
        function onPacketLossChanged(loss, timestamp) {
            if (!isPaused && robotConnected) {
                addDataPoint(packetLossData, packetLossSeries, timestamp, loss)
                updateChart(packetLossChart, packetLossData)
            }
        }
        
        function onBandwidthChanged(bandwidth, timestamp) {
            if (!isPaused && robotConnected) {
                addDataPoint(bandwidthData, bandwidthSeries, timestamp, bandwidth)
                updateChart(bandwidthChart, bandwidthData)
            }
        }
    }
    
    function addDataPoint(dataArray, series, timestamp, value) {
        dataArray.push({x: timestamp, y: value})
        
        const cutoffTime = timestamp - timeRange
        while (dataArray.length > 0 && dataArray[0].x < cutoffTime) {
            dataArray.shift()
        }
        
        if (dataArray.length > maxDataPoints) {
            dataArray.shift()
        }
        
        series.clear()
        for (let i = 0; i < dataArray.length; i++) {
            series.append(dataArray[i].x, dataArray[i].y)
        }
    }
    
    function updateChart(chart, dataArray) {
        if (dataArray.length === 0) return
        
        const now = Date.now()
        const minTime = now - timeRange
        const maxTime = now
        
        chart.axisX().min = minTime
        chart.axisX().max = maxTime
    }
    
    function clearAllData() {
        latencyData = []
        packetLossData = []
        bandwidthData = []
        
        latencySeries.clear()
        packetLossSeries.clear()
        bandwidthSeries.clear()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // Control Panel
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: "Network Diagnostics"
                font.pixelSize: 18
                font.bold: true
                color: "white"
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: isPaused ? "Resume" : "Pause"
                onClicked: isPaused = !isPaused
                background: Rectangle {
                    color: parent.pressed ? (isPaused ? "#4CAF50" : "#FF9800") : (isPaused ? "#66BB6A" : "#FFA726")
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
                text: "Clear"
                onClicked: clearAllData()
                background: Rectangle {
                    color: parent.pressed ? "#d32f2f" : "#f44336"
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
        
        // Current Network Status
        GroupBox {
            title: "Current Network Status"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 4
                columnSpacing: 20
                
                // Network Quality
                ColumnLayout {
                    Label { 
                        text: "Network Quality"
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label { 
                        text: robotState.networkQuality
                        font.pixelSize: 18
                        color: {
                            switch(robotState.networkQuality) {
                                case "Excellent": return "limegreen"
                                case "Good": return "#4caf50"
                                case "Fair": return "orange"
                                case "Poor": return "#ff5722"
                                case "Critical": return "red"
                                default: return "gray"
                            }
                        }
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                
                // Latency
                ColumnLayout {
                    Label { 
                        text: "Latency"
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label { 
                        text: robotState.networkLatency.toFixed(1) + " ms"
                        font.pixelSize: 18
                        color: robotState.networkLatency < 25 ? "limegreen" : 
                               robotState.networkLatency < 50 ? "orange" : "red"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                
                // Packet Loss
                ColumnLayout {
                    Label { 
                        text: "Packet Loss"
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label { 
                        text: robotState.packetLoss.toFixed(1) + "%"
                        font.pixelSize: 18
                        color: robotState.packetLoss < 3 ? "limegreen" : 
                               robotState.packetLoss < 10 ? "orange" : "red"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
                
                // Bandwidth
                ColumnLayout {
                    Label { 
                        text: "Bandwidth"
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                    }
                    Label { 
                        text: robotState.bandwidth.toFixed(1) + " KB/s"
                        font.pixelSize: 18
                        color: robotState.bandwidth > 10 ? "limegreen" : 
                               robotState.bandwidth > 5 ? "orange" : "red"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
        
        // Network Charts
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ColumnLayout {
                width: parent.width
                spacing: 10
                
                // Latency Chart
                ChartView {
                    id: latencyChart
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    title: "Network Latency (ms)"
                    antialiasing: true
                    backgroundColor: "#2d2d2d"
                    titleColor: "white"
                    legend.visible: false
                    
                    DateTimeAxis {
                        id: latencyTimeAxis
                        format: "hh:mm:ss"
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    ValueAxis {
                        id: latencyValueAxis
                        min: 0
                        max: 100
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    LineSeries {
                        id: latencySeries
                        axisX: latencyTimeAxis
                        axisY: latencyValueAxis
                        color: "#2196f3"
                        width: 2
                    }
                }
                
                // Packet Loss Chart
                ChartView {
                    id: packetLossChart
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    title: "Packet Loss (%)"
                    antialiasing: true
                    backgroundColor: "#2d2d2d"
                    titleColor: "white"
                    legend.visible: false
                    
                    DateTimeAxis {
                        id: packetLossTimeAxis
                        format: "hh:mm:ss"
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    ValueAxis {
                        id: packetLossValueAxis
                        min: 0
                        max: 20
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    LineSeries {
                        id: packetLossSeries
                        axisX: packetLossTimeAxis
                        axisY: packetLossValueAxis
                        color: "#ff5722"
                        width: 2
                    }
                }
                
                // Bandwidth Chart
                ChartView {
                    id: bandwidthChart
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    title: "Bandwidth (KB/s)"
                    antialiasing: true
                    backgroundColor: "#2d2d2d"
                    titleColor: "white"
                    legend.visible: false
                    
                    DateTimeAxis {
                        id: bandwidthTimeAxis
                        format: "hh:mm:ss"
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    ValueAxis {
                        id: bandwidthValueAxis
                        min: 0
                        max: 50
                        color: "white"
                        labelsColor: "white"
                    }
                    
                    LineSeries {
                        id: bandwidthSeries
                        axisX: bandwidthTimeAxis
                        axisY: bandwidthValueAxis
                        color: "#4caf50"
                        width: 2
                    }
                }
            }
        }
    }

    // No Robot Connected Overlay
    Rectangle {
        anchors.fill: parent
        color: "#2d2d2d"
        opacity: 0.95
        visible: !robotConnected
        
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20
            
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64
                height: 64
                radius: 32
                color: "#404040"
                border.color: "red"
                border.width: 3
                
                Label {
                    anchors.centerIn: parent
                    text: "!"
                    font.pixelSize: 32
                    font.bold: true
                    color: "red"
                }
            }
            
            Label {
                text: "No Robot Connection"
                font.pixelSize: 24
                font.bold: true
                color: "red"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: "Connect to your team robot to view network diagnostics"
                font.pixelSize: 14
                color: "lightgray"
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
