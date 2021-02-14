import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

ApplicationWindow {
    id: appWin

    title: qsTr("Bubbles")
    width: 800
    height: 600
    visible: true

    background: Rectangle {
        color: Theme.backgroundColor
    }

    Column {
        id: bar

        property int currentButtonIndex: 0
        readonly property var names: [qsTr("Servers"), qsTr("Audio Input"), qsTr("Audio Output"), qsTr("Password Manager"), qsTr("Certificate Manager")]
        readonly property var icons: ["qrc:/img/server.svg", "qrc:/img/microphone.svg", "qrc:/img/volume.svg", "qrc:/img/key.svg", "qrc:/img/certificate.svg"]
        readonly property var pages: ["qrc:/qml/Servers.qml", "qrc:/qml/AudioInput.qml", "qrc:/qml/AudioOutput.qml", "qrc:/qml/PasswordManager.qml", "qrc:/qml/CertificateManager.qml"]

        Component.onCompleted: {
            if (0 === bar.currentButtonIndex) {
                servers.startPingTick(true)
            }
        }

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        width: 60
        spacing: 0

        Repeater {
            model: bar.names.length
            TabButton {
                id: tabButton
                property bool isSelected: bar.currentButtonIndex === index
                property color textColor: isSelected ? Theme.tabButtonColorSel : Theme.tabButtonColor
                display: AbstractButton.IconOnly
                text: "<font color='" + tabButton.textColor + "'>" + bar.names[index] + "</font>"
                icon {
                    source: bar.icons[index]
                    color: tabButton.textColor
                }
                font.pointSize: 5
                width: bar.width
                height: width + 2 * Theme.windowMargin
                background: Rectangle {
                    color: Theme.backgroundColor
                }
                onClicked: {
                    bar.currentButtonIndex = index
                    tabView.replace(bar.pages[index])
                    servers.startPingTick(0 === index)
                }
                ToolTip {
                    visible: tabButton.hovered
                    text: bar.names[index]
                }
            }
        }
    }
    StackView {
        id: tabView

        anchors {
            top: parent.top
            bottom: parent.bottom
            left: bar.right
            right: parent.right
        }
        width: parent.width
        initialItem: "qrc:/qml/Servers.qml"
    }

    Loader {
        id: addEditServerDlg

        function addNewServer() {
            servers.currentIndex = -1
            servers.resetServer()
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
        }
        function editServer(idx) {
            servers.currentIndex = idx
            servers.resetServer()
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
        }

        anchors.fill: parent
        active: false
        source: "qrc:/qml/dialog/AddEditServer.qml"
    }
}