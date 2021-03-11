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

    Component.onCompleted: view.currentIndex = 0

    SwipeView {
        id: view

        readonly property var pageArr0: ["qrc:/qml/page/CertificateAuth.qml", "qrc:/qml/page/NewCertificate.qml", "qrc:/qml/page/ReplaceCertificate.qml", "qrc:/qml/page/ExportCertificate.qml", "qrc:/qml/page/FinishCertificate.qml"]
        readonly property var pageArr1: ["qrc:/qml/page/CertificateAuth.qml", "qrc:/qml/page/ImportCertificate.qml", "qrc:/qml/page/ReplaceCertificate.qml", "qrc:/qml/page/FinishCertificate.qml"]
        readonly property var pageArr2: ["qrc:/qml/page/CertificateAuth.qml", "qrc:/qml/page/ExportCertificate.qml"]

        function getPage(idx) {
            if (CertificateModel.NEW_CERT_PAGE_COUNT === certModel.pageCount) {
                return view.pageArr0[idx]
            }
            if (CertificateModel.IMPORT_CERT_PAGE_COUNT === certModel.pageCount) {
                return view.pageArr1[idx]
            }
            if (CertificateModel.EXPORT_CERT_PAGE_COUNT === certModel.pageCount) {
                return view.pageArr2[idx]
            }
            return ""
        }

        currentIndex: 0
        clip: true
        interactive: true
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
            enabled: (0 < view.currentIndex) && nextBtn.enabled
            text: qsTr("Back")
            icon.source: "qrc:/img/chevron-circle-left.svg"
            width: backBtn.width
            height: backBtn.height
            onClicked: view.currentIndex -= 1
        }
        CustomTabButton {
            id: nextBtn
            enabled: view.currentIndex < certModel.pageCount
            text: qsTr("Next")
            icon.source: "qrc:/img/chevron-circle-right.svg"
            width: backBtn.width
            height: backBtn.height
            onClicked: {
                console.log("Current idx " + view.currentIndex)
                if (CertificateModel.NEW_CERT_PAGE_COUNT === certModel.pageCount) {
                    if (0 === view.currentIndex) {
                        certModel.newSubjectName = ""
                        certModel.newSubjectEmail = ""
                    } else if (1 === view.currentIndex) {
                        if (("" === certModel.newSubjectName) || ("" === certModel.newSubjectEmail)) {
                            msgDlg.title = qsTr("Error")
                            msgDlg.text = qsTr("Invalid subject name of email. Please choose valid values.")
                            msgDlg.okCancel = false
                            msgDlg.showDlg()
                            return
                        }
                        const rc = certModel.generateNewCert()
                        if (!rc) {
                            msgDlg.title = qsTr("Error")
                            msgDlg.text = qsTr("There was an error generating your certificate. Please try again.")
                            msgDlg.okCancel = false
                            msgDlg.showDlg()
                            return
                        }
                    } else if (3 === view.currentIndex) {
                        const rc = certModel.exportCert()
                        if (!rc) {
                            return
                        }
                    } else if (4 === view.currentIndex) {
                        certModel.finish()
                    }
                } else if (CertificateModel.IMPORT_CERT_PAGE_COUNT === certModel.pageCount) {
                    if (1 === view.currentIndex) {
                        const rc = certModel.importCert()
                        if (!rc) {
                            return
                        }
                    } else if (3 === view.currentIndex) {
                        certModel.finish()
                    }
                }

                if ((certModel.pageCount - 1) === view.currentIndex) {
                    view.currentIndex = 0;
                } else {
                    view.currentIndex += 1
                }
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
