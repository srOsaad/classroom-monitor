import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import "NeuPalette.js" as Neu

Window {
    id: app

    readonly property bool overlayActive: notificationActive || signalActive
    property bool notificationActive: false
    property bool signalActive: false

    readonly property int pairW: 320
    readonly property int pairH: 340
    readonly property int msgW: 460
    readonly property int sigWin: 112
    readonly property int dotSize: 44
    readonly property int sigInset: 24

    visible: !clientBridge.paired || overlayActive

    width:  !clientBridge.paired ? pairW
            : notificationActive  ? Screen.width
            : signalActive        ? sigWin : 0
    height: !clientBridge.paired ? pairH
            : notificationActive  ? Screen.height
            : signalActive        ? sigWin : 0

    x: !clientBridge.paired ? (Screen.width  - width)  / 2
       : notificationActive ? 0
       : signalActive       ? (Screen.width - sigWin - sigInset) : 0
    y: !clientBridge.paired ? (Screen.height - height) / 2
       : notificationActive ? 0
       : signalActive       ? sigInset : 0

    title: qsTr("Classroom Student")
    color: "transparent"

    flags: !clientBridge.paired
           ? (Qt.Window | Qt.FramelessWindowHint)
           : (Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
              | (overlayActive ? 0 : Qt.WindowTransparentForInput))

    Loader {
        anchors.fill: parent
        active: !clientBridge.paired
        sourceComponent: pairingComp
    }

    Component {
        id: pairingComp
        Item {
            function submit() {
                const code = codeField.text.trim()
                if (code.length > 0) clientBridge.pair(code)
            }
            NeuPanel {
                anchors.fill: parent
                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - 44
                    spacing: 14
                    Text {
                        text: qsTr("Join the classroom")
                        font.pixelSize: 20; font.bold: true; color: Neu.text
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: qsTr("Enter the code shown on the teacher's screen.")
                        font.pixelSize: 12; color: Neu.textMuted
                        wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    NeuTextField {
                        id: codeField
                        placeholderText: qsTr("Pairing code")
                        font.family: "Consolas"; font.capitalization: Font.AllUppercase
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        onAccepted: submit()
                    }
                    NeuButton {
                        text: qsTr("Connect"); accent: true
                        enabled: codeField.text.trim().length > 0
                        Layout.fillWidth: true
                        onClicked: submit()
                    }
                    Text {
                        text: clientBridge.status
                        font.pixelSize: 12; color: Neu.textMuted
                        wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: app.notificationActive

        NeuPanel {
            anchors.centerIn: parent
            width: app.msgW
            height: msgCol.implicitHeight + 64

            ColumnLayout {
                id: msgCol
                anchors.centerIn: parent
                width: parent.width - 56
                spacing: 16
                Text {
                    text: qsTr("Message from teacher")
                    font.pixelSize: 18; font.bold: true; color: Neu.text
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    id: messageText
                    font.pixelSize: 15; color: Neu.text
                    wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                NeuButton {
                    text: qsTr("OK"); accent: true
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: app.notificationActive = false
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: app.signalActive

        MouseArea { anchors.fill: parent; onClicked: app.signalActive = false }

        Rectangle {
            id: signalDot
            anchors.centerIn: parent
            width: app.dotSize; height: app.dotSize
            radius: width / 2
            color: "lightgray"
            border.width: 2
            border.color: Qt.lighter(color, 1.35)

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: signalDot.color
                shadowBlur: 1.0
                shadowScale: 1.1
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 0
                blurMax: 40
            }

            SequentialAnimation on scale {
                running: app.signalActive
                loops: Animation.Infinite
                NumberAnimation { from: 0.9;  to: 1.08; duration: 850; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.08; to: 0.9;  duration: 850; easing.type: Easing.InOutSine }
            }
        }
    }

    Connections {
        target: clientBridge
        function onMessageReceived(text) {
            messageText.text = text
            app.notificationActive = true
        }
        function onSignalReceived(color, text) {
            signalDot.color = color && color.length > 0 ? color : "lightgray"
            app.signalActive = true
        }
    }
}
