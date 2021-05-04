import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

CustomTabButton {
    text: qsTr("Forward")
    icon {
        width: 1.5 * Theme.buttonIconWidth
        height: 1.5 * Theme.buttonIconWidth
        source: "qrc:/img/chevron-circle-right.svg"
    }
}
