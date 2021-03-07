import QtQuick 2.12
import QtQuick.Controls 2.12
import "page"
import "custom"
import CertificateModel 1.0

Page {
    id: control

    background: Rectangle {
        color: Theme.backgroundColor
    }

    SwipeView {
        id: view

        readonly property var pageArr: ["qrc:/qml/page/CertificateAuth.qml", "qrc:/qml/page/NewCertificate.qml", "qrc:/qml/page/ReplaceCertificate.qml"]

        function getPage(idx) {
            if ((0 <= idx) && (idx < certModel.pageCount)) {
                if (0 === idx) {
                    return view.pageArr[idx]
                }
                if (1 === idx) {
                    return view.pageArr[idx]
                }
                if (2 === idx) {
                    return view.pageArr[idx]
                }
            }
            return ""
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
            onClicked: {
                if (CertificateModel.NEW_CERT_PAGE_COUNT === certModel.pageCount) {
                    if (0 === view.currentIndex) {
                        certModel.newSubjectName = ""
                        certModel.newSubjectEmail = ""
                    }
                    if (1 === view.currentIndex) {
                        if (("" === certModel.newSubjectName) || ("" === certModel.newSubjectEmail)) {
                            return
                        }
                    }
                }
                view.currentIndex += 1
            }
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
