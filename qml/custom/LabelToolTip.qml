import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

Label {
    id: controlLabel
    property alias toolTipZ: controlToolTip.z
    property bool forceTooltip: false
    wrapMode: Text.WordWrap
    clip: true
    elide: Text.ElideRight
    color: Theme.textColor
    MouseArea {
        id: controlLabelMouseArea
        enabled: controlLabel.truncated || controlLabel.forceTooltip
        anchors.fill: parent
        hoverEnabled: true
    }
    ToolTip {
        id: controlToolTip
        visible: controlLabelMouseArea.containsMouse
        text: controlLabel.text
    }
}
