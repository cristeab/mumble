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
    standardButtons: Dialog.Ok | Dialog.Cancel
    implicitWidth: 400
    implicitHeight: dlgColumn.height
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2

    background: Rectangle {
        color: Theme.backgroundColor
    }
    onAccepted: {
        servers.onLineEditDlgAccepted()
        control.cleanup()
    }
    onRejected: control.cleanup()
    Component.onCompleted: control.visible = true

    contentItem: Column {
        id: dlgColumn
        Label {
            width: parent.width
            text: servers.dlgTextLabel
            maximumLineCount: 5
            clip: true
            elide: Text.ElideRight
        }
        CustomTextField {
            width: parent.width
            echoMode: servers.dlgIsPassword ? TextInput.Password : TextInput.Normal
            onTextChanged: servers.dlgText = text
        }
    }
}
