import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    function getStandardButtons() {
        return msgDlg.okCancel ? (Dialog.Ok | Dialog.Cancel) :  Dialog.Ok
    }

    implicitWidth: 400
    implicitHeight: 200
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    z: 2
    onAccepted: {
        if (null !== msgDlg.acceptCallback) {
            msgDlg.acceptCallback()
            msgDlg.acceptCallback = null
        }
        msgDlg.text = ""
        msgDlg.okCancel = false
        msgDlg.active = false
    }
    onRejected: {
        msgDlg.text = ""
        msgDlg.okCancel = false
        msgDlg.active = false
    }
    visible: "" !== controlLabel.text
    title: msgDlg.title
    modal: true
    standardButtons: control.getStandardButtons()
    LabelToolTip {
        id: controlLabel
        text: msgDlg.text
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        clip: true
    }
}
