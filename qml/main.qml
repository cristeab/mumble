import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.2
import "custom"

ApplicationWindow {
    id: appWin

    readonly property bool isFullScreen: appWin.visibility === ApplicationWindow.FullScreen
    readonly property bool isLandscape: Qt.LandscapeOrientation === Screen.primaryOrientation
    readonly property bool isBig: isFullScreen && isLandscape

    title: qsTr("Bubbles")
    width: 800
    height: 600
    visible: true

    Component.onCompleted: {
        appWin.showMaximized()
        appWin.raise()
    }

    background: Rectangle {
        color: Theme.backgroundColor
    }

    CustomTabButton {
        id: backBtn
        anchors {
            top: parent.top
            left: parent.left
        }
        visible: (1 < tabView.depth) && (0 === bar.currentButtonIndex)
        text: qsTr("Back")
        icon {
            width: Theme.buttonIconWidth
            height: Theme.buttonIconWidth
            source: "qrc:/img/chevron-circle-left.svg"
        }
        onClicked: tabView.pop()
    }

    Column {
        id: bar

        enabled: true

        property int currentButtonIndex: 0
        readonly property var names: [qsTr("Servers"), qsTr("Audio Input"), qsTr("Audio Output"), qsTr("Password Manager"), qsTr("Certificate Manager")]
        readonly property var icons: ["qrc:/img/server.svg", "qrc:/img/microphone.svg", "qrc:/img/volume.svg", "qrc:/img/key.svg", "qrc:/img/certificate.svg"]
        readonly property var pages: ["qrc:/qml/Servers.qml", "qrc:/qml/AudioInput.qml", "qrc:/qml/AudioOutput.qml", "qrc:/qml/PasswordManager.qml", "qrc:/qml/CertificateManager.qml"]

        readonly property var serversPages: ["qrc:/qml/Servers.qml", "qrc:/qml/Schools.qml", "qrc:/qml/Classes.qml", "qrc:/qml/Rooms.qml"]

        Component.onCompleted: {
            if (0 === bar.currentButtonIndex) {
                servers.startPingTick(true)
            }
        }

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        width: 0.075 * appWin.width
        spacing: Theme.windowMargin

        Repeater {
            model: bar.names.length
            CustomTabButton {
                id: tabButton
                property bool isSelected: bar.currentButtonIndex === index
                property color textColor: isSelected ? Theme.tabButtonColorSel : Theme.tabButtonColor
                icon {
                    source: bar.icons[index]
                    color: tabButton.textColor
                    width: ((1 === index) ? 0.8 : 1.0) * Theme.tabIconWidth
                    height: Theme.tabIconWidth
                }
                onClicked: {
                    bar.currentButtonIndex = index
                    if ((0 === index) && (1 < tabView.depth)) {
                        console.log("Restoring servers tab " + tabView.depth)
                        tabView.replace(bar.serversPages[tabView.depth - 1])
                    } else {
                        tabView.replace(bar.pages[index])
                    }
                    servers.startPingTick(0 === index)
                    if ((1 === index) || (2 === index)) {
                        audioDevices.init(1 === index)
                    }
                    if (3 === index) {
                        if (0 <= servers.connectedServerIndex) {
                            tokensModel.load()
                        }
                    }
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
        initialItem: "qrc:/qml/Servers.qml"
    }
    Connections {
        target: servers
        function onSchoolsAvailable() {
            if (1 === tabView.depth) {
                tabView.push("qrc:/qml/Schools.qml")
            }
        }
        function onDlgTitleChanged() {
            if ("" !== servers.dlgTitle) {
                lineEditDlg.showDlg()
            }
        }
        function onChannelAllowedChanged(allowed) {
            const idx = tabView.depth - 1
            if (1 === idx) {
                if (allowed && servers.gotoSchoolInternal()) {
                    tabView.push("qrc:/qml/Classes.qml")
                } else {
                    msgDlg.showDialog(qsTr("Error"), qsTr("You were denied access to this school"))
                }
            } else if (2 === idx) {
                if (allowed && servers.gotoClassInternal()) {
                    tabView.push("qrc:/qml/Rooms.qml")
                    console.log("server: cur " + servers.currentIndex + ", conn " + servers.connectedServerIndex)
                    console.log("class: cur " + servers.currentClassIndex + ", conn " + servers.connectedClassIndex)
                } else {
                    msgDlg.showDialog(qsTr("Error"), qsTr("You were denied access to this class"))
                }
            } else if (3 === idx) {
                if (!allowed || !servers.joinRoomInternal()) {
                    msgDlg.showDialog(qsTr("Error"), qsTr("You were denied access to this room"))
                }
            }
        }
    }

    Loader {
        id: addEditServerDlg

        function addNewServer() {
            servers.currentIndex = -1
            servers.resetServer()
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
        }
        function editServer() {
            servers.resetServer()
            addEditServerDlg.active = true
            addEditServerDlg.item.visible = true
        }

        anchors.fill: parent
        active: false
        source: "qrc:/qml/dialog/AddEditServer.qml"
    }

    Loader {
        id: lineEditDlg

        function showDlg() {
            lineEditDlg.active = true
            lineEditDlg.item.visible = true
        }

        anchors.fill: parent
        active: false
        source: "qrc:/qml/dialog/LineEditDialog.qml"
    }

    Loader {
        id: addEditTokenDlg

        function addToken() {
            tokensModel.currentIndex = -1
            addEditTokenDlg.active = true
            addEditTokenDlg.item.visible = true
        }
        function editToken(index) {
            tokensModel.currentIndex = index
            addEditTokenDlg.active = true
            addEditTokenDlg.item.visible = true
        }

        anchors.fill: parent
        active: false
        source: "qrc:/qml/dialog/AddEditToken.qml"
    }

    Loader {
        id: certDlg

        property bool selectExisting: false

        function showExportDialog() {
            certDlg.selectExisting = false
            certDlg.active = true
        }
        function showImportDialog() {
            certDlg.selectExisting = true
            certDlg.active = true
        }

        active: false
        anchors.centerIn: parent
        sourceComponent: FileDialog {
            title: certDlg.selectExisting ? qsTr("Select file to import certificate from") : qsTr("Select file to export certificate to")
            folder: shortcuts.home
            Component.onCompleted: visible = true
            selectExisting: certDlg.selectExisting
            selectFolder: false
            selectMultiple: false
            defaultSuffix: "p12"
            nameFilters: [ "PKCS12 (*.p12 *.pfx *.pkcs12)", "All files (*)" ]
            onAccepted: {
                certModel.toLocalFile(fileUrl)
                certDlg.active = false
            }
            onRejected: certDlg.active = false
        }
    }

    Connections {
        target: certModel
        function onShowDialog(title, text) {
            msgDlg.showDialog(title, text)
        }
    }
    QtObject {
        id: msgDlg

        function showDialog(title, text, okCalcel=false, callback=null) {
            const comp = Qt.createComponent("qrc:/qml/dialog/MessageDialog.qml")
            const dlg = comp.createObject(appWin, {
                                              "title": title,
                                              "text": text,
                                              "okCancel": okCalcel,
                                              "acceptCallback": callback,
                                              "visible": true
                                          })
            if (null === dlg) {
                console.error("Cannot create message dialog")
            }
        }
    }
}
