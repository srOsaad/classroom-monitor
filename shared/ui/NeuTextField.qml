import QtQuick
import QtQuick.Controls.Basic
import "NeuPalette.js" as Neu

TextField {
    id: control

    implicitWidth: 240
    implicitHeight: 56

    leftPadding: Neu.shadowMargin + 14
    rightPadding: Neu.shadowMargin + 14
    topPadding: Neu.shadowMargin + 2
    bottomPadding: Neu.shadowMargin + 2
    color: Neu.text
    placeholderTextColor: Neu.textMuted
    font.pixelSize: 15
    selectionColor: Neu.accent
    selectedTextColor: Neu.accentText

    background: NeuPanel {

        sunken: true
        surfaceColor: control.activeFocus ? Neu.baseSunkenFocus : Neu.baseSunken
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Neu.shadowMargin - 2
        radius: Neu.radius + 2
        color: "transparent"
        border.color: Neu.accent
        border.width: 2
        visible: control.activeFocus
    }
}
