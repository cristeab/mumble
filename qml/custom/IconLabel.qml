import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

Column {
    property alias text: controlLabel.text
    property alias icon: controlIcon.source

    spacing: Theme.windowMargin / 2
    Image {
        id: controlIcon
        source: "qrc:/img/info-circle-solid.svg"
        mipmap: true
        height: 20
        width: height
        fillMode: Image.PreserveAspectFit
    }
    Label {
        id: controlLabel
        color: Theme.textColor
        wrapMode: Text.WordWrap
        width: parent.width
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }
}
