import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.2
import "custom"

ApplicationWindow {
    id: appWin

    title: qsTr("Bubbles")
    width: 800
    height: 600
    visible: true

    background: Rectangle {
        color: Theme.backgroundColor
    }

    CustomTabButton {
        id: backBtn
        anchors {
            top: parent.top
            left: parent.left
        }
        visible: 1 < tabView.depth
        text: qsTr("Back")
        icon.source: "qrc:/img/chevron-circle-left.svg"
        width: bar.width
        height: width + Theme.windowMargin
        onClicked: tabView.pop()
    }

    Column {
        id: bar

        enabled: true

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
            CustomTabButton {
                id: tabButton
                property bool isSelected: bar.currentButtonIndex === index
                property color textColor: isSelected ? Theme.tabButtonColorSel : Theme.tabButtonColor
                //text: bar.names[index]
                icon {
                    source: bar.icons[index]
                    color: tabButton.textColor
                }
                width: bar.width
                height: width + 2 * Theme.windowMargin
                onClicked: {
                    bar.currentButtonIndex = index
                    tabView.replace(bar.pages[index])
                    servers.startPingTick(0 === index)
                    if ((1 === index) || (2 === index)) {
                        audioDevices.init(1 === index)
                    }
                    if (3 === index) {
                        tokensModel.load()
                    } else {
                        tokensModel.save()
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
        width: parent.width
        initialItem: "qrc:/qml/Servers.qml"
    }
    Connections {
        target: servers
        function onClassesAvailable() {
            if (1 === tabView.depth) {
                tabView.push("qrc:/qml/Classes.qml")
            }
        }
        function onDlgTitleChanged() {
            if ("" !== servers.dlgTitle) {
                lineEditDlg.showDlg()
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
        function onShowErrorDialog(msg) {
            msgDlg.title = qsTr("Error")
            msgDlg.text = msg
            msgDlg.okCancel = false
            msgDlg.showDlg()
        }
    }
    Loader {
        id: msgDlg

        property string title: ""
        property string text: ""
        property bool okCancel: true
        property var acceptCallback: null

        function showDlg() {
            msgDlg.active = true
            msgDlg.item.visible = true
        }

        anchors.fill: parent
        active: false
        source: "qrc:/qml/dialog/MessageDialog.qml"
    }
}
