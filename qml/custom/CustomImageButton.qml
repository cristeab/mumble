import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

Item {
    id: control
    signal clicked()
    property alias color: controlOverlay.color
    property alias source: controlImage.source
    Image {
        id: controlImage
        anchors.fill: parent
        mipmap: true
        fillMode: Image.PreserveAspectFit
        visible: false
        sourceSize: Qt.size(parent.width, parent.height)
    }
    ColorOverlay {
        id: controlOverlay
        anchors.fill: controlImage
        source: controlImage
    }
    MouseArea {
        anchors.fill: parent
        onClicked: control.clicked()
    }
}
