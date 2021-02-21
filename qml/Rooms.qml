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
        cellWidth: (roomsGrid.width - (roomsGrid.column - 1) * Theme.windowMargin) / roomsGrid.columns
        cellHeight: 250
        currentIndex: 0
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: servers.roomsModel
        delegate: ListView {
            id: usersList

            Label {
                id: usersListHeader
                width: parent.width
                clip: true
                elide: Text.ElideRight
                text: name
                color: Theme.textColor2
                font.pixelSize: 15
                padding: Theme.windowMargin / 2
                verticalAlignment: Text.AlignVCenter
                background: Rectangle { color: Theme.tableBackgroundColor }
            }
            topMargin: usersListHeader.implicitHeight

            width: roomsGrid.cellWidth
            height: roomsGrid.cellHeight
            currentIndex: 0
            clip: true
            boundsBehavior: ListView.StopAtBounds
            model: users
            delegate: Label {
                width: parent.width
                padding: Theme.windowMargin / 2
                text: modelData
                color: Theme.textColor
                clip: true
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                background: Rectangle {
                    color: usersList.GridView.isCurrentItem ?  Theme.backgroundColor2 : Theme.backgroundColor
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: roomsGrid.currentIndex = index
                onDoubleClicked: {
                    //TODO: join room
                }
            }
        }
    }
}
