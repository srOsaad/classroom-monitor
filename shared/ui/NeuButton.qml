import QtQuick
import QtQuick.Controls.Basic
import "NeuPalette.js" as Neu

Button {
    id: control

    property bool accent: false

    implicitWidth: Math.max(140, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 56

    leftPadding: Neu.shadowMargin + 18
    rightPadding: Neu.shadowMargin + 18
    topPadding: Neu.shadowMargin + 4
    bottomPadding: Neu.shadowMargin + 4
    font.pixelSize: 14
    font.bold: true

    background: NeuPanel {
        sunken: control.pressed
        surfaceColor: {

            if (!control.enabled)
                return Neu.baseSunken
            if (control.accent)
                return control.pressed ? Qt.darker(Neu.accent, 1.12) : Neu.accent
            return control.hovered ? Neu.baseHover : Neu.base
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: {
            if (!control.enabled)
                return Neu.textMuted
            return control.accent ? Neu.accentText : Neu.text
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: Neu.shadowMargin - 2
        radius: Neu.radius + 2
        color: "transparent"
        border.color: Neu.accent
        border.width: 2
        visible: control.visualFocus
    }
}
