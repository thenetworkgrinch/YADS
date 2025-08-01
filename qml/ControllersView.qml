import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    background: Rectangle { color: "transparent" }
    
    property var boundSlots: ({}) // Map of slot -> controller data
    property var unassignedControllers: ({}) // Map of deviceId -> controller data
    
    // Connect to controller handler signals
    Connections {
        target: controllerHandler
        
        function onControllerConnected(deviceId, name, signature) {
            console.log(`Controller connected: ${name} (ID: ${deviceId}, Sig: ${signature})`)
            // Will be assigned to slot or unassigned list via other signals
        }
        
        function onControllerDisconnected(deviceId) {
            console.log(`Controller disconnected: ID ${deviceId}`)
            // Remove from unassigned list if present
            if (unassignedControllers[deviceId]) {
                delete unassignedControllers[deviceId]
                unassignedRepeater.model = Object.keys(unassignedControllers).length
            }
        }
        
        function onControllerBoundToSlot(slot, deviceId, name, signature) {
            console.log(`Controller bound: ${name} -> Slot ${slot}`)
            
            // Remove from unassigned if present
            if (unassignedControllers[deviceId]) {
                delete unassignedControllers[deviceId]
                unassignedRepeater.model = Object.keys(unassignedControllers).length
            }
            
            // Get controller capabilities from the actual device
            let device = controllerHandler.getControllerInSlot(slot)
            let axisCount = device ? device.axisCount : 6
            let buttonCount = device ? device.buttonCount : 16
            let povCount = device ? device.povCount : 1
            let supportsRumble = device ? device.supportsRumble : false
            
            // Add to bound slots
            boundSlots[slot] = {
                deviceId: deviceId,
                name: name,
                signature: signature,
                axisCount: axisCount,
                buttonCount: buttonCount,
                povCount: povCount,
                supportsRumble: supportsRumble,
                axes: new Array(axisCount).fill(0),
                buttons: new Array(buttonCount).fill(false),
                povs: new Array(povCount).fill(-1)
            }
            
            // Force update of slot display
            slotRepeater.itemAt(slot)?.updateSlotData()
        }
        
        function onControllerUnboundFromSlot(slot) {
            console.log(`Slot ${slot} unbound`)
            if (boundSlots[slot]) {
                // Move to unassigned list
                const deviceData = boundSlots[slot]
                unassignedControllers[deviceData.deviceId] = deviceData
                unassignedRepeater.model = Object.keys(unassignedControllers).length
                
                // Remove from bound slots
                delete boundSlots[slot]
                slotRepeater.itemAt(slot)?.updateSlotData()
            }
        }
        
        // Input signals now use slot instead of deviceId
        function onAxisChanged(slot, axisId, value) {
            if (boundSlots[slot] && axisId < boundSlots[slot].axes.length) {
                boundSlots[slot].axes[axisId] = value
                slotRepeater.itemAt(slot)?.updateAxis(axisId, value)
            }
        }
        
        function onButtonPressed(slot, buttonId) {
            if (boundSlots[slot] && buttonId < boundSlots[slot].buttons.length) {
                boundSlots[slot].buttons[buttonId] = true
                slotRepeater.itemAt(slot)?.updateButton(buttonId, true)
            }
        }
        
        function onButtonReleased(slot, buttonId) {
            if (boundSlots[slot] && buttonId < boundSlots[slot].buttons.length) {
                boundSlots[slot].buttons[buttonId] = false
                slotRepeater.itemAt(slot)?.updateButton(buttonId, false)
            }
        }
        
        function onPovChanged(slot, povId, angle) {
            if (boundSlots[slot] && povId < boundSlots[slot].povs.length) {
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
                text: "FRC Driver Station - Controller Slots"
                font.pixelSize: 20
                font.bold: true
                color: "white"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: "Double-click a controller to bind/unbind it to/from a slot • Supports Xbox, PlayStation, HOTAS, Flight Sticks, and Generic HID controllers"
                font.pixelSize: 12
                color: "lightgray"
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            
            // Bound Slots (0-5)
            GroupBox {
                title: "Controller Slots"
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                
                GridLayout {
                    anchors.fill: parent
                    columns: 2
                    
                    Repeater {
                        id: slotRepeater
                        model: 6 // Fixed 6 slots
                        
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            border.color: "#cccccc"
                            border.width: 1
                            color: controllerHandler.getControllerInSlot(index) ? "#e8f5e8" : "#f8f8f8"
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 5
                                
                                Label {
                                    text: `Slot ${index}:`
                                    font.bold: true
                                }
                                
                                Label {
                                    text: {
                                        var controller = controllerHandler.getControllerInSlot(index)
                                        return controller ? controller.name : "Empty"
                                    }
                                    Layout.fillWidth: true
                                }
                                
                                Button {
                                    text: "Unbind"
                                    visible: controllerHandler.getControllerInSlot(index) !== null
                                    onClicked: controllerHandler.unbindControllerFromSlot(index)
                                }
                            }
                        }
                    }
                }
            }
            
            // Available controllers
            GroupBox {
                title: "Available Controllers"
                Layout.fillWidth: true
                Layout.fillHeight: true
                
                ScrollView {
                    anchors.fill: parent
                    
                    ListView {
                        model: controllerHandler.getAllControllers()
                        
                        delegate: Rectangle {
                            width: parent.width
                            height: 60
                            border.color: "#cccccc"
                            border.width: 1
                            color: "#ffffff"
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                
                                Column {
                                    Layout.fillWidth: true
                                    
                                    Label {
                                        text: modelData.name
                                        font.bold: true
                                    }
                                    
                                    Label {
                                        text: `VID: ${modelData.vendorId.toString(16).toUpperCase().padStart(4, '0')}, PID: ${modelData.productId.toString(16).toUpperCase().padStart(4, '0')}`
                                        font.pixelSize: 10
                                        color: "#666666"
                                    }
                                    
                                    Label {
                                        text: `Axes: ${modelData.axisCount}, Buttons: ${modelData.buttonCount}, POVs: ${modelData.povCount}`
                                        font.pixelSize: 10
                                        color: "#666666"
                                    }
                                }
                                
                                ComboBox {
                                    model: ["Slot 0", "Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5"]
                                    currentIndex: controllerHandler.getSlotForController(modelData.deviceId)
                                    onCurrentIndexChanged: {
                                        if (currentIndex >= 0) {
                                            controllerHandler.bindControllerToSlot(modelData.deviceId, currentIndex)
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
