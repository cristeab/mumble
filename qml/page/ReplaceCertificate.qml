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
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
        }
        text: qsTr("Replace Certificate")
        color: Theme.textColor
        elide: Text.ElideRight
        font.bold: true
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
        text: qsTr("Replace existing certificate with new certificate?")
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
        text: qsTr("You already have a certificate stored in Bubbles, and you are about to replace it.\nIf you are upgrading to a certificate issued to you by a trusted CA and the email addresses match your current certificate, this is completely safe, and servers you connect to will automatically recognize the strong certificate for your email address.\nIf this is not the case, you will no longer be recognized by any server you previously have authenticated with. If you haven't been registered on any server yet, this is nothing to worry about.\nAre you sure you wish to replace your certificate?")
        color: Theme.textColor
    }

    Row {
        id: certRow
        anchors {
            top: contentText.bottom
            topMargin: Theme.windowMargin
            left: parent.left
            leftMargin: 3 * Theme.windowMargin
            right: parent.right
            rightMargin: 3 * Theme.windowMargin
        }
        spacing: Theme.windowMargin / 2
        GroupBox {
            id: currentCert

            readonly property var nameArr: [qsTr("Name"), qsTr("Email"), qsTr("Issuer"), qsTr("Expiry Date")]
            readonly property var valueArr: [certModel.subjectName, certModel.subjectEmail, certModel.issuerName, certModel.expiry]

            width: (parent.width - certRow.spacing) / 2
            label: Label {
                width: 0.8 * currentCert.width
                text: qsTr("Current Certificate")
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
                            width: 0.2 * currentCert.width
                            text: currentCert.nameArr[index]
                            color: Theme.textColor
                            elide: Text.ElideRight
                        }
                        Label {
                            width: 0.8 * currentCert.width
                            text: currentCert.valueArr[index]
                            color: Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
        GroupBox {
            id: newCert
            readonly property var valueArr: [certModel.newSubjectName, certModel.newSubjectEmail, certModel.newIssuerName, certModel.newExpiry]

            width: currentCert.width
            label: Label {
                width: 0.8 * currentCert.width
                text: qsTr("New Certificate")
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
                            width: 0.2 * newCert.width
                            text: currentCert.nameArr[index]
                            color: Theme.textColor
                            elide: Text.ElideRight
                        }
                        Label {
                            width: 0.8 * newCert.width
                            text: newCert.valueArr[index]
                            color: Theme.textColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
