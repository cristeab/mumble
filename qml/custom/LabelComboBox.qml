import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import ".."

Row {
    id: control
    property alias text: controlLabel.text
    property alias model: controlCombo.model
    property alias currentIndex: controlCombo.currentIndex
    spacing: Theme.windowMargin
    Material.accent: Theme.backgroundColor2
    Label {
        id: controlLabel
        anchors.verticalCenter: controlCombo.verticalCenter
        elide: Text.ElideRight
        color: Theme.textColor
        font {
            italic: true
            pointSize: Theme.labelFontSize
        }
    }
    ComboBox {
        id: controlCombo
        background: Rectangle {
            implicitWidth: 150
            implicitHeight: 40
            color: Theme.tableBackgroundColor
        }
        popup.background: Rectangle {
            color: Theme.tableBackgroundColor
        }
    }
}
