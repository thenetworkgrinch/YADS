import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import FRC.Backend 1.0

Frame {
    background: Rectangle { color: "transparent" }

    property var logEntries: []
    property string filterText: ""
    property string selectedCategory: "All"
    property bool robotConnected: robotState.commsStatus.toLowerCase().includes("connected")

    // Connect to logging signals
    Connections {
        target: robotState
        
        function onEventLogged(event, details, timestamp) {
            var entry = {
                timestamp: timestamp,
                time: new Date(timestamp).toLocaleTimeString(),
                event: event,
                details: details,
                category: getCategoryFromEvent(event)
            }
            
            logEntries.unshift(entry) // Add to beginning
            
            // Limit entries to prevent memory issues
            if (logEntries.length > 1000) {
                logEntries = logEntries.slice(0, 1000)
            }
            
            logListModel.clear()
            updateFilteredEntries()
        }
    }

    // Functions from EventLogView
    function getCategoryFromEvent(event) {
        if (event.includes("Emergency")) return "Emergency"
        if (event.includes("Robot") || event.includes("Control")) return "Robot"
        if (event.includes("Battery")) return "Battery"
        if (event.includes("Network") || event.includes("Communication")) return "Network"
        if (event.includes("Practice")) return "Practice"
        if (event.includes("Dashboard")) return "Dashboard"
        if (event.includes("Configuration")) return "Config"
        if (event.includes("FMS")) return "FMS"
        return "System"
    }

    function updateFilteredEntries() {
        logListModel.clear()

        for (var i = 0; i < logEntries.length; i++) {
            var entry = logEntries[i]

            // Apply category filter
            if (selectedCategory !== "All" && entry.category !== selectedCategory) {
                continue
            }

            // Apply text filter
            if (filterText !== "" &&
                !entry.event.toLowerCase().includes(filterText.toLowerCase()) &&
                !entry.details.toLowerCase().includes(filterText.toLowerCase())) {
                continue
            }

            logListModel.append(entry)
        }
    }

    function exportLog() {
        fileDialog.open()
    }

    function clearLog() {
        logEntries = []
        logListModel.clear()
        robotState.clearEventLog()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // View switcher
        TabBar {
            id: logTabs
            currentIndex: 0
            Layout.fillWidth: true

            TabButton { text: "Event Log" }
            TabButton { text: "Console Output" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: logTabs.currentIndex

            // Event Log View
            ColumnLayout {
                // Header and Controls
                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "Event Log"
                        font.pixelSize: 18
                        font.bold: true
                        color: "white"
                    }

                    Item { Layout.fillWidth: true }

                    // Category Filter
                    ComboBox {
                        id: categoryComboBox
                        model: ["All", "Emergency", "Robot", "Battery", "Network", "Practice", "Dashboard", "Config", "FMS", "System"]
                        currentText: selectedCategory
                        onCurrentTextChanged: {
                            selectedCategory = currentText
                            updateFilteredEntries()
                        }

                        background: Rectangle {
                            color: "#404040"
                            border.color: "#666666"
                            radius: 4
                        }

                        contentItem: Text {
                            text: categoryComboBox.displayText
                            font: categoryComboBox.font
                            color: "white"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 10
                        }
                    }

                    // Text Filter
                    TextField {
                        id: filterTextField
                        placeholderText: "Filter events..."
                        text: filterText
                        onTextChanged: {
                            filterText = text
                            updateFilteredEntries()
                        }

                        background: Rectangle {
                            color: "#404040"
                            border.color: "#666666"
                            radius: 4
                        }

                        color: "white"
                        selectionColor: "#2196f3"
                        selectedTextColor: "white"
                    }

                    Button {
                        text: "Export"
                        onClicked: exportLog()

                        background: Rectangle {
                            color: parent.pressed ? "#1976d2" : "#2196f3"
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
                        onClicked: clearLog()

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

                // Log Statistics
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: "#1e1e1e"
                    border.color: "#404040"
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 30

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Total Events"
                                color: "lightgray"
                                font.pixelSize: 11
                            }
                            Label {
                                text: logEntries.length.toString()
                                color: "white"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Filtered"
                                color: "lightgray"
                                font.pixelSize: 11
                            }
                            Label {
                                text: logListModel.count.toString()
                                color: "white"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: "Category"
                                color: "lightgray"
                                font.pixelSize: 11
                            }
                            Label {
                                text: selectedCategory
                                color: "#2196f3"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: "🔄 Live"
                            color: "limegreen"
                            font.pixelSize: 12
                            font.bold: true

                            SequentialAnimation on opacity {
                                running: true
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.5; duration: 1000 }
                                NumberAnimation { to: 1.0; duration: 1000 }
                            }
                        }
                    }
                }

                // Event Log List
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1e1e1e"
                    border.color: "#404040"
                    radius: 4

                    ListView {
                        id: logListView
                        anchors.fill: parent
                        anchors.margins: 5
                        model: ListModel { id: logListModel }
                        clip: true

                        ScrollBar.vertical: ScrollBar {
                            active: true
                            policy: ScrollBar.AlwaysOn

                            background: Rectangle {
                                color: "#2d2d2d"
                                radius: 4
                            }

                            contentItem: Rectangle {
                                color: "#666666"
                                radius: 4
                            }
                        }

                        delegate: Rectangle {
                            width: logListView.width - 10
                            height: 60
                            color: index % 2 === 0 ? "#2d2d2d" : "#252525"
                            border.color: {
                                switch(model.category) {
                                    case "Emergency": return "#f44336"
                                    case "Robot": return "#2196f3"
                                    case "Battery": return "#ff9800"
                                    case "Network": return "#9c27b0"
                                    case "Practice": return "#4caf50"
                                    case "Dashboard": return "#00bcd4"
                                    case "Config": return "#795548"
                                    case "FMS": return "#ff5722"
                                    default: return "#666666"
                                }
                            }
                            border.width: 1
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 15

                                // Category Indicator
                                Rectangle {
                                    width: 8
                                    height: 40
                                    radius: 4
                                    color: parent.parent.border.color
                                }

                                // Time
                                ColumnLayout {
                                    spacing: 2
                                    Layout.preferredWidth: 80

                                    Label {
                                        text: model.time
                                        color: "white"
                                        font.pixelSize: 12
                                        font.bold: true
                                    }

                                    Label {
                                        text: model.category
                                        color: parent.parent.parent.border.color
                                        font.pixelSize: 10
                                    }
                                }

                                // Event Details
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        text: model.event
                                        color: "white"
                                        font.pixelSize: 13
                                        font.bold: true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: model.details
                                        color: "lightgray"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    // Could show detailed view or copy to clipboard
                                    console.log("Event clicked:", model.event, model.details)
                                }
                            }
                        }

                        // Empty state
                        Label {
                            anchors.centerIn: parent
                            text: logEntries.length === 0 ? "No events logged yet" : "No events match current filter"
                            color: "gray"
                            font.pixelSize: 14
                            visible: logListModel.count === 0
                        }
                    }
                }
            }

            // Console Output View
            Item {
                // Placeholder for Console Output View
                Label {
                    anchors.centerIn: parent
                    text: "Console Output View\n(Implementation placeholder)"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 18
                    color: "#888888"
                }
            }
        }
    }

    // File Dialog for Export
    FileDialog {
        id: fileDialog
        title: "Export Event Log"
        selectExisting: false
        nameFilters: ["Text files (*.txt)", "All files (*)"]
        defaultSuffix: "txt"

        onAccepted: {
            var filePath = fileDialog.fileUrl.toString().replace("file://", "")
            robotState.exportEventLog(filePath)
        }
    }

    // Initialize with recent log entries
    Component.onCompleted: {
        var recentEntries = robotState.getRecentLogEntries(100)
        for (var i = 0; i < recentEntries.length; i++) {
            var parts = recentEntries[i].match(/\[([^\]]+)\] ([^:]+): (.+)/)
            if (parts && parts.length >= 4) {
                var entry = {
                    timestamp: Date.now() - (recentEntries.length - i) * 1000,
                    time: parts[1],
                    event: parts[2],
                    details: parts[3],
                    category: getCategoryFromEvent(parts[2])
                }
                logEntries.push(entry)
            }
        }
        updateFilteredEntries()
    }
}
