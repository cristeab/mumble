import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Dialog {
    id: control

    title: ""
    property string text: ""
    property bool okCancel: true
    property var acceptCallback: null

    Connections {
        target: servers
        function onCloseDialog() {
            control.destroy()
        }
    }

    background: Rectangle {
        color: Theme.backgroundColor
    }
    implicitWidth: 400
    implicitHeight: 200
    x: (appWin.width-width)/2
    y: (appWin.height-height)/2
    z: 2
    onAccepted: {
        if (null !== control.acceptCallback) {
            control.acceptCallback()
        }
        control.destroy()
    }
    onRejected: control.destroy()
    visible: "" !== controlLabel.text
    modal: true
    closePolicy: Popup.NoAutoClose
    LabelToolTip {
        id: controlLabel
        text: control.text
        anchors.fill: parent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        clip: true
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
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
            visible: control.okCancel
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        background: Rectangle {
            color: Theme.backgroundColor
        }
    }
}
