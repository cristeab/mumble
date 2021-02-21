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
            model: audioDevices.inputSystems
            enabled: 1 < audioDevices.inputSystems.length
            currentIndex: audioDevices.inputSystemIndex
            onCurrentIndexChanged: audioDevices.inputSystemIndex = currentIndex
        }
        LabelComboBox {
            text: qsTr("Device")
            model: audioDevices.inputDevices
            enabled: 1 < audioDevices.inputDevices.length
            currentIndex: audioDevices.inputDeviceIndex
            onCurrentIndexChanged: audioDevices.inputDeviceIndex = currentIndex
        }
    }
}
