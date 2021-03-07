import QtQuick 2.12
import QtQuick.Controls 2.12
import "page"
import "custom"

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    SwipeView {
        id: view

        readonly property var pageArr: ["qrc:/qml/page/CertificateAuth.qml"]

        function getPage(idx) {
            if ((0 <= idx) && (idx < certModel.pageCount)) {
                return view.pageArr[0]
            }
            return null
        }

        currentIndex: 0
        clip: true
        anchors {
            top: parent.top
            left: parent.left
            bottom: controlBtn.top
        }
        width: parent.width

        Repeater {
            model: certModel.pageCount
            Loader {
                active: SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
                source: view.getPage(index)
            }
        }
    }

    Row {
        id: controlBtn
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: indicator.top
        }
        spacing: 2 * Theme.windowMargin
        CustomTabButton {
            enabled: 0 < view.currentIndex
            text: qsTr("Back")
            icon.source: "qrc:/img/chevron-circle-left.svg"
            width: backBtn.width
            height: backBtn.height
            onClicked: view.currentIndex -= 1
        }
        CustomTabButton {
            enabled: view.currentIndex < (certModel.pageCount - 1)
            text: qsTr("Next")
            icon.source: "qrc:/img/chevron-circle-right.svg"
            width: backBtn.width
            height: backBtn.height
            onClicked: view.currentIndex += 1
        }
    }

    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
