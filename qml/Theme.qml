import QtQuick 2.12
import QtQuick.Controls.Material 2.12

pragma Singleton

QtObject {
    readonly property real windowMargin: 10

    readonly property real labelFontSize: 12

    readonly property color backgroundColor: Material.background
    readonly property color tabButtonColor: "white"
    readonly property color tabButtonColorSel: "lightblue"
    readonly property color buttonBlueColor: "#0F80FE"
    readonly property color tableBackgroundColor: "#13151E"
    readonly property color errorColor: "red"
}
