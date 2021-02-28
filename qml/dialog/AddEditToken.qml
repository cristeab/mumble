import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    function cleanup() {
        addEditTokenDlg.active = false
    }

    title: (0 <= tokensModel.currentIndex) ? qsTr("Edit token") : qsTr("Add token")
    implicitWidth: 400
    implicitHeight: tokenTextField.height + 120
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2

    background: Rectangle {
        color: Theme.backgroundColor
        border {
            width: 1
            color: Theme.tabButtonColor
        }
    }
    onAccepted: {
        servers.setCurrentToken(tokenTextField.text)
        control.cleanup()
    }
    onRejected: control.cleanup()
    Component.onCompleted: control.visible = true

    CustomTextField {
        id: tokenTextField
        width: parent.width
        echoMode: TextInput.Normal
        text: tokensModel.currentToken()
        color: Theme.textColor
        placeholderText: qsTr("Token")
    }

    header: Rectangle {
        color: Theme.backgroundColor
        border {
            width: 1
            color: Theme.tabButtonColor
        }
        height: childrenRect.height
        Rectangle {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width - 2
            height: 1
            color: Theme.backgroundColor
        }
        Label {
            text: control.title
            color: Theme.textColor
            font {
                bold: true
                pointSize: Theme.titleFontSize
            }
            topPadding: Theme.windowMargin
            leftPadding: Theme.windowMargin
            bottomPadding: Theme.windowMargin
        }
    }

    footer: DialogButtonBox {
        CustomButton {
            text: qsTr("OK")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        CustomButton {
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: Theme.backgroundColor
            Rectangle {
                anchors {
                    top: parent.top
                    horizontalCenter: parent.horizontalCenter
                }
                width: parent.width - 2
                height: 1
                color: Theme.backgroundColor
            }
            border {
                width: 1
                color: Theme.tabButtonColor
            }
        }
    }
}
