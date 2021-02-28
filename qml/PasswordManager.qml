import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }
    Component.onCompleted: tokensModel.currentInde = 0

    ListView {
        id: tokensList
        readonly property int swipeWidth: 70
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: addButton.top
        }
        clip: true
        boundsBehavior: ListView.StopAtBounds
        spacing: 2
        model: tokensModel
        delegate: SwipeDelegate {
            id: delegateControl
            width: parent.width
            background: Rectangle {
                color: (index === tokensModel.currentIndex) ? Theme.tableSelectedBackgroundColor : Theme.tableBackgroundColor
            }
            contentItem: Label {
                id: contentTextField
                Behavior on x {
                    enabled: !delegateControl.down
                    NumberAnimation {
                        easing.type: Easing.InOutCubic
                        duration: 400
                    }
                }
                text: name
                padding: Theme.windowMargin / 2
            }
            onClicked: tokensModel.currentIndex = index
            swipe.enabled: true
            swipe.right: Rectangle {
                height: parent.height
                width: tokensList.swipeWidth
                anchors.right: parent.right
                color: SwipeDelegate.pressed ? Qt.darker(Theme.swipeRemoveItemColor, 1.1) : Theme.swipeRemoveItemColor
                SwipeDelegate.onClicked: {
                    tokensModel.remove(index)
                    delegateControl.swipe.close()
                }
                Column {
                    anchors.fill: parent
                    spacing: 0
                    Item {
                        height: 5
                        width: parent.width
                    }
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height/2 - 6
                        width: height

                        source: "qrc:/img/trash-solid.svg"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                        scale: 0.8
                    }
                    Label {
                        id: deleteLabel
                        text: qsTr("Delete")
                        color: Theme.tabButtonColor
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
                    }
                }
            }
            swipe.left: Rectangle {
                height: parent.height
                width: tokensList.swipeWidth
                anchors.left: parent.left
                color: SwipeDelegate.pressed ? Qt.darker(Theme.swipeEditItemColor, 1.1) : Theme.swipeEditItemColor
                SwipeDelegate.onClicked: {
                    tokensModel.currentEditIndex = index
                    delegateControl.swipe.close()
                }
                Column {
                    anchors.fill: parent
                    spacing: 0
                    Item {
                        height: 5
                        width: parent.width
                    }
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: parent.height/2 - 6
                        width: height
                        source: "qrc:/img/edit-solid.svg"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                    }
                    Label {
                        id: editLabel
                        text: qsTr("Edit")
                        color: Theme.tabButtonColor
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
                        wrapMode: Text.WordWrap
                    }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.tabButtonColor
                }
            }
        }
    }

    TabButton {
        id: addButton
        anchors {
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: infoLabel.top
        }
        background: Rectangle {
            color: Theme.backgroundColor
        }
        display: AbstractButton.IconOnly
        icon {
            source: "qrc:/img/plus-circle-solid.svg"
            color: addButton.pressed ? Theme.tabButtonColorSel : Theme.tabButtonColor
        }
        onClicked: tokensModel.add()
        height: 60
        width: height
        ToolTip {
            visible: addButton.hovered
            text: qsTr("Add a token")
        }
    }
    IconLabel {
        id: infoLabel
        anchors {
            bottom: parent.bottom
            bottomMargin: 2 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("This is an editable list of access tokens on the connected server. An access token is a text string, which can be used as a password for very simple access management on channels. Mumble will remember the tokens you've used and resend them to the server next time you reconnect, so you don't have to enter these every time.")
    }
}
