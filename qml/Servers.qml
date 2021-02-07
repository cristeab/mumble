import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control
    CustomTableView {
        id: srvTbl
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            bottom: connectBtn.top
            bottomMargin: Theme.windowMargin
        }
    }
    CustomButton {
        id: connectBtn
        anchors {
            left: srvTbl.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        backgroundColor: Theme.buttonBlueColor
        text: qsTr("Connect")
        onClicked: {
            //TODO
        }
    }
    Row {
        anchors {
            right: srvTbl.right
            bottom: connectBtn.bottom
        }
        spacing: Theme.windowMargin
        CustomButton {
            backgroundColor: "lightgray"
            textColor: "black"
            text: qsTr("Add New...")
            onClicked: {
                //TODO
            }
        }
        CustomButton {
            backgroundColor: "lightgray"
            textColor: "black"
            text: qsTr("Edit...")
            onClicked: {
                //TODO
            }
        }
    }
}
