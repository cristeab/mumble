import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

TableView {
    id: controlTable

    readonly property var columnWidths: [440, 120, 120]
    property int currentRow: 0

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
    model: servers
    clip: true
    boundsBehavior: ListView.StopAtBounds
    columnSpacing: 0
    rowSpacing: 2
    delegate:  Label {
        id: controlDelegate

        property int row: index % controlTable.rows
        property int col: index / controlTable.rows

        padding: Theme.windowMargin
        text: ((0 === col) || servers.isReachable(row)) ? display : ""
        color: Theme.textColor2
        clip: true
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        background: Rectangle {
            color: (controlDelegate.row !== controlTable.currentRow) ? Theme.tableBackgroundColor : Theme.tableSelectedBackgroundColor
        }
        MouseArea {
            anchors.fill: parent
            onClicked: controlTable.currentRow = controlDelegate.row
        }
    }
}
