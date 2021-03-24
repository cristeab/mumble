import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

RangeSlider {
    id: control

    property real backgroundValue: 0

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.height
        radius: 2
        color: "#bdbebf"

        Rectangle {
            anchors {
                left: parent.left
                right: middleRect.left
            }
            height: middleRect.height
            color: Theme.sliderBelowColor
            radius: 2
        }
        Rectangle {
            id: middleRect
            x: control.first.visualPosition * parent.width
            width: control.second.visualPosition * parent.width - x
            height: parent.height
            color: Theme.sliderMiddleColor
            radius: 2
        }
        Rectangle {
            anchors {
                left: middleRect.right
                right: parent.right
            }
            height: middleRect.height
            color: Theme.sliderAboveColor
            radius: 2
        }
        Rectangle {
            anchors.right: parent.right
            width: (1 - control.backgroundValue) * parent.width
            height: middleRect.height
            color: Theme.sliderBackgroundColor
            radius: 2
        }
    }

    first.handle: Rectangle {
        x: control.leftPadding + control.first.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: Theme.sliderWidth
        implicitHeight: control.height + Theme.windowMargin
        radius: 3
        color: control.first.pressed ? Theme.sliderPressedColor : Theme.sliderReleasedColor
        border.color: Theme.sliderBorderColor
        ToolTip {
            visible: control.first.hovered
            text: qsTr("Signal values below this count as silence")
        }
    }

    second.handle: Rectangle {
        x: control.leftPadding + control.second.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: Theme.sliderWidth
        implicitHeight: control.height + Theme.windowMargin
        radius: 3
        color: control.second.pressed ? Theme.sliderPressedColor : Theme.sliderReleasedColor
        border.color: Theme.sliderBorderColor
        ToolTip {
            visible: control.second.hovered
            text: qsTr("Signal values above this count as voice")
        }
    }
}
