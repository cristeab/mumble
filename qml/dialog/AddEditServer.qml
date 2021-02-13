import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    property int serverIndex: -1

    implicitWidth: 400
    implicitHeight: dlgColumn.height + 120
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: (-1 === control.serverIndex) ? qsTr("Add Server") : qsTr("Edit Server")
    modal: true
    visible: false
    closePolicy: Popup.NoAutoClose

    function validate() {
        if ("" === servers.address) {
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

    onAccepted: {
        if (!control.validate()) {
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
            return
        }
        servers.changeServer(control.serverIndex)
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
            editText: servers.address
            placeholderText: "127.0.0.1"
            validator: RegExpValidator {
                regExp:  /^((?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\.){0,3}(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$/
            }
            onEditTextChanged: {
                servers.address = editText
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
}
