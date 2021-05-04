import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    Component.onCompleted: srvTbl.forceLayout()

    background: CustomBackground {}

    CustomTableView {
        id: srvTbl
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: connectBtnRow.top
            bottomMargin: Theme.windowMargin
        }
    }
    Row {
        id: connectBtnRow

        property bool isConnected: servers.currentIndex === servers.connectedServerIndex

        anchors {
            left: srvTbl.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        spacing: 2 * Theme.windowMargin
        CustomButton {
            anchors.verticalCenter: fwBtn.verticalCenter
            enabled: servers.isReachable(servers.currentIndex)
            text: connectBtnRow.isConnected ? qsTr("Disconnect") : qsTr("Connect")
            onClicked: {
                if (connectBtnRow.isConnected) {
                    servers.disconnectServer()
                } else {
                    servers.connectServer()
                }
            }
        }
        CustomForwardButton {
            id: fwBtn
            visible: (0 <= servers.connectedServerIndex) && (0 < servers.schoolNameList.length)
            onClicked: tabView.push("qrc:/qml/Schools.qml")
        }
    }
    Row {
        anchors {
            right: srvTbl.right
            bottom: connectBtnRow.bottom
        }
        spacing: Theme.windowMargin
        CustomButton {
            text: qsTr("Add New...")
            onClicked: addEditServerDlg.addNewServer()
        }
        CustomButton {
            enabled: !connectBtnRow.isConnected
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
