import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: CustomBackground {}

    Component.onCompleted: roomsGrid.currentIndex = 0

    Row {
        anchors {
            top: parent.top
            topMargin: 1.5 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
        }
        visible: (servers.currentClassIndex === servers.connectedClassIndex)
        spacing: Theme.windowMargin
        CustomImageButton {
            id: micBtn
            scale: servers.roomsModel.microphoneOff ? 1.45 : 1
            source: servers.roomsModel.microphoneOff ? "qrc:/img/microphone-off.svg" : "qrc:/img/microphone.svg"
            width: 0.8 * Theme.smallTabIconWidth
            height: Theme.smallTabIconWidth
            onClicked: {
                servers.roomsModel.microphoneOff = !servers.roomsModel.microphoneOff
                if (servers.roomsModel.speakerOff) {
                    servers.roomsModel.speakerOff = false
                }
                if (servers.roomsModel.microphoneOff) {
                    volBtn.changeMic = false
                }
            }
        }
        CustomImageButton {
            id: volBtn
            property bool changeMic: false
            source: servers.roomsModel.speakerOff ? "qrc:/img/volume-off.svg" : "qrc:/img/volume.svg"
            width: Theme.smallTabIconWidth
            height: Theme.smallTabIconWidth
            onClicked: {
                servers.roomsModel.speakerOff = !servers.roomsModel.speakerOff
                if (volBtn.changeMic) {
                    servers.roomsModel.microphoneOff = !servers.roomsModel.microphoneOff
                    return
                }
                if (!servers.roomsModel.microphoneOff) {
                    servers.roomsModel.microphoneOff = true
                    volBtn.changeMic = true
                } else {
                    volBtn.changeMic = false
                }
            }
        }
    }

    Label {
        anchors {
            top: parent.top
            topMargin: 2 * Theme.windowMargin
            horizontalCenter: parent.horizontalCenter
        }
        text: servers.currentClassName
        color: Theme.textColor
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        clip: true
        elide: Text.ElideRight
    }

    GridView {
        id: roomsGrid

        property int channelIndex: -1
        property string userName: ""

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: joinBtn.top
        }
        cellWidth: roomsGrid.width / 4
        cellHeight: 400
        currentIndex: 0
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: servers.roomsModel
        delegate: DropArea {
            id: delegateControl

            width: roomsGrid.cellWidth
            height: roomsGrid.cellHeight

            onEntered: {
                console.log("Drag entered  " + index)
                roomsGrid.channelIndex = index
            }
            onExited: {
                console.log("Drag exited  " + index)
            }
            onDropped: {
                console.log("Drag dropped " + index)
            }

            Rectangle {
                anchors.fill: usersList
                color: delegateControl.GridView.isCurrentItem ? Theme.backgroundColor2 : Theme.backgroundColor
                border {
                    width: 1
                    color: Theme.tableBackgroundColor
                }
                radius: Theme.rectRadius
            }
            ListView {
                id: usersList

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
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
                    padding: Theme.windowMargin / 2
                    verticalAlignment: Text.AlignVCenter
                    background: Rectangle {
                        radius: Theme.rectRadius
                        color: Theme.tableBackgroundColor
                    }
                    color: Theme.textColor2
                    font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
                }
                topMargin: usersListHeader.implicitHeight

                currentIndex: 0
                clip: true
                boundsBehavior: ListView.StopAtBounds
                model: users
                delegate: Label {
                    width: parent.width
                    padding: Theme.windowMargin / 2
                    text: (servers.currentUsername === modelData) ? ("<b><big>" + modelData + "</big></b>") : modelData
                    font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
                    color: delegateControl.GridView.isCurrentItem ? Theme.textColor2 : Theme.textColor
                    clip: true
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    background: Item {}
                    Drag.active: dragArea.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: 0
                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        drag.target: parent
                        hoverEnabled: true
                        onPressed: {
                            roomsGrid.channelIndex = -1
                            roomsGrid.userName = modelData
                            mouse.accepted = true
                        }
                        onReleased: {
                            console.log("Mouse released: user " + roomsGrid.userName + ", channel " + roomsGrid.channelIndex)
                            if (-1 !== roomsGrid.channelIndex) {
                                servers.joinRoom(roomsGrid.channelIndex, roomsGrid.userName)
                            }
                            mouse.accepted = true
                        }
                    }
                }
                MouseArea {
                    propagateComposedEvents: true
                    anchors.fill: parent
                    onPressed: {
                        roomsGrid.currentIndex = index
                        mouse.accepted = false
                    }
                    onReleased: {
                        roomsGrid.currentIndex = index
                        mouse.accepted = false
                    }
                }
            }
        }
    }
    CustomButton {
        id: joinBtn

        function joinAction(idx) {
            if (servers.joinRoom(idx, servers.username)) {
                servers.connectedClassIndex = servers.currentClassIndex
            }
        }

        enabled: (servers.roomsModel.currentRoomIndex !== roomsGrid.currentIndex) || (servers.currentClassIndex !== servers.connectedClassIndex)
        anchors {
            left: roomsGrid.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        text: qsTr("Join room")
        onClicked: joinBtn.joinAction(roomsGrid.currentIndex)
    }
}
