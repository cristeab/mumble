import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    function validate() {
        if ("" === servers.hostname) {
            address.error = true
            return false
        }
        if (0 === servers.port) {
            port.error = true
            return false
        }

        if ("" === servers.username) {
            username.error = true
            return false
        }
        if ("" === servers.label) {
            label.error = true
            return false
        }
        return true
    }

    background: Rectangle {
        color: Theme.backgroundColor
    }
    implicitWidth: 400
    implicitHeight: dlgColumn.height + 120
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    title: (-1 === servers.currentIndex) ? qsTr("Add Server") : qsTr("Edit Server")
    modal: true
    visible: false
    closePolicy: Popup.NoAutoClose

    onAccepted: {
        if (!control.validate()) {
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
            return
        }
        servers.changeServer()
        addEditServerDlg.active = false
    }
    onRejected: addEditServerDlg.active = false

    Column {
        id: dlgColumn
        width: parent.width
        spacing: Theme.windowMargin

        LabelTextField {
            id: address
            text: qsTr("Address")
            editText: servers.hostname
            placeholderText: "127.0.0.1"
            onEditTextChanged: {
                servers.hostname = editText
                servers.label = editText
            }
        }
        LabelTextField {
            id: port
            text: qsTr("Port")
            editText: servers.port
            placeholderText: "64738"
            validator: IntValidator {
                bottom: 0
                top: 65535
            }
            onEditTextChanged: servers.port = parseInt(editText)
        }
        LabelTextField {
            id: username
            text: qsTr("Username")
            editText: servers.username
            placeholderText: qsTr("Your username")
            onEditTextChanged: servers.username = editText
        }
        LabelTextField {
            id: label
            text: qsTr("Label")
            editText: servers.label
            placeholderText: qsTr("Local server label")
            onEditTextChanged: servers.label = editText
        }
    }

    header: Rectangle {
        color: Theme.backgroundColor
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
        }
    }
}
