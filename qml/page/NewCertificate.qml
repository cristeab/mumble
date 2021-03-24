import QtQuick 2.12
import QtQuick.Controls 2.12
import "../custom"
import ".."

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Label {
        id: pageTitle
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("New Certificate")
        color: Theme.textColor
        elide: Text.ElideRight
        font {
            bold: true
            pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        }
    }
    Label {
        id: pageSubTitle
        anchors {
            top: pageTitle.bottom
            topMargin: Theme.windowMargin / 2
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        text: qsTr("Generate a new certificate for strong authentication")
        color: Theme.textColor
        elide: Text.ElideRight
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }

    Rectangle {
        id: headerSep
        anchors {
            top: pageSubTitle.bottom
            topMargin: 1.5 * Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: Theme.windowMargin
        }
        height: 1
        color: Theme.separatorColor
    }

    Label {
        id: contentText
        anchors {
            top: headerSep.bottom
            topMargin: 1.5 * Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        wrapMode: Text.WordWrap
        text: qsTr("Bubbles will now generate a strong certificate for authentication to servers.\nIf you wish, you may provide some additional information to be stored in the certificate, which will be presented to servers when you connect. If you provide a valid email address, you can upgrade to a CA issued email certificate later on, which provides strong identification.")
        color: Theme.textColor
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }

    Column {
        anchors {
            top: contentText.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        LabelTextField {
            id: nameField
            text: qsTr("Name")
            onEditingFinished: certModel.newSubjectName = nameField.editText
        }
        LabelTextField {
            id: emailField
            text: qsTr("Email")
            validator: RegExpValidator { regExp:/\w+([-+.']\w+)*@\w+([-.]\w+)*\.\w+([-.]\w+)*/ }
            onEditingFinished: {
                if (emailField.acceptableInput) {
                    certModel.newSubjectEmail = emailField.editText
                }
            }
        }
    }
}
