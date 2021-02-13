import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

TableView {
    id: controlTable

    readonly property int delegateHeight: 35
    readonly property var columnWidths: [200, 100, 100]

    visible: 0 < controlTable.rows

    Row {
        id: columnsHeader
        y: controlTable.contentY
        z: 2
        spacing: 2
        Repeater {
            model: controlTable.columns > 0 ? controlTable.columns : 1
            Label {
                width: controlTable.columnWidthProvider(modelData) - (modelData === 2 ? 0 : 2)
                height: controlTable.delegateHeight
                text: servers.headerData(modelData, Qt.Horizontal)
                color: "white"
                font.pixelSize: 15
                padding: Theme.windowMargin
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { color: Theme.tableBackgroundColor }
            }
        }
    }
    topMargin: columnsHeader.implicitHeight + 3

    columnWidthProvider: function (column) {
        return controlTable.columnWidths[column]
    }
    width: 400 + (controlTable.columns - 1) * Theme.windowMargin
    model: servers
    clip: true
    boundsBehavior: ListView.StopAtBounds
    columnSpacing: 0
    rowSpacing: 0
    delegate:  Label {
        padding: Theme.windowMargin
        text: display
        height: controlTable.delegateHeight
        clip: true
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        rightPadding: Theme.windowMargin
        background: Rectangle { color: Theme.tableBackgroundColor }
    }
}
