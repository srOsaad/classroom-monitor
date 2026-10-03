import QtQuick

Item {
    id: root

    property string frameSource: ""
    property int fillMode: Image.PreserveAspectFit

    property int _front: 0

    function _back() { return _front === 0 ? imgB : imgA }

    onFrameSourceChanged: {
        if (frameSource.length === 0) {
            imgA.source = ""
            imgB.source = ""
            _front = 0
            return
        }

        _back().source = frameSource
    }

    Image {
        id: imgA
        anchors.fill: parent
        fillMode: root.fillMode
        cache: false
        asynchronous: true
        smooth: true
        visible: root._front === 0
        onStatusChanged: if (status === Image.Ready && root._front !== 0) root._front = 0
    }

    Image {
        id: imgB
        anchors.fill: parent
        fillMode: root.fillMode
        cache: false
        asynchronous: true
        smooth: true
        visible: root._front === 1
        onStatusChanged: if (status === Image.Ready && root._front !== 1) root._front = 1
    }
}
