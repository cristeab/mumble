import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    property int serverIndex: -1

    implicitWidth: 300
    implicitHeight: 400
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    standardButtons: Dialog.Ok | Dialog.Cancel
    title: (-1 === control.serverIndex) ? qsTr("Add Server") : qsTr("Edit Server")
    modal: true
    visible: false

    function validate() {
        if ("" === address.editText) {
            address.error = true
            return false
        }
        if ("" === username.editText) {
            username.error = true
            return false
        }
        if ("" === label.editText) {
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
        addEditServerDlg.active = false
    }
    onRejected: addEditServerDlg.active = false

    Column {
        id: dlgColumn
        width: parent.width
        spacing: Theme.windowMargin

        Component.onCompleted: console.log("height " + height)

        LabelTextField {
            id: address
            text: qsTr("Address")
            editText: servers.address
            placeholderText: "127.0.0.1"
        }
        LabelTextField {
            id: port
            text: qsTr("Port")
            editText: servers.port
            placeholderText: "64738"
            validator: IntValidator{bottom: 0; top: 65535;}
        }
        LabelTextField {
            id: username
            text: qsTr("Username")
            editText: servers.username
            placeholderText: qsTr("Your username")
        }
        LabelTextField {
            id: label
            text: qsTr("Label")
            editText: servers.label
            placeholderText: qsTr("Local server label")
        }
    }
}
