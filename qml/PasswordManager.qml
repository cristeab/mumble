import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    ListView {
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
        model: tokensModel
        delegate: SwipeDelegate {
            id: delegateControl
            height: 35
            background: Rectangle {
                color: Theme.backgroundColor
            }
            contentItem: CustomTextField {
                id: contentTextField
                text: display
                onTextChanged: edit = text
                readOnly: !focus
            }
            swipe.enabled: true
            swipe.right: Rectangle {
                height: parent.height
                width: 70
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
                        color: Theme.textColor
                        clip: true
                        elide: Text.ElideRight
                        verticalAlignment: Label.AlignVCenter
                        width: parent.width
                        horizontalAlignment: Label.AlignHCenter
                        topPadding: 3
                    }
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
