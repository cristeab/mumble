import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

TabButton {
    id: control

    display: AbstractButton.IconOnly
    icon.color: control.pressed ? Theme.tabButtonColorSel : Theme.tabButtonColor
    background: Rectangle {
        color: Theme.backgroundColor
    }
    font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
}
