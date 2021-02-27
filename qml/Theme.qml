import QtQuick 2.12
import QtQuick.Controls.Material 2.12

pragma Singleton

QtObject {
    readonly property real windowMargin: 10

    readonly property real labelFontSize: 10
    readonly property real titleFontSize: 12

    readonly property color backgroundColor: "#C2CCDA"
    readonly property color backgroundColor2: "#0F80FE"
    readonly property color tabButtonColor: "#130039"
    readonly property color tabButtonColorSel: "#0F80FE"
    readonly property color buttonBlueColor: "#0F80FE"
    readonly property color tableBackgroundColor: "#130039"
    readonly property color tableSelectedBackgroundColor: "#0F80FE"
    readonly property color errorColor: "red"
    readonly property color textColor: "#130039"
    readonly property color textColor2: "white"
    readonly property color borderColor: "#8D007B"

    readonly property color sliderPressedColor: "#f0f0f0"
    readonly property color sliderReleasedColor: "#f6f6f6"
    readonly property color sliderBorderColor: "#bdbebf"

    readonly property color sliderBelowColor: "red"
    readonly property color sliderMiddleColor: "yellow"
    readonly property color sliderAboveColor: "green"
    readonly property color sliderBackgroundColor: "#80000000"
}
