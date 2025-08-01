import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    background: Rectangle { color: "transparent" }
    
    property var boundSlots: ({}) // Map of slot -> joystick data
    property var unassignedJoysticks: ({}) // Map of deviceId -> joystick data
    
    // Connect to joystick handler signals
    Connections {
        target: joystickHandler
        
        function onJoystickConnected(deviceId, name, signature) {
            console.log(`Joystick connected: ${name} (ID: ${deviceId}, Sig: ${signature})`)
            // Will be assigned to slot or unassigned list via other signals
        }
        
        function onJoystickDisconnected(deviceId) {
            console.log(`Joystick disconnected: ID ${deviceId}`)
            // Remove from unassigned list if present
            if (unassignedJoysticks[deviceId]) {
                delete unassignedJoysticks[deviceId]
                unassignedRepeater.model = Object.keys(unassignedJoysticks).length
            }
        }
        
        function onJoystickBoundToSlot(slot, deviceId, name, signature) {
            console.log(`Joystick bound: ${name} -> Slot ${slot}`)
            
            // Remove from unassigned if present
            if (unassignedJoysticks[deviceId]) {
                delete unassignedJoysticks[deviceId]
                unassignedRepeater.model = Object.keys(unassignedJoysticks).length
            }
            
            // Add to bound slots
            boundSlots[slot] = {
                deviceId: deviceId,
                name: name,
                signature: signature,
                axes: [0, 0, 0, 0, 0, 0],
                buttons: new Array(16).fill(false),
                povs: [-1, -1, -1, -1]
            }
            
            // Force update of slot display
            slotRepeater.itemAt(slot)?.updateSlotData()
        }
        
        function onJoystickUnboundFromSlot(slot) {
            console.log(`Slot ${slot} unbound`)
            if (boundSlots[slot]) {
                // Move to unassigned list
                const deviceData = boundSlots[slot]
                unassignedJoysticks[deviceData.deviceId] = deviceData
                unassignedRepeater.model = Object.keys(unassignedJoysticks).length
                
                // Remove from bound slots
                delete boundSlots[slot]
                slotRepeater.itemAt(slot)?.updateSlotData()
            }
        }
        
        // Input signals now use slot instead of deviceId
        function onAxisChanged(slot, axisId, value) {
            if (boundSlots[slot]) {
                boundSlots[slot].axes[axisId] = value
                slotRepeater.itemAt(slot)?.updateAxis(axisId, value)
            }
        }
        
        function onButtonPressed(slot, buttonId) {
            if (boundSlots[slot]) {
                boundSlots[slot].buttons[buttonId] = true
                slotRepeater.itemAt(slot)?.updateButton(buttonId, true)
            }
        }
        
        function onButtonReleased(slot, buttonId) {
            if (boundSlots[slot]) {
                boundSlots[slot].buttons[buttonId] = false
                slotRepeater.itemAt(slot)?.updateButton(buttonId, false)
            }
        }
        
        function onPovChanged(slot, povId, angle) {
            if (boundSlots[slot]) {
                boundSlots[slot].povs[povId] = angle
                slotRepeater.itemAt(slot)?.updatePOV(povId, angle)
            }
        }
        
        function onLeftStickMoved(slot, x, y) {
            console.log(`Slot ${slot} left stick: (${x.toFixed(3)}, ${y.toFixed(3)})`)
        }
        
        function onActionButtonPressed(slot, buttonType) {
            const buttonNames = ["A", "B", "X", "Y"]
            console.log(`Slot ${slot} ${buttonNames[buttonType]} pressed`)
        }
    }

    ScrollView {
        anchors.fill: parent
        
        ColumnLayout {
            width: parent.width
            spacing: 20
            
            // Header
            Label {
                text: "FRC Driver Station - Joystick Slots"
                font.pixelSize: 20
                font.bold: true
                color: "white"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: "Double-click a joystick to bind/unbind it to/from a slot"
                font.pixelSize: 12
                color: "lightgray"
                Layout.alignment: Qt.AlignHCenter
            }
            
            // Bound Slots (0-5)
            GroupBox {
                title: "Joystick Slots (0-5)"
                Layout.fillWidth: true
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    columnSpacing: 20
                    rowSpacing: 15
                    
                    Repeater {
                        id: slotRepeater
                        model: 6 // Fixed 6 slots
                        
                        delegate: Frame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 200
                            
                            property var slotData: boundSlots[index] || null
                            property bool isOccupied: slotData !== null
                            
                            function updateSlotData() {
                                slotData = boundSlots[index] || null
                                isOccupied = slotData !== null
                            }
                            
                            function updateAxis(axisId, value) {
                                if (slotData && axisId < axisValues.length) {
                                    axisValues[axisId] = value
                                    axisRepeater.itemAt(axisId)?.forceUpdate()
                                }
                            }
                            
                            function updateButton(buttonId, pressed) {
                                if (slotData && buttonId < buttonStates.length) {
                                    buttonStates[buttonId] = pressed
                                    buttonRepeater.itemAt(buttonId)?.forceUpdate()
                                }
                            }
                            
                            function updatePOV(povId, angle) {
                                if (slotData && povId < povAngles.length) {
                                    povAngles[povId] = angle
                                    povRepeater.itemAt(povId)?.forceUpdate()
                                }
                            }
                            
                            property var axisValues: slotData ? slotData.axes : []
                            property var buttonStates: slotData ? slotData.buttons : []
                            property var povAngles: slotData ? slotData.povs : []
                            
                            background: Rectangle {
                                color: isOccupied ? "#404040" : "#2a2a2a"
                                radius: 8
                                border.color: isOccupied ? "limegreen" : "gray"
                                border.width: 2
                            }
                            
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                
                                // Slot header
                                RowLayout {
                                    Layout.fillWidth: true
                                    
                                    Label {
                                        text: `Slot ${index}:`
                                        font.bold: true
                                        font.pixelSize: 14
                                        color: "white"
                                    }
                                    
                                    Label {
                                        text: isOccupied ? slotData.name : "Empty"
                                        font.pixelSize: 12
                                        color: isOccupied ? "limegreen" : "gray"
                                        Layout.fillWidth: true
                                    }
                                    
                                    Button {
                                        text: "Clear"
                                        visible: isOccupied
                                        onClicked: joystickHandler.unbindSlot(index)
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
                                
                                // Joystick data (if occupied)
                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    visible: isOccupied
                                    
                                    RowLayout {
                                        // Axes
                                        ColumnLayout {
                                            Label {
                                                text: "Axes"
                                                font.bold: true
                                                color: "white"
                                            }
                                            
                                            Repeater {
                                                id: axisRepeater
                                                model: 6
                                                
                                                RowLayout {
                                                    property real currentValue: axisValues && index < axisValues.length ? axisValues[index] : 0
                                                    
                                                    function forceUpdate() {
                                                        currentValue = axisValues[index]
                                                    }
                                                    
                                                    Label { 
                                                        text: `${index}:`
                                                        color: "white"
                                                        Layout.preferredWidth: 20
                                                    }
                                                    ProgressBar {
                                                        value: (currentValue + 1.0) / 2.0
                                                        Layout.preferredWidth: 80
                                                        Layout.preferredHeight: 15
                                                    }
                                                    Label {
                                                        text: currentValue.toFixed(2)
                                                        color: "white"
                                                        Layout.preferredWidth: 40
                                                        font.pixelSize: 10
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // Buttons
                                        ColumnLayout {
                                            Label {
                                                text: "Buttons"
                                                font.bold: true
                                                color: "white"
                                            }
                                            
                                            Flow {
                                                Layout.preferredWidth: 120
                                                spacing: 2
                                                
                                                Repeater {
                                                    id: buttonRepeater
                                                    model: 16
                                                    
                                                    Rectangle {
                                                        width: 20; height: 20
                                                        radius: 2
                                                        border.color: "gray"
                                                        border.width: 1
                                                        
                                                        property bool isPressed: buttonStates && index < buttonStates.length ? buttonStates[index] : false
                                                        color: isPressed ? "limegreen" : "#2d2d2d"
                                                        
                                                        function forceUpdate() {
                                                            isPressed = buttonStates[index]
                                                        }
                                                        
                                                        Label {
                                                            text: index
                                                            anchors.centerIn: parent
                                                            color: "white"
                                                            font.pixelSize: 8
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Unassigned Joysticks
            GroupBox {
                title: "Unassigned Joysticks"
                Layout.fillWidth: true
                
                ColumnLayout {
                    anchors.fill: parent
                    
                    Label {
                        text: "Double-click to bind to an available slot"
                        font.pixelSize: 12
                        color: "lightgray"
                    }
                    
                    Repeater {
                        id: unassignedRepeater
                        model: 0
                        
                        delegate: Frame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 60
                            
                            property var deviceData: {
                                const keys = Object.keys(unassignedJoysticks)
                                if (index < keys.length) {
                                    const deviceId = keys[index]
                                    return {
                                        deviceId: parseInt(deviceId),
                                        ...unassignedJoysticks[deviceId]
                                    }
                                }
                                return null
                            }
                            
                            background: Rectangle {
                                color: "#353535"
                                radius: 8
                                border.color: "orange"
                                border.width: 1
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                onDoubleClicked: {
                                    if (deviceData) {
                                        joystickHandler.handleDoubleClick(deviceData.deviceId)
                                    }
                                }
                            }
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                
                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: "orange"
                                }
                                
                                Label {
                                    text: deviceData ? deviceData.name : "Unknown"
                                    font.pixelSize: 14
                                    color: "white"
                                    Layout.fillWidth: true
                                }
                                
                                Label {
                                    text: deviceData ? `ID: ${deviceData.deviceId}` : ""
                                    font.pixelSize: 12
                                    color: "lightgray"
                                }
                            }
                        }
                    }
                    
                    Label {
                        text: unassignedRepeater.model === 0 ? "No unassigned joysticks" : ""
                        color: "gray"
                        font.italic: true
                        visible: unassignedRepeater.model === 0
                    }
                }
            }
            
            // Placeholder for Joysticks View implementation
            Item {
                Label {
                    anchors.centerIn: parent
                    text: "Joysticks View\n(Implementation placeholder)"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 18
                    color: "#888888"
                }
            }
        }
    }
}
