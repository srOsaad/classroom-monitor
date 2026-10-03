import QtQuick
import QtQuick.Effects
import "NeuPalette.js" as Neu

Item {
    id: root

    property bool sunken: false
    property real radius: Neu.radius
    property color surfaceColor: Neu.base

    property real shadowMargin: Neu.shadowMargin
    property real shadowOffset: Neu.shadowOffset
    property real shadowSoftness: Neu.shadowBlur

    default property alias content: contentArea.data
    readonly property alias contentArea: contentArea

    implicitWidth: 120
    implicitHeight: 56

    Rectangle {
        id: surface
        anchors.fill: parent
        anchors.margins: root.shadowMargin
        radius: root.radius
        color: root.surfaceColor
        visible: false

        Behavior on color {
            ColorAnimation { duration: 110 }
        }
    }

    MultiEffect {
        anchors.fill: surface
        source: surface
        autoPaddingEnabled: true
        shadowEnabled: true
        shadowColor: Neu.shadowDark
        shadowBlur: root.shadowSoftness
        blurMax: 32
        shadowHorizontalOffset: root.sunken ? -root.shadowOffset : root.shadowOffset
        shadowVerticalOffset: root.sunken ? -root.shadowOffset : root.shadowOffset

        Behavior on shadowHorizontalOffset {
            NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on shadowVerticalOffset {
            NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
    }

    MultiEffect {
        anchors.fill: surface
        source: surface
        autoPaddingEnabled: true
        shadowEnabled: true
        shadowColor: Neu.shadowLight
        shadowBlur: root.shadowSoftness
        blurMax: 32
        shadowHorizontalOffset: root.sunken ? root.shadowOffset : -root.shadowOffset
        shadowVerticalOffset: root.sunken ? root.shadowOffset : -root.shadowOffset

        Behavior on shadowHorizontalOffset {
            NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
        Behavior on shadowVerticalOffset {
            NumberAnimation { duration: 90; easing.type: Easing.OutCubic }
        }
    }

    Rectangle {
        anchors.fill: surface
        radius: root.radius
        visible: root.sunken
        opacity: 0.6
        gradient: Gradient {
            GradientStop { position: 0.0; color: Neu.innerShade }

            GradientStop {
                position: Math.min(0.5, 22 / Math.max(surface.height, 1))
                color: "transparent"
            }
        }
    }

    Item {
        id: contentArea
        anchors.fill: surface
    }
}
