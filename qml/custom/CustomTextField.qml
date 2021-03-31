import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import ".."

TextField {
    id: control
    property bool error: false
    selectByMouse: true
    color: Theme.textColor
    Material.accent: Theme.textColor
    font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
        Rectangle {
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
            color: "transparent"
            border {
                width: 2
                color: control.error ? Theme.errorColor : Theme.borderColor
            }
        }
    }
}
