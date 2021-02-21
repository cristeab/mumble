import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Component.onCompleted: roomsGrid.currentIndex = 0
    Component.onDestruction: servers.roomsModel.clear()

    Label {
        anchors {
            top: parent.top
            topMargin: 3 * Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        text: servers.currentClassName
        color: Theme.textColor
        font.pixelSize: 16
        clip: true
        elide: Text.ElideRight
    }

    GridView {
        id: roomsGrid

        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: parent.bottom
        }
        cellWidth: roomsGrid.width / 4
        cellHeight: 250
        currentIndex: 0
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: servers.roomsModel
        delegate: Item {
            id: delegateControl

            width: roomsGrid.cellWidth
            height: roomsGrid.cellHeight

            Rectangle {
                anchors.fill: usersList
                color: delegateControl.GridView.isCurrentItem ?  Theme.backgroundColor2 : Theme.backgroundColor
            }
            ListView {
                id: usersList

                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: Theme.windowMargin
                    right: parent.right
                    rightMargin: Theme.windowMargin
                    bottom: parent.bottom
                    bottomMargin: Theme.windowMargin
                }

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
                    background: Item {}
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: roomsGrid.currentIndex = index
                    onDoubleClicked: {
                        roomsGrid.currentIndex = index
                        servers.joinRoom(roomsGrid.currentIndex)
                    }
                }
            }
        }
    }
}
