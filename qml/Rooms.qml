import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    GridView {
        id: roomsGrid

        readonly property int columns: 3

        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        cellWidth: (parent.width - (roomsGrid.columns - 1) * Theme.windowMargin) / roomsGrid.columns
        cellHeight: 300
        currentIndex: 0
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: servers.roomsModel
        delegate: ListView {
            id: usersList

            property int gridIndex: index

            Label {
                id: usersListHeader
                width: parent.width
                clip: true
                elide: Text.ElideRight
                text: name
                color: Theme.textColor2
                font.pixelSize: 15
                padding: Theme.windowMargin
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { color: Theme.tableBackgroundColor }
            }
            topMargin: usersListHeader.implicitHeight + 3

            width: roomsGrid.cellWidth
            height: 6 * usersListHeader.height
            currentIndex: 0
            clip: true
            boundsBehavior: ListView.StopAtBounds
            model: users
            delegate: Label {
                width: parent.width
                padding: Theme.windowMargin
                text: modelData
                color: Theme.textColor2
                clip: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                background: Rectangle {
                    color: (roomsGrid.currentIndex === usersList.gridIndex) ? Theme.backgroundColor : Theme.backgroundColor2
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: roomsGrid.currentIndex = usersList.gridIndex
                onDoubleClicked: {
                    //TODO: join room
                }
            }
        }
    }
}
