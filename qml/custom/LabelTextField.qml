import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."

Row {
    id: control

    property alias text: controlLabel.text
    property alias editText: controlTextField.text
    property alias error: controlTextField.error
    property alias placeholderText: controlTextField.placeholderText
    property alias validator: controlTextField.validator
    property alias acceptableInput: controlTextField.acceptableInput
    property alias echoMode: controlTextField.echoMode

    signal editingFinished()

    spacing: 5
    width: parent.width
    height: childrenRect.height

    onEditTextChanged: control.error = false

    Label {
        id: controlLabel
        anchors.verticalCenter: controlTextField.verticalCenter
        width: 100
        elide: Text.ElideRight
        color: control.error ? Theme.errorColor : Theme.textColor
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
    }
    CustomTextField {
        id: controlTextField
        font.pointSize: appWin.isBig ? Theme.bigLabelFontSize : Theme.labelFontSize
        width: parent.width - controlLabel.width - control.spacing
        onEditingFinished: control.editingFinished()
    }
}
