import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: win
    visible: true
    width: 1000
    height: 760
    title: "omapic"
    color: "#1c1c1e"

    // --- geometry: map the displayed (fit) image to source pixels ---
    readonly property int imgW: backend.imageWidth
    readonly property int imgH: backend.imageHeight
    readonly property real fitScale: (imgW > 0 && imgH > 0)
        ? Math.min(canvas.width / imgW, canvas.height / imgH)
        : 1
    readonly property real dispW: imgW * fitScale
    readonly property real dispH: imgH * fitScale

    // Focus target for keybindings.
    Item {
        id: keys
        anchors.fill: parent
        focus: true

        Keys.onPressed: function (e) {
            if (e.key === Qt.Key_Return || e.key === Qt.Key_Enter) {
                band.commit();
                e.accepted = true;
            } else if (e.key === Qt.Key_Escape) {
                band.clear();
                e.accepted = true;
            } else if (e.matches(StandardKey.Undo) ||
                       (e.modifiers & Qt.ControlModifier && e.key === Qt.Key_Z)) {
                backend.undo();
                e.accepted = true;
            } else if (e.modifiers & Qt.ControlModifier && e.key === Qt.Key_C) {
                backend.copyToClipboard();
                e.accepted = true;
            } else if (e.modifiers & Qt.ControlModifier && e.key === Qt.Key_S) {
                backend.save();
                e.accepted = true;
            } else if (e.modifiers & Qt.ControlModifier && e.key === Qt.Key_O) {
                backend.openDialog();
                e.accepted = true;
            }
        }

        // --- image canvas ---
        Item {
            id: canvas
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: statusBar.top
            anchors.margins: 12

            CutBand {
                id: band
                x: (canvas.width - win.dispW) / 2
                y: (canvas.height - win.dispH) / 2
                width: win.dispW
                height: win.dispH
                imgW: win.imgW
                imgH: win.imgH
                revision: backend.revision
                visible: backend.hasImage
                onApply: function (orientation, start, end) {
                    backend.applyCut(orientation, start, end);
                }
            }

            Label {
                anchors.centerIn: parent
                visible: !backend.hasImage
                text: "Open an image  (Ctrl+O)"
                color: "#8e8e93"
                font.pixelSize: 18
            }
        }

        // --- status bar ---
        Rectangle {
            id: statusBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 30
            color: "#2c2c2f"
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: backend.status
                color: "white"
                font.pixelSize: 13
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                text: "drag=band  Enter=cut  Ctrl+Z/C/S/O"
                color: "#8e8e93"
                font.pixelSize: 12
            }
        }
    }
}
