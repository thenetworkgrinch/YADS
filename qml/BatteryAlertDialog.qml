import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: dialog
    title: "Battery Alert"
    width: 350
    height: 200
    modal: true
    
    property int alertLevel: 1 // 0 = critical, 1 = warning
    property real voltage: 0.0
    
    Rectangle {
        anchors.fill: parent
        color: alertLevel === 0 ? "#ffebee" : "#fff3e0"
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 15
            
            Image {
                source: alertLevel === 0 ? "qrc:/icons/battery-critical.png" : "qrc:/icons/battery-warning.png"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
            }
            
            Label {
                text: alertLevel === 0 ? "CRITICAL BATTERY VOLTAGE" : "LOW BATTERY WARNING"
                font.bold: true
                font.pixelSize: 16
                color: alertLevel === 0 ? "#d32f2f" : "#f57c00"
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: `Current voltage: ${voltage.toFixed(2)}V`
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }
            
            Label {
                text: alertLevel === 0 ? 
                      "Robot will be automatically disabled to prevent damage." :
                      "Consider replacing or charging the battery soon."
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
            }
            
            Button {
                text: "Acknowledge"
                Layout.alignment: Qt.AlignHCenter
                onClicked: dialog.close()
            }
        }
    }
}
