import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import ".."

RadioButton {
    id: control

    Material.accent: Theme.backgroundColor2
    background: Rectangle {
        color: Theme.backgroundColor
    }
    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.down ? Qt.lighter(Theme.textColor) : Theme.textColor
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
