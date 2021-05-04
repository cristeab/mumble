import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    function cleanup() {
        lineEditDlg.active = false
        servers.dlgTitle = ""
        servers.dlgTextLabel = ""
        servers.dlgText = ""
        servers.dlgIsPassword = false
    }

    title: servers.dlgTitle
    implicitWidth: 400
    implicitHeight: dlgColumn.height + 120
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2

    background: CustomDialogBackground {}
    onAccepted: {
        servers.onLineEditDlgAccepted()
        control.cleanup()
    }
    onRejected: {
        servers.connectedServerIndex = -1
        control.cleanup()
    }
    Component.onCompleted: control.visible = true
    modal: true
    closePolicy: Popup.CloseOnEscape

    Column {
        id: dlgColumn
        anchors.topMargin: 2 * Theme.windowMargin
        width: parent.width
        spacing: Theme.windowMargin
        Label {
            width: parent.width
            text: servers.dlgTextLabel
            maximumLineCount: 5
            wrapMode: Text.WordWrap
            clip: true
            elide: Text.ElideRight
            color: Theme.textColor
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        }
        CustomTextField {
            width: parent.width
            echoMode: servers.dlgIsPassword ? TextInput.Password : TextInput.Normal
            text: servers.dlgText
            onTextChanged: servers.dlgText = text
            color: Theme.textColor
            placeholderText: servers.dlgIsPassword ? qsTr("Your password") : qsTr("Your username")
        }
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
                pointSize: appWin.isBig ? Theme.bigTitleFontSize : Theme.titleFontSize
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
