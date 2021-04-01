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
        id: comboBoxRow
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

    Label {
        id: volLabel
        anchors {
            top: comboBoxRow.bottom
            topMargin: 2 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
        }
        color: Theme.textColor
        text: qsTr("Audio output")
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }
    CustomSlider {
        id: volSlider
        anchors {
            top: volLabel.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        from: 0
        to: 200
        stepSize: 1
        value: audioDevices.volume
        onValueChanged: audioDevices.volume = value
        leftText: qsTr("Volume")
        rightText: value + " %"
        ToolTip {
            visible: volSlider.hovered
            text: qsTr("Volume of incoming speech")
        }
    }
    CustomSlider {
        id: delaySlider
        anchors {
            top: volSlider.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        from: 1
        to: 10
        stepSize: 1
        value: audioDevices.outputDelay
        onValueChanged: audioDevices.outputDelay = value
        leftText: qsTr("Output delay")
        rightText: (10 * value) + " ms"
        ToolTip {
            visible: delaySlider.hovered
            text: qsTr("Amount of data to buffer")
        }
    }
    CustomSlider {
        id: attSlider
        anchors {
            top: delaySlider.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        enabled: audioDevices.whileOtherUsersTalk || audioDevices.whileYouTalk
        from: 0
        to: 100
        stepSize: 1
        value: audioDevices.attenuation
        onValueChanged: audioDevices.attenuation = value
        leftText: qsTr("Attenuate applications by...")
        rightText: value + " %"
        ToolTip {
            visible: attSlider.hovered
            text: qsTr("Attenuation of other applications during speech")
        }
    }
    Row {
        anchors {
            top: attSlider.bottom
            topMargin: Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        spacing: 3 * Theme.windowMargin
        CustomCheckBox {
            id: whileOtherCheckBox
            text: qsTr("while other users talk")
            checked: audioDevices.whileOtherUsersTalk
            onCheckedChanged: audioDevices.whileOtherUsersTalk = checked
            ToolTip {
                visible: whileOtherCheckBox.hovered
                text: qsTr("If checked Bubbles lowers the volume of other applications while other users talk")
            }
        }
        CustomCheckBox {
            id: whileYouCheckBox
            text: qsTr("while you talk")
            checked: audioDevices.whileYouTalk
            onCheckedChanged: audioDevices.whileYouTalk = checked
            ToolTip {
                visible: whileYouCheckBox.hovered
                text: qsTr("If checked Bubbles lowers the volume of other applications while you talk")
            }
        }
    }
}
