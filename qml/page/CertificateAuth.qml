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
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("Certificate Authentication")
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
        text: qsTr("Authenticating to servers without using passwords")
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        color: Theme.textColor
        elide: Text.ElideRight
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
        text: qsTr("Bubbles can use certificates to authenticate with servers. Using certificates avoids passwords, meaning you don't need to disclose any password to the remote site. It also enables very easy user registration and a client side friends list independent of servers.\nWhile Bubbles can work without certificates, the majority of servers will expect you to have one.\nCreating a new certificate automatically is sufficient for most use cases. But Bubbles also supports certificates representing trust in the users ownership of an email address. These certificates are issued by third parties.")
        color: Theme.textColor
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }
    GroupBox {
        id: currentCert

        readonly property var nameArr: [qsTr("Name"), qsTr("Email"), qsTr("Issuer"), qsTr("Expiry Date")]
        readonly property var valueArr: [certModel.subjectName, certModel.subjectEmail, certModel.issuerName, certModel.expiry]

        anchors {
            top: contentText.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        label: Label {
            width: 0.8 * currentCert.width
            text: qsTr("Current Certificate")
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
            color: Theme.textColor
            elide: Text.ElideRight
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

    GroupBox {
        id: autoCert

        readonly property var nameArr: [qsTr("Create a new certificate"), qsTr("Import a certificate"), qsTr("Export current certificate")]

        anchors {
            top: currentCert.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        label: Label {
            width: 0.8 * currentCert.width
            text: qsTr("Automatic Certificate Creation")
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
            color: Theme.textColor
            elide: Text.ElideRight
        }
        ButtonGroup {
            exclusive: true
            buttons: autoCertCol.children
            onClicked: console.log("clicked:", button.text)
        }
        Column {
            id: autoCertCol
            spacing: 0
            Repeater {
                model: autoCert.nameArr.length
                CustomRadioButton {
                    checked: ((CertificateModel.NEW_CERT_PAGE_COUNT === certModel.pageCount) && (0 === index)) || ((CertificateModel.IMPORT_CERT_PAGE_COUNT === certModel.pageCount) && (1 === index)) || ((CertificateModel.EXPORT_CERT_PAGE_COUNT === certModel.pageCount) && (2 === index))
                    text: autoCert.nameArr[index]
                    onCheckedChanged: {
                        if (checked) {
                            if (0 === index) {
                                certModel.pageCount = CertificateModel.NEW_CERT_PAGE_COUNT
                            } else if (1 === index) {
                                certModel.certFilePath = ""
                                certModel.requestPassword = false
                                certModel.certPassword = ""
                                certModel.pageCount = CertificateModel.IMPORT_CERT_PAGE_COUNT
                            } else if (2 === index) {
                                certModel.certFilePath = ""
                                certModel.pageCount = CertificateModel.EXPORT_CERT_PAGE_COUNT
                            }
                        }
                    }
                }
            }
        }
    }
}
