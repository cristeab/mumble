import QtQuick 2.12
import QtQuick.Controls 2.12
import "page"

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
        anchors.fill: parent

        Repeater {
            model: certModel.pageCount
            Loader {
                active: SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
                source: view.getPage(index)
            }
        }
    }
    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: view.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
