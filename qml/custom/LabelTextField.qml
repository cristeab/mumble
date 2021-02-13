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

    spacing: 5

    onEditTextChanged: control.error = false

    Label {
        id: controlLabel
        anchors.verticalCenter: controlTextField.verticalCenter
        width: 100
        elide: Text.ElideRight
        color: control.error ? Theme.errorColor : Theme.textColor
        font {
            italic: true
            pointSize: Theme.labelFontSize
        }
    }
    CustomTextField {
        id: controlTextField
        width: parent.width - controlLabel.width - control.spacing
    }
}
