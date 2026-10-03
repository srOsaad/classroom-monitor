import QtQuick
import QtQuick.Controls.Basic
import "NeuPalette.js" as Neu

Switch {
    id: control

    implicitHeight: 48
    padding: 0
    spacing: 12
    font.pixelSize: 14

    indicator: NeuPanel {
        id: track

        implicitWidth: 76
        implicitHeight: 44
        y: (control.height - height) / 2

        sunken: true
        radius: height / 2
        shadowMargin: 8

        Rectangle {
            id: knob

            width: parent.height - 8
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? parent.width - width - 4 : 4
            color: control.checked ? Neu.ok : Neu.base
            border.width: control.checked ? 0 : 1
            border.color: Neu.shadowDark

            Behavior on x {
                NumberAnimation { duration: 130; easing.type: Easing.OutCubic }
            }
            Behavior on color {
                ColorAnimation { duration: 130 }
            }
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? Neu.text : Neu.textMuted
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        leftPadding: control.indicator.width + control.spacing
    }
}
