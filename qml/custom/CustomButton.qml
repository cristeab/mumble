import QtQuick 2.12
import QtQuick.Controls 2.5
import ".."

Button {
    id: control
    property color backgroundColor: "black"
    property color textColor: "white"
    contentItem: Label {
        anchors.centerIn: parent
        text: control.text
        color: control.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pointSize: Theme.buttonFontSize
    }
    background: Rectangle {
        color: control.pressed?Qt.darker(control.backgroundColor):control.backgroundColor
        height: control.height
        width: control.width
        radius: 3
    }
}
