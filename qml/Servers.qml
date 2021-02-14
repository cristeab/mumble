import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    CustomTableView {
        id: srvTbl
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: connectBtn.top
            bottomMargin: Theme.windowMargin
        }
    }
    CustomButton {
        id: connectBtn
        property bool isConnected: servers.isConnected(servers.currentIndex)
        anchors {
            left: srvTbl.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        text: connectBtn.isConnected ? qsTr("Disconnect") : qsTr("Connect")
        onClicked: {
            if (connectBtn.isConnected) {
                servers.disconnectServer()
            } else {
                servers.connect()
            }
        }
    }
    Row {
        anchors {
            right: srvTbl.right
            bottom: connectBtn.bottom
        }
        spacing: Theme.windowMargin
        CustomButton {
            text: qsTr("Add New...")
            onClicked: addEditServerDlg.addNewServer()
        }
        CustomButton {
            enabled: !connectBtn.isConnected
            text: qsTr("Edit...")
            onClicked: addEditServerDlg.editServer()
        }
    }
}
