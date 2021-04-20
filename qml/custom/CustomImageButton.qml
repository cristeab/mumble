import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

Item {
    id: control
    signal clicked()
    property alias source: controlImage.source
    Image {
        id: controlImage
        anchors.fill: parent
        smooth: true
        fillMode: Image.PreserveAspectFit
        visible: true
        sourceSize: Qt.size(parent.width, parent.height)
    }
    MouseArea {
        anchors.fill: parent
        onClicked: control.clicked()
    }
}
