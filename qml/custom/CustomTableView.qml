import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

TableView {
    id: controlTable
    readonly property int delegateHeight: 30
    model: softphone.audioCodecs
    clip: true
    boundsBehavior: ListView.StopAtBounds
    delegate:  Label {
        text: display
        width: delegateSpinBox.width
        height: controlTable.delegateHeight
        clip: true
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignRight
        rightPadding: Theme.windowMargin
    }
}
