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
            text: qsTr("Mute speakers")
            checked: audioDevices.outputDeviceMute
            onCheckedChanged: audioDevices.outputDeviceMute = checked
        }
        CustomCheckBox {
            text: qsTr("Reconnect automatically")
            checked: audioDevices.outputDeviceAutoConnect
            onCheckedChanged: audioDevices.outputDeviceAutoConnect = checked
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
