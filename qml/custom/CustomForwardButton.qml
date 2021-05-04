import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

CustomTabButton {
    text: qsTr("Forward")
    icon {
        width: Theme.buttonIconWidth
        height: Theme.buttonIconWidth
        source: "qrc:/img/chevron-circle-right.svg"
    }
}
