import QtQuick 2.12
import QtQuick.Controls 2.12
import "custom"

Page {
    id: control

    Component.onCompleted: schoolList.currentIndex = 0

    background: Rectangle {
        color: Theme.backgroundColor
    }

    CustomTabButton {
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
        onClicked: tabView.push("qrc:/qml/Classes.qml")
    }

    ListView {
        id: schoolList

        Label {
            id: schoolListHeader
            width: parent.width
            text: qsTr("School name")
            color: Theme.textColor2
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
            padding: Theme.windowMargin
            verticalAlignment: Text.AlignVCenter
            background: Rectangle { color: Theme.tableBackgroundColor }
        }
        topMargin: schoolListHeader.implicitHeight + 3

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
        model: servers.schoolNameList
        delegate: Label {
            width: parent.width
            padding: Theme.windowMargin
            text: modelData
            font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
            color: Theme.textColor2
            clip: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            background: Rectangle {
                color: (index !== schoolList.currentIndex) ? Theme.tableBackgroundColor : Theme.tableSelectedBackgroundColor
            }
            MouseArea {
                anchors.fill: parent
                onClicked: schoolList.currentIndex = index
                onDoubleClicked: {
                    schoolList.currentIndex = index
                    goBtn.gotoAction()
                }
            }
        }
    }

    CustomButton {
        id: goBtn

        function gotoAction() {
            servers.gotoSchool(schoolList.currentIndex)
        }

        anchors {
            left: schoolList.left
            bottom: parent.bottom
            bottomMargin: Theme.windowMargin
        }
        text: qsTr("Go to school")
        onClicked: goBtn.gotoAction()
    }
}
