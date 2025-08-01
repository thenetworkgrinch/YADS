import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3

Frame {
    background: Rectangle { color: "transparent" }
    
    property bool showingInternalDashboard: robotState.selectedDashboard === "Driver Station"
    property bool dashboardRunning: robotState.isDashboardRunning(robotState.selectedDashboard)
    
    Connections {
        target: robotState
        
        function onSelectedDashboardChanged(dashboard) {
            dashboardRunning = robotState.isDashboardRunning(dashboard)
        }
        
        function onDashboardLaunched(name, success) {
            if (name === robotState.selectedDashboard) {
                dashboardRunning = success
            }
        }
        
        function onDashboardClosed(name) {
            if (name === robotState.selectedDashboard) {
                dashboardRunning = false
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // Dashboard Selection Header
        RowLayout {
            Layout.fillWidth: true
            
            Label {
                text: "Robot Dashboard"
                font.pixelSize: 18
                font.bold: true
                color: "white"
            }
            
            Item { Layout.fillWidth: true }
            
            Label {
                text: "Dashboard:"
                color: "white"
            }
            
            ComboBox {
                id: dashboardCombo
                model: robotState.getAvailableDashboards()
                currentIndex: {
                    const dashboards = robotState.getAvailableDashboards()
                    return Math.max(0, dashboards.indexOf(robotState.selectedDashboard))
                }
                onCurrentTextChanged: {
                    if (currentText && currentText !== robotState.selectedDashboard) {
                        robotState.selectedDashboard = currentText
                    }
                }
                
                Connections {
                    target: robotState
                    function onDashboardListChanged() {
                        dashboardCombo.model = robotState.getAvailableDashboards()
                    }
                }
            }
            
            Button {
                text: "Manage"
                onClicked: dashboardManager.open()
            }
            
            Button {
                text: showingInternalDashboard ? "Refresh" : (dashboardRunning ? "Stop" : "Launch")
                enabled: showingInternalDashboard || robotState.getDashboardPath(robotState.selectedDashboard) !== ""
                onClicked: {
                    if (showingInternalDashboard) {
                        // For internal dashboard, just refresh the data
                        console.log("Refreshing internal dashboard")
                    } else if (dashboardRunning) {
                        robotState.closeDashboard(robotState.selectedDashboard)
                    } else {
                        robotState.launchDashboard(robotState.selectedDashboard)
                    }
                }
                
                background: Rectangle {
                    color: {
                        if (showingInternalDashboard) return parent.pressed ? "#4CAF50" : "#66BB6A"
                        if (dashboardRunning) return parent.pressed ? "#d32f2f" : "#f44336"
                        return parent.pressed ? "#4CAF50" : "#66BB6A"
                    }
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
        
        // Status Display
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#404040"
            radius: 4
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: {
                        if (showingInternalDashboard) return "limegreen"
                        if (dashboardRunning) return "limegreen"
                        return robotState.getDashboardPath(robotState.selectedDashboard) !== "" ? "orange" : "red"
                    }
                }
                
                Label {
                    text: {
                        if (showingInternalDashboard) return "Driver Station Dashboard"
                        if (dashboardRunning) return `${robotState.selectedDashboard} is running`
                        if (robotState.getDashboardPath(robotState.selectedDashboard) === "") return `${robotState.selectedDashboard} - No path configured`
                        return `${robotState.selectedDashboard} - Ready to launch`
                    }
                    color: "white"
                    Layout.fillWidth: true
                }
                
                Label {
                    text: robotState.getCurrentPlatform()
                    color: "lightgray"
                }
            }
        }
        
        // Dashboard Content
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"
            radius: 8
            border.color: "#555"
            border.width: 1
            
            // Internal Driver Station Dashboard
            DiagnosticsView {
                anchors.fill: parent
                anchors.margins: 1
                visible: showingInternalDashboard
            }
            
            // External Dashboard Status
            ColumnLayout {
                anchors.centerIn: parent
                visible: !showingInternalDashboard
                spacing: 20
                
                Label {
                    text: {
                        if (dashboardRunning) return `${robotState.selectedDashboard} Running`
                        if (robotState.getDashboardPath(robotState.selectedDashboard) === "") return "Dashboard Not Configured"
                        return `${robotState.selectedDashboard} Ready`
                    }
                    font.pixelSize: 24
                    color: dashboardRunning ? "limegreen" : 
                           (robotState.getDashboardPath(robotState.selectedDashboard) !== "" ? "orange" : "gray")
                    Layout.alignment: Qt.AlignHCenter
                }
                
                Label {
                    text: {
                        if (dashboardRunning) return "Dashboard is running as external process"
                        if (robotState.getDashboardPath(robotState.selectedDashboard) === "") return "Configure the path to launch this dashboard"
                        return "Click Launch to start the dashboard"
                    }
                    font.pixelSize: 14
                    color: "lightgray"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }
                
                Label {
                    text: `Path: ${robotState.getDashboardPath(robotState.selectedDashboard) || "Not set"}`
                    font.pixelSize: 12
                    color: "gray"
                    Layout.alignment: Qt.AlignHCenter
                    visible: !dashboardRunning
                }
                
                Button {
                    text: robotState.getDashboardPath(robotState.selectedDashboard) === "" ? "Configure Path" : "Launch Dashboard"
                    Layout.alignment: Qt.AlignHCenter
                    visible: !dashboardRunning
                    onClicked: {
                        if (robotState.getDashboardPath(robotState.selectedDashboard) === "") {
                            dashboardManager.open()
                        } else {
                            robotState.launchDashboard(robotState.selectedDashboard)
                        }
                    }
                }
            }
        }
    }
    
    // Dashboard Management Dialog
    Dialog {
        id: dashboardManager
        title: "Manage Dashboards"
        width: 700
        height: 500
        modal: true
        anchors.centerIn: parent
        
        background: Rectangle {
            color: "#2d2d2d"
            radius: 8
            border.color: "#555"
            border.width: 1
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            RowLayout {
                Layout.fillWidth: true
                
                Label {
                    text: `Dashboard Paths (${robotState.getCurrentPlatform()})`
                    font.pixelSize: 16
                    font.bold: true
                    color: "white"
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "Reload Configs"
                    onClicked: robotState.reloadDashboardConfigs()
                }
            }
            
            Label {
                text: "Dashboards are loaded from config files. User changes override the defaults."
                color: "lightgray"
                font.pixelSize: 10
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                ListView {
                    model: robotState.getAvailableDashboards()
                    spacing: 8
                    
                    delegate: Frame {
                        width: parent.width
                        background: Rectangle {
                            color: "#404040"
                            radius: 4
                        }
                        
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8
                            
                            RowLayout {
                                Layout.fillWidth: true
                                
                                Label {
                                    text: modelData
                                    font.bold: true
                                    color: "white"
                                    font.pixelSize: 14
                                }
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: {
                                        if (modelData === "Driver Station") return "limegreen"
                                        const path = robotState.getDashboardPath(modelData)
                                        if (path === "") return "red"
                                        return robotState.isDashboardRunning(modelData) ? "limegreen" : "orange"
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                Button {
                                    text: "Edit"
                                    enabled: modelData !== "Driver Station"
                                    onClicked: {
                                        editDialog.dashboardName = modelData
                                        editDialog.dashboardPath = robotState.getDashboardPath(modelData)
                                        editDialog.dashboardArgs = robotState.getDashboardArgs(modelData)
                                        editDialog.open()
                                    }
                                }
                                
                                Button {
                                    text: "Remove"
                                    enabled: modelData !== "Driver Station"
                                    onClicked: robotState.removeDashboard(modelData)
                                    background: Rectangle {
                                        color: parent.enabled ? (parent.pressed ? "#d32f2f" : "#f44336") : "#666"
                                        radius: 4
                                    }
                                }
                            }
                            
                            Label {
                                text: `Path: ${robotState.getDashboardPath(modelData) || "Not configured"}`
                                color: "lightgray"
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                            }
                            
                            Label {
                                text: `Args: ${robotState.getDashboardArgs(modelData) || "None"}`
                                color: "lightgray"
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                visible: robotState.getDashboardArgs(modelData) !== ""
                            }
                        }
                    }
                }
            }
            
            RowLayout {
                Layout.fillWidth: true
                
                Button {
                    text: "Add Dashboard"
                    onClicked: {
                        editDialog.dashboardName = ""
                        editDialog.dashboardPath = ""
                        editDialog.dashboardArgs = ""
                        editDialog.open()
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "Close"
                    onClicked: dashboardManager.close()
                }
            }
        }
    }
    
    // Edit Dashboard Dialog
    Dialog {
        id: editDialog
        title: dashboardName === "" ? "Add Dashboard" : `Edit ${dashboardName}`
        width: 600
        height: 300
        modal: true
        anchors.centerIn: parent
        
        property string dashboardName: ""
        property string dashboardPath: ""
        property string dashboardArgs: ""
        
        background: Rectangle {
            color: "#2d2d2d"
            radius: 8
            border.color: "#555"
            border.width: 1
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            GridLayout {
                columns: 3
                columnSpacing: 10
                rowSpacing: 10
                Layout.fillWidth: true
                
                Label {
                    text: "Name:"
                    color: "white"
                }
                
                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    text: editDialog.dashboardName
                    placeholderText: "Dashboard Name"
                }
                
                Label {
                    text: "Path:"
                    color: "white"
                }
                
                TextField {
                    id: pathField
                    Layout.fillWidth: true
                    text: editDialog.dashboardPath
                    placeholderText: "Path to executable"
                }
                
                Button {
                    text: "Browse"
                    onClicked: fileDialog.open()
                }
                
                Label {
                    text: "Arguments:"
                    color: "white"
                }
                
                TextField {
                    id: argsField
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    text: editDialog.dashboardArgs
                    placeholderText: "Command line arguments (optional)"
                }
            }
            
            Label {
                text: "Path variables: ~ (home), %USERPROFILE% (Windows), %LOCALAPPDATA% (Windows)"
                color: "lightgray"
                font.pixelSize: 10
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }
            
            RowLayout {
                Layout.fillWidth: true
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "Cancel"
                    onClicked: editDialog.close()
                }
                
                Button {
                    text: "Save"
                    enabled: nameField.text.trim() !== ""
                    onClicked: {
                        robotState.setDashboardPath(nameField.text.trim(), pathField.text.trim(), argsField.text.trim())
                        editDialog.close()
                    }
                }
            }
        }
    }
    
    // File Dialog
    FileDialog {
        id: fileDialog
        title: "Select Dashboard Executable"
        selectExisting: true
        selectMultiple: false
        nameFilters: {
            if (robotState.getCurrentPlatform() === "windows") {
                return ["Executable files (*.exe)", "All files (*)"]
            } else {
                return ["All files (*)"]
            }
        }
        onAccepted: {
            pathField.text = fileDialog.fileUrl.toString().replace("file://", "")
        }
    }
}
