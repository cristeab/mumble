import QtQuick 2.12
import QtQuick.Controls 2.5
import ".."

Button {
    id: control

    property color backgroundColor: Theme.backgroundColor
    property color textColor: control.enabled ? Theme.textColor : "gray"
    contentItem: Label {
        anchors.centerIn: parent
        text: control.text
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: Theme.windowMargin / 2
        rightPadding: Theme.windowMargin / 2
    }
    background: Rectangle {
        color: control.pressed ? Qt.darker(control.backgroundColor) : control.backgroundColor
        radius: height / 2
        border {
            width: 2
            color: control.textColor
        }
    }
}
