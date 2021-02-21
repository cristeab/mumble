import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Row {
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        spacing: Theme.windowMargin
        LabelComboBox {
            text: qsTr("System")
            model: audioDevices.outputSystems
            enabled: 1 < audioDevices.outputSystems.length
            currentIndex: audioDevices.outputSystemIndex
            onCurrentIndexChanged: audioDevices.outputSystemIndex = currentIndex
        }
        LabelComboBox {
            text: qsTr("Device")
            model: audioDevices.outputDevices
            enabled: 1 < audioDevices.outputDevices.length
            currentIndex: audioDevices.outputDeviceIndex
            onCurrentIndexChanged: audioDevices.outputDeviceIndex = currentIndex
        }
    }
}
