import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Component.onCompleted: srvTbl.forceLayout()

    CustomTabButton {
        id: forwardBtn
        anchors {
            top: parent.top
            right: parent.right
        }
        visible: (0 <= servers.connectedServerIndex) && (0 < servers.classNameList.length)
        text: qsTr("Forward")
        icon {
            width: Theme.buttonIconWidth
            height: Theme.buttonIconWidth
            source: "qrc:/img/chevron-circle-right.svg"
        }
        onClicked: tabView.push("qrc:/qml/Schools.qml")
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

        property bool isConnected: servers.currentIndex === servers.connectedServerIndex

        enabled: servers.isReachable(servers.currentIndex)
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
                servers.connectServer()
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
        CustomButton {
            text: qsTr("Remove")
            onClicked: {
                msgDlg.showDialog(qsTr("Delete Server"),
                                  qsTr("Are you sure you want to delete ") + servers.currentServerName() + " ?",
                                  true,
                                  servers.removeServer)
            }
        }
    }
}
