import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Row {
        id: checkBoxRow
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 2 * Theme.windowMargin
        CustomCheckBox {
            text: qsTr("Mute microphone")
            checked: audioDevices.inputDeviceMute
            onCheckedChanged: audioDevices.inputDeviceMute = checked
        }
        CustomCheckBox {
            text: qsTr("Reconnect automatically")
            checked: audioDevices.inputDeviceAutoConnect
            onCheckedChanged: audioDevices.inputDeviceAutoConnect = checked
        }
    }

    Row {
        anchors {
            top: checkBoxRow.bottom
            topMargin: Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 2 * Theme.windowMargin
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
