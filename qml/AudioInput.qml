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
        id: comboBoxRow
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

    Label {
        id: volLabel
        anchors {
            top: comboBoxRow.bottom
            topMargin: 2 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
        }
        color: Theme.textColor
        text: qsTr("Sound limitation")
    }
    CustomRangeSlider {
        id: sliderFrame
        anchors {
            top: volLabel.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        height: 30
        from: 0
        to: 1
        first.value: audioDevices.sliderBelowValue
        first.onValueChanged: audioDevices.sliderBelowValue = first.value
        second.value: audioDevices.sliderAboveValue
        second.onValueChanged: audioDevices.sliderAboveValue = second.value
        backgroundValue: audioDevices.micValue
    }

    IconLabel {
        id: infoLabel
        anchors {
            top: sliderFrame.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("Adjust the sliders such that when you speak the voice exceeds the green field, but when you are quiet the background noise is in the red field.")
    }

    Label {
        id: compLabel
        anchors {
            top: infoLabel.bottom
            topMargin: 2 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
        }
        color: Theme.textColor
        text: qsTr("Compression")
    }
    CustomSlider {
        id: qualitySlider
        anchors {
            top: compLabel.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        from: 8000
        to: 192000
        stepSize: 1000
        value: audioDevices.quality
        onValueChanged: audioDevices.quality = value
        leftText: qsTr("Quality")
        rightText: (value / 1000) + " kb/s"
        ToolTip {
            visible: qualitySlider.hovered
            text: qsTr("Quality of compression (peak bandwidth)")
        }
    }
    CustomSlider {
        id: framesSlider

        anchors {
            top: qualitySlider.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        from: 1
        to: 4
        stepSize: 1
        value: audioDevices.frames
        onValueChanged: audioDevices.frames = value
        leftText: qsTr("Audio per packet")
        rightText: (10 * audioDevices.framesPerPacket(value)) + " ms"
        ToolTip {
            visible: framesSlider.hovered
            text: qsTr("How many audio frames to send per packet")
        }
    }
    Label {
        anchors {
            top: framesSlider.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        clip: true
        text: audioDevices.bitRateText
        color: audioDevices.bitRateAlarm ? Theme.errorColor : Theme.textColor
    }
}
