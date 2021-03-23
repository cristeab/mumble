import QtQuick 2.12
import QtQuick.Controls 2.12
import CertificateModel 1.0
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
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("Import Certificate")
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
        text: qsTr("PKCS #12 Certificate Import")
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
        text: qsTr("Bubbles can import certificates stored in PKCS #12 format. This is the format used when exporting a key from Bubbles, and also when exporting keys from Firefox, Internet Explorer, Opera etc.\nIf the file is password protected, you will need the password to import the certificate.")
        color: Theme.textColor
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }

    Row {
        id: importRow
        anchors {
            top: contentText.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        spacing: Theme.windowMargin
        LabelTextField {
            text: qsTr("Import from")
            width: parent.width - openBtn.width - importRow.spacing
            editText: certModel.certFilePath
            onEditingFinished: certModel.certFilePath = editText
        }
        CustomButton {
            id: openBtn
            text: qsTr("Open...")
            onClicked: certDlg.showImportDialog()
        }
    }
    LabelTextField {
        id: pwdField
        anchors {
            top: importRow.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        visible: certModel.requestPassword
        echoMode: TextInput.PasswordEchoOnEdit
        text: qsTr("Password")
        width: parent.width
        editText: certModel.certPassword
        onEditingFinished: certModel.certPassword = editText
        onEditTextChanged: certModel.requestPassword = !certModel.importCert(true)
    }

    GroupBox {
        id: currentCert

        readonly property var nameArr: [qsTr("Name"), qsTr("Email"), qsTr("Issuer"), qsTr("Expiry Date")]
        readonly property var valueArr: [certModel.subjectName, certModel.subjectEmail, certModel.issuerName, certModel.expiry]

        anchors {
            top: pwdField.visible ? pwdField.bottom : importRow.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        label: Label {
            width: 0.8 * currentCert.width
            text: qsTr("Current Certificate")
            color: Theme.textColor
            elide: Text.ElideRight
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        }
        Column {
            spacing: Theme.windowMargin
            Repeater {
                model: currentCert.nameArr.length
                Row {
                    spacing: Theme.windowMargin
                    Label {
                        width: 0.1 * currentCert.width
                        text: currentCert.nameArr[index]
                        color: Theme.textColor
                        elide: Text.ElideRight
                        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
                    }
                    Label {
                        width: 0.9 * currentCert.width
                        text: currentCert.valueArr[index]
                        color: Theme.textColor
                        elide: Text.ElideRight
                        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
                    }
                }
            }
        }
    }
}
