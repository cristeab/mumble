import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import ".."

Row {
    id: control

    property alias leftText: leftLabel.text
    property alias from: middleSlider.from
    property alias to: middleSlider.to
    property alias stepSize: middleSlider.stepSize
    property alias value: middleSlider.value
    property alias rightText: rightLabel.text
    property alias hovered: middleSlider.hovered

    spacing: Theme.windowMargin

    Label {
        id: leftLabel
        anchors.verticalCenter: middleSlider.verticalCenter
        width: 100
        color: Theme.textColor
        clip: true
        elide: Text.ElideRight
        wrapMode: Text.WordWrap
        maximumLineCount: 2
    }
    Slider {
        id: middleSlider
        width: control.width - leftLabel.width - rightLabel.width - 2 * control.spacing
        Material.accent: Theme.backgroundColor2
    }
    Label {
        id: rightLabel
        anchors.verticalCenter: middleSlider.verticalCenter
        width: 50
        color: Theme.textColor
        clip: true
        elide: Text.ElideRight
    }
}
