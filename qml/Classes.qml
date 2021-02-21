import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    ListView {
        id: classList

        Label {
            id: classListHeader
            width: parent.width
            text: qsTr("Class name")
            color: Theme.textColor2
            font.pixelSize: 15
            padding: Theme.windowMargin
            verticalAlignment: Text.AlignVCenter
            background: Rectangle { color: Theme.tableBackgroundColor }
        }
        topMargin: classListHeader.implicitHeight + 3

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        anchors {
            top: parent.top
            topMargin: 8 * Theme.windowMargin
            left: parent.left
            leftMargin: 2 * Theme.windowMargin
            right: parent.right
            rightMargin: 2 * Theme.windowMargin
            bottom: goBtn.top
            bottomMargin: Theme.windowMargin
        }
        currentIndex: 0
        clip: true
        boundsBehavior: ListView.StopAtBounds
        model: servers.classNameList
        delegate: Label {
            width: parent.width
            padding: Theme.windowMargin
            text: modelData
            color: Theme.textColor2
            clip: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            background: Rectangle {
                color: (index !== classList.currentIndex) ? Theme.tableBackgroundColor : Theme.tableSelectedBackgroundColor
            }
            MouseArea {
                anchors.fill: parent
                onClicked: classList.currentIndex = index
                onDoubleClicked: {
                    classList.currentIndex = index
                    goBtn.gotoAction()
                }
            }
        }
    }

    CustomButton {
        id: goBtn

        function gotoAction() {
            servers.gotoClass(classList.currentIndex)
            tabView.push("qrc:/qml/Rooms.qml")
        }

        anchors {
            left: classList.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        text: qsTr("Go to class")
        onClicked: goBtn.gotoAction()
    }
}
