import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

CustomTabButton {
    visible: (1 < tabView.depth) && (0 === bar.currentButtonIndex)
    text: qsTr("Back")
    icon {
        width: Theme.buttonIconWidth
        height: Theme.buttonIconWidth
        source: "qrc:/img/chevron-circle-left.svg"
    }
    onClicked: tabView.pop()
}
