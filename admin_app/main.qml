import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "NeuPalette.js" as Neu

Window {
    id: root

    width: 1100
    height: 740
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: qsTr("Classroom Teacher")
    color: Neu.base

    property string selectedHost: ""
    property var selectedStream: null
    property bool remoteControl: false

    property bool highQuality: false

    property bool screenMaximized: false

    function closeFullView() {
        if (root.remoteControl) adminServer.releaseAllInputs(root.selectedHost)
        root.remoteControl = false
        if (root.screenMaximized) {
            root.screenMaximized = false
            root.visibility = Window.Windowed
        }
        root.highQuality = false
        root.selectedHost = ""
        root.selectedStream = null
    }

    function toggleScreenMaximized() {
        root.screenMaximized = !root.screenMaximized
        root.visibility = root.screenMaximized ? Window.FullScreen : Window.Windowed
    }

    NeuPanel {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.margins: 12
        height: 104

        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            ColumnLayout {
                spacing: 2
                Text {
                    text: qsTr("Classroom Teacher")
                    font.pixelSize: 22; font.bold: true; color: Neu.text
                }
                Text {
                    text: adminServer.listening
                          ? qsTr("%1 connected").arg(adminServer.studentCount)
                          : qsTr("Server stopped")
                    font.pixelSize: 12
                    color: adminServer.listening ? Neu.textMuted : Neu.bad
                }
            }

            Item { Layout.fillWidth: true }

            NeuPanel {
                Layout.preferredWidth: 190
                Layout.preferredHeight: 60
                sunken: true; radius: 12; shadowMargin: 8; surfaceColor: Neu.baseSunken
                Text {
                    anchors.centerIn: parent
                    text: adminServer.pairingCode
                    font.pixelSize: 22; font.bold: true
                    font.family: "Consolas"; font.letterSpacing: 3
                    color: Neu.text
                }
            }

            NeuButton {
                id: regenBtn
                Layout.preferredWidth: 60
                Layout.preferredHeight: 60
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Regenerate pairing code")
                onClicked: regenConfirm.open()

                contentItem: Item {
                    Canvas {
                        id: refreshIcon
                        anchors.centerIn: parent
                        width: 24; height: 24
                        property color stroke: regenBtn.enabled ? Neu.text : Neu.textMuted
                        onStrokeChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            const cx = width / 2, cy = height / 2
                            const r = width * 0.32
                            ctx.lineWidth = width * 0.11
                            ctx.lineCap = "round"
                            ctx.strokeStyle = stroke
                            ctx.fillStyle = stroke
                            const start = -Math.PI * 0.2
                            const end = Math.PI * 1.55
                            ctx.beginPath()
                            ctx.arc(cx, cy, r, start, end)
                            ctx.stroke()

                            const hx = cx + r * Math.cos(start)
                            const hy = cy + r * Math.sin(start)
                            const a = width * 0.19
                            ctx.beginPath()
                            ctx.moveTo(hx, hy)
                            ctx.lineTo(hx - a * 1.25, hy - a * 0.15)
                            ctx.lineTo(hx + a * 0.05, hy - a * 1.25)
                            ctx.closePath()
                            ctx.fill()
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        id: toolbar
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        anchors.leftMargin: 12; anchors.rightMargin: 12
        spacing: 0
        NeuButton { text: qsTr("Start all"); onClicked: adminServer.startAllStreams() }
        NeuButton { text: qsTr("Stop all");  onClicked: adminServer.stopAllStreams() }
        NeuButton { text: qsTr("Message all"); onClicked: { msgDialog.target = ""; msgDialog.open() } }
        NeuButton { text: qsTr("Signal all"); onClicked: { signalDialog.target = ""; signalDialog.open() } }
        Item { Layout.fillWidth: true }
    }

    NeuPanel {
        anchors { top: toolbar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        anchors.margins: 12
        sunken: true; surfaceColor: Neu.baseSunken

        Text {
            anchors.centerIn: parent
            visible: grid.count === 0
            text: qsTr("Waiting for students to join.")
            color: Neu.textMuted; font.pixelSize: 14
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: 8
            clip: true
            cellWidth: 300; cellHeight: 212
            model: streamManager.students

            delegate: Item {
                width: grid.cellWidth; height: grid.cellHeight
                required property var modelData

                NeuPanel {
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: 12; shadowMargin: 8

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 8
                            color: "#11000000"
                            clip: true

                            LiveImage {
                                anchors.fill: parent
                                frameSource: modelData.frame
                                visible: modelData.frame.length > 0
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: modelData.frame.length === 0
                                text: modelData.connected ? qsTr("no frame yet") : qsTr("offline")
                                color: Neu.textMuted; font.pixelSize: 12
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.selectedHost = modelData.hostname
                                    root.selectedStream = modelData
                                    root.remoteControl = false
                                    root.highQuality = false
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle {
                                implicitWidth: 8; implicitHeight: 8; radius: 4
                                color: modelData.connected ? Neu.ok : Neu.textMuted
                            }
                            Text {
                                text: modelData.hostname
                                font.pixelSize: 12; font.bold: true; color: Neu.text
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: fullView
        anchors.fill: parent
        visible: root.selectedHost !== ""
        color: "#e9ecf3"

        function rowFor(host) {
            const list = adminServer.students
            for (let i = 0; i < list.length; i++)
                if (list[i].hostname === host) return list[i]
            return null
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 14

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Text {
                        text: root.selectedHost
                        color: "white"; font.pixelSize: 18; font.bold: true
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }
                    Text {
                        visible: root.remoteControl
                        text: qsTr("remote control on")
                        color: "#ffd27f"; font.pixelSize: 12
                    }

                    NeuButton {
                        text: root.screenMaximized ? qsTr("Exit full window")
                                                   : qsTr("Full window")
                        onClicked: root.toggleScreenMaximized()
                    }

                    NeuButton {
                        text: qsTr("Close")
                        onClicked: root.closeFullView()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#000000"; radius: 8; clip: true

                    LiveImage {
                        id: bigView
                        anchors.fill: parent
                        frameSource: root.selectedStream ? root.selectedStream.frame : ""
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: !root.selectedStream || root.selectedStream.frame.length === 0
                        text: qsTr("no frame")
                        color: "#888888"
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: root.remoteControl
                        hoverEnabled: root.remoteControl
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onPositionChanged: (m) => adminServer.sendMouseEventNormalized(
                            root.selectedHost, "move", m.x / width, m.y / height, 1)
                        onPressed: (m) => adminServer.sendMouseEventNormalized(
                            root.selectedHost, "press", m.x / width, m.y / height,
                            m.button === Qt.RightButton ? 2 : 1)
                        onReleased: (m) => adminServer.sendMouseEventNormalized(
                            root.selectedHost, "release", m.x / width, m.y / height,
                            m.button === Qt.RightButton ? 2 : 1)
                    }
                }
            }

            NeuPanel {
                visible: !root.screenMaximized
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                surfaceColor: Neu.base

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 16
                    contentWidth: width
                    contentHeight: controls.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: controls
                        width: parent.width
                        spacing: 10

                        Text {
                            text: {
                                const r = fullView.rowFor(root.selectedHost)
                                if (!r) return qsTr("offline")
                                return r.username + "  -  " + r.address + "\n"
                                       + qsTr("since %1").arg(r.connectedAt)
                            }
                            color: Neu.textMuted; font.pixelSize: 12
                            wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }

                        NeuToggle {
                            text: qsTr("Remote control")
                            checked: root.remoteControl
                            onToggled: {
                                root.remoteControl = checked
                                if (!checked) adminServer.releaseAllInputs(root.selectedHost)
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            NeuButton {
                                text: root.highQuality ? qsTr("HQ on") : qsTr("HQ off")
                                accent: root.highQuality
                                onClicked: {
                                    root.highQuality = !root.highQuality
                                    adminServer.requestHighQualityStream(root.selectedHost, root.highQuality)
                                }
                            }
                            NeuButton { text: qsTr("Listen"); onClicked: adminServer.startListeningTo(root.selectedHost) }
                            NeuButton { text: qsTr("Stop");   onClicked: adminServer.stopListening() }
                            NeuButton { text: qsTr("Speak");  onClicked: adminServer.startSpeakingTo(root.selectedHost) }
                            NeuButton { text: qsTr("Mute");   onClicked: adminServer.stopSpeaking() }
                            NeuButton { text: qsTr("Signal"); onClicked: { signalDialog.target = root.selectedHost; signalDialog.open() } }
                            NeuButton { text: qsTr("Full view"); onClicked: if (!root.screenMaximized) root.toggleScreenMaximized() }
                        }

                        Text { text: qsTr("Message"); color: Neu.textMuted; font.pixelSize: 12 }
                        NeuTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Message this student")
                            onAccepted: if (text.length) { adminServer.sendMessageTo(root.selectedHost, text); text = "" }
                        }
                        Text { text: qsTr("Run command"); color: Neu.textMuted; font.pixelSize: 12 }
                        NeuTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("e.g. notepad.exe")
                            onAccepted: if (text.length) { adminServer.runCommandOn(root.selectedHost, text); text = "" }
                        }
                        Text { text: qsTr("Type on student"); color: Neu.textMuted; font.pixelSize: 12 }
                        NeuTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Text to type remotely")
                            onAccepted: if (text.length) { adminServer.sendKeySequence(root.selectedHost, text); text = "" }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: regenConfirm
        anchors.centerIn: parent
        modal: true; width: 360
        title: qsTr("Regenerate code?")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: adminServer.regenerateCode()
        contentItem: Text {
            text: qsTr("Connected students stay connected, but anyone pairing from now on needs the new code.")
            wrapMode: Text.WordWrap; color: Neu.text
        }
    }

    Dialog {
        id: msgDialog
        property string target: ""
        anchors.centerIn: parent
        modal: true; width: 360
        title: target.length ? qsTr("Message %1").arg(target) : qsTr("Message everyone")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (msgInput.text.length) {
                if (target.length) adminServer.sendMessageTo(target, msgInput.text)
                else adminServer.sendMessageToAll(msgInput.text)
            }
            msgInput.text = ""
        }
        contentItem: NeuTextField { id: msgInput; placeholderText: qsTr("Message text") }
    }

    Dialog {
        id: signalDialog
        property string target: ""
        anchors.centerIn: parent
        modal: true
        padding: 20
        background: NeuPanel { surfaceColor: Neu.base }

        contentItem: ColumnLayout {
            spacing: 6

            Text {
                text: qsTr("Send a signal")
                font.pixelSize: 18; font.bold: true; color: Neu.text
            }
            Text {
                text: signalDialog.target.length
                      ? qsTr("To %1").arg(signalDialog.target)
                      : qsTr("To every connected student")
                font.pixelSize: 12; color: Neu.textMuted
                Layout.bottomMargin: 10
            }

            RowLayout {
                spacing: 12
                Repeater {
                    model: [ { c: "#15803d", n: qsTr("Green"), m: qsTr("All good") },
                             { c: "#c9a21a", n: qsTr("Amber"), m: qsTr("Slow down") },
                             { c: "#c01f1f", n: qsTr("Red"),   m: qsTr("Stop now") } ]
                    delegate: NeuPanel {
                        id: tile
                        required property var modelData
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 128
                        radius: 14; shadowMargin: 8
                        sunken: tileArea.pressed
                        surfaceColor: tileArea.containsMouse ? Neu.baseHover : Neu.base

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 8
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                implicitWidth: 48; implicitHeight: 48; radius: 24
                                color: tile.modelData.c
                                border.width: 2
                                border.color: Qt.darker(tile.modelData.c, 1.25)
                            }
                            Text {
                                text: tile.modelData.n
                                Layout.alignment: Qt.AlignHCenter
                                font.pixelSize: 14; font.bold: true; color: Neu.text
                            }
                            Text {
                                text: tile.modelData.m
                                Layout.alignment: Qt.AlignHCenter
                                font.pixelSize: 11; color: Neu.textMuted
                            }
                        }

                        MouseArea {
                            id: tileArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (signalDialog.target.length)
                                    adminServer.sendSignalTo(signalDialog.target,
                                                             tile.modelData.c, tile.modelData.m)
                                else
                                    adminServer.sendSignalToAll(tile.modelData.c, tile.modelData.m)
                                signalDialog.close()
                            }
                        }
                    }
                }
            }

            NeuButton {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                Layout.topMargin: 12
                onClicked: signalDialog.close()
            }
        }
    }
}
