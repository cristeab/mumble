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
        text: qsTr("Export Certificate")
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
        text: qsTr("Make a backup of your certificate")
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
        text: qsTr("If you ever lose your current certificate, which will happen if your computer suffers a hardware failure or you reinstall your machine, you will no longer be able to authenticate to any server you are registered on. It is therefore <b>mandatory</b> that you make a backup of your certificate. We strongly recommend you store this backup on removable storage, such as a USB flash drive.<br>Note that this file will not be encrypted, and if anyone gains access to it, they will be able to impersonate you, so take good care of it.")
        color: Theme.textColor
    }

    /*Row {
        id: exportRow
        spacing: Theme.windowMargin / 2
        LabelTextField {
            text: qsTr("Export to")
        }
        CustomButton {
            text: qsTr("Save As...")
        }
    }

    GroupBox {
        id: currentCert

        readonly property var nameArr: [qsTr("Name"), qsTr("Email"), qsTr("Issuer"), qsTr("Expiry Date")]
        readonly property var valueArr: [certModel.newSubjectName, certModel.newSubjectEmail, certModel.newIssuerName, certModel.newExpiry]

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
            text: qsTr("Certificate Details")
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
                    }
                    Label {
                        width: 0.9 * currentCert.width
                        text: currentCert.valueArr[index]
                        color: Theme.textColor
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }*/
}
