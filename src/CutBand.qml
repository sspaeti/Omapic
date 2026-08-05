import QtQuick

// The displayed image plus an interactive cut band. Drag on the image to
// create a band; the dominant drag axis picks orientation. While a band is
// active the image is drawn as two clipped halves that slide together,
// previewing the collapse live. Enter commits the real cut via onApply.
Item {
    id: root

    property int imgW: 0
    property int imgH: 0
    property int revision: 0

    // 0 = none, 1 = horizontal band (cuts height), 2 = vertical band (cuts width)
    property int orientation: 0
    property real bStart: 0     // image px
    property real bEnd: 0       // image px
    readonly property bool active: orientation !== 0 && bEnd > bStart

    readonly property real sx: imgW > 0 ? width / imgW : 1
    readonly property real sy: imgH > 0 ? height / imgH : 1
    readonly property string src: "image://omapic/" + revision
    readonly property color accent: "#FFD60A"

    signal apply(int orientation, int start, int end)

    function clear() { orientation = 0; bStart = 0; bEnd = 0; }
    function commit() {
        if (!active) return;
        // Backend Orientation enum: Horizontal=0, Vertical=1.
        apply(orientation === 1 ? 0 : 1, Math.round(bStart), Math.round(bEnd));
        clear();
    }

    // ---- full image (shown when no band is active) ----
    Image {
        id: full
        anchors.fill: parent
        visible: !root.active
        source: root.src
        cache: false
        fillMode: Image.Stretch
        sourceSize: Qt.size(root.imgW, root.imgH)
    }

    // ---- live collapse preview: two clip-windows onto the full-size image
    // that slide together. Each window is an Item{clip:true} holding a full
    // display-size Image offset inside it, so only the wanted rows/cols show.
    // (Avoids Image.sourceClipRect, which is unreliable on some Qt builds and
    // was rendering the whole image stretched instead of the cut region.) ----
    Item {
        // Horizontal band: rows [0,bStart) on top, rows [bEnd,imgH) pulled up.
        anchors.fill: parent
        visible: root.active && root.orientation === 1
        Item {
            x: 0; y: 0
            width: root.width; height: root.bStart * root.sy
            clip: true
            Image {
                source: root.src; cache: false; fillMode: Image.Stretch
                x: 0; y: 0; width: root.width; height: root.height
                sourceSize: Qt.size(root.imgW, root.imgH)
            }
        }
        Item {
            x: 0; y: root.bStart * root.sy
            width: root.width; height: (root.imgH - root.bEnd) * root.sy
            clip: true
            Image {
                source: root.src; cache: false; fillMode: Image.Stretch
                x: 0; y: -root.bEnd * root.sy; width: root.width; height: root.height
                sourceSize: Qt.size(root.imgW, root.imgH)
            }
        }
    }
    Item {
        // Vertical band: cols [0,bStart) on the left, cols [bEnd,imgW) pulled left.
        anchors.fill: parent
        visible: root.active && root.orientation === 2
        Item {
            x: 0; y: 0
            width: root.bStart * root.sx; height: root.height
            clip: true
            Image {
                source: root.src; cache: false; fillMode: Image.Stretch
                x: 0; y: 0; width: root.width; height: root.height
                sourceSize: Qt.size(root.imgW, root.imgH)
            }
        }
        Item {
            x: root.bStart * root.sx; y: 0
            width: (root.imgW - root.bEnd) * root.sx; height: root.height
            clip: true
            Image {
                source: root.src; cache: false; fillMode: Image.Stretch
                x: -root.bEnd * root.sx; y: 0; width: root.width; height: root.height
                sourceSize: Qt.size(root.imgW, root.imgH)
            }
        }
    }

    // ---- seam line at the collapse point (visual guide while active) ----
    Rectangle {
        visible: root.active && root.orientation === 1
        x: 0; y: root.bStart * root.sy
        width: root.width; height: 2
        color: root.accent
    }
    Rectangle {
        visible: root.active && root.orientation === 2
        x: root.bStart * root.sx; y: 0
        width: 2; height: root.height
        color: root.accent
    }

    // ---- interaction ----
    MouseArea {
        anchors.fill: parent
        property real pressX: 0
        property real pressY: 0
        property bool decided: false

        onPressed: function (m) {
            pressX = m.x; pressY = m.y; decided = false;
            root.orientation = 0; root.bStart = 0; root.bEnd = 0;
        }
        onPositionChanged: function (m) {
            if (root.imgW <= 0) return;
            var dx = Math.abs(m.x - pressX);
            var dy = Math.abs(m.y - pressY);
            if (!decided && (dx > 4 || dy > 4)) {
                root.orientation = (dy >= dx) ? 1 : 2;  // dominant axis
                decided = true;
            }
            if (root.orientation === 1) {
                var y0 = Math.min(pressY, m.y) / root.sy;
                var y1 = Math.max(pressY, m.y) / root.sy;
                root.bStart = Math.max(0, Math.min(y0, root.imgH));
                root.bEnd = Math.max(root.bStart, Math.min(y1, root.imgH));
            } else if (root.orientation === 2) {
                var x0 = Math.min(pressX, m.x) / root.sx;
                var x1 = Math.max(pressX, m.x) / root.sx;
                root.bStart = Math.max(0, Math.min(x0, root.imgW));
                root.bEnd = Math.max(root.bStart, Math.min(x1, root.imgW));
            }
        }
    }
}
