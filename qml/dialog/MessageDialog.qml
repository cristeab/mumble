import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
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
    closePolicy: Popup.NoAutoClose
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

    header: Rectangle {
        color: Theme.backgroundColor
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
            visible: msgDlg.okCancel
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: Theme.backgroundColor
        }
    }
}
