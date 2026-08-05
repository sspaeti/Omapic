# omapic Cut-Out Tool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `omapic`, a Qt6 Quick tool that cuts a horizontal or vertical band out of the middle of an image and collapses the remaining pieces together, with a live drag preview.

**Architecture:** Qt Quick (QML) UI + a C++ `Backend` compiled to a single binary with QML embedded via Qt resources (mirrors [omacut](https://github.com/omacom-io/omacut)). The cut engine is pure `QImage` math (no ffmpeg, no ImageMagick). Live collapse-while-dragging is a QML view trick (two clipped `Image` views); the real pixel cut is committed only on Enter.

**Tech Stack:** C++17, Qt6 (`qt6-base`, `qt6-declarative`), qmake6, QtTest, `wl-clipboard`, `xdg-desktop-portal`.

**Reference checkout:** omacut is cached at `~/.cache/checkouts/github.com/omacom-io/omacut` — several files are copied/adapted from it.

**Design spec:** `docs/superpowers/specs/2026-08-05-omapic-cut-out-tool-design.md`

---

## File structure

```
omapic.pro                 # qmake project (adapted from omacut.pro)
src/main.cpp               # app entry, CLI args, registers Backend + ImageProvider
src/backend.h              # Backend QObject: image state, cut, undo, io
src/backend.cpp
src/imageprovider.h        # QQuickImageProvider serving the current QImage
src/imageprovider.cpp
src/portalfilepicker.h     # native Open dialog (adapted from omacut, open-only, image filters)
src/portalfilepicker.cpp
src/filepicker.h           # abstract picker interface (adapted from omacut)
src/Main.qml               # window, image canvas, keybindings, status bar
src/CutBand.qml            # drag-to-create band + two draggable handles (adapted from TrimBar.qml)
src/resources.qrc          # embeds Main.qml, CutBand.qml
tests/backend_tests.cpp    # QtTest cut/undo/save unit tests
tests/backend_tests.pro
bin/build                  # qmake6 + make (copied from omacut)
bin/test                   # build + run tests (copied from omacut)
bin/install                # makepkg -fsi (copied from omacut)
pkgbuild/PKGBUILD
pkgbuild/omapic.desktop
pkgbuild/omapic.install
pkgbuild/omapic.svg
scripts/omapic-capture-cut.sh  # Hyprland wrapper: open omapic on newest screenshot
README.md
```

**Responsibilities:**
- `backend.*` — all image state and operations. The only unit-tested unit.
- `imageprovider.*` — bridges the current `QImage` to QML `Image` elements.
- `Main.qml` — layout, keybindings, coordinate conversion (screen ↔ image px).
- `CutBand.qml` — the drag interaction and live-collapse preview.
- Everything else is build/packaging/glue copied from omacut.

---

## Task 1: Build skeleton — window that opens

**Files:**
- Create: `omapic.pro`, `src/main.cpp`, `src/Main.qml`, `src/resources.qrc`
- Create: `bin/build`, `bin/test`, `bin/install`
- Create: `.gitignore` (already present from spec commit — verify contents)

- [ ] **Step 1: Copy the build scripts from omacut and rename the target**

Copy verbatim, then replace `omacut` → `omapic`:
```bash
mkdir -p bin src tests pkgbuild scripts
cp ~/.cache/checkouts/github.com/omacom-io/omacut/bin/build bin/build
cp ~/.cache/checkouts/github.com/omacom-io/omacut/bin/test  bin/test
cp ~/.cache/checkouts/github.com/omacom-io/omacut/bin/install bin/install
sed -i 's/omacut/omapic/g' bin/build bin/test bin/install
chmod +x bin/build bin/test bin/install
```

- [ ] **Step 2: Write `omapic.pro`**

```pro
QT += core gui qml quick quickcontrols2 dbus
CONFIG += c++17 release
TARGET = omapic
TEMPLATE = app

HEADERS += \
    src/filepicker.h \
    src/portalfilepicker.h \
    src/imageprovider.h \
    src/backend.h

SOURCES += \
    src/main.cpp \
    src/portalfilepicker.cpp \
    src/imageprovider.cpp \
    src/backend.cpp

RESOURCES += src/resources.qrc
```

- [ ] **Step 3: Write `src/resources.qrc`**

```xml
<RCC>
    <qresource prefix="/">
        <file alias="Main.qml">Main.qml</file>
        <file alias="CutBand.qml">CutBand.qml</file>
    </qresource>
</RCC>
```

- [ ] **Step 4: Write a minimal `src/Main.qml` (placeholder window)**

```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: win
    visible: true
    width: 900
    height: 700
    title: "omapic"

    Label {
        anchors.centerIn: parent
        text: "omapic — open an image"
        color: "white"
    }

    color: "#1c1c1e"
}
```

- [ ] **Step 5: Write a minimal `src/main.cpp`**

```cpp
// omapic — a dead-simple image cut-out tool. Qt Quick (QML) UI, QImage cuts.
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("omapic");
    app.setDesktopFileName("omapic");
    app.setWindowIcon(QIcon::fromTheme("omapic"));
    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
```

- [ ] **Step 6: Build and verify the window opens**

Run: `./bin/build`
Expected: prints `Built <root>/build/omapic`, exit 0.
Run (Wayland session): `./build/omapic &` then confirm a window titled "omapic" appears; `kill %1`.

- [ ] **Step 7: Commit**

```bash
git add omapic.pro bin src/main.cpp src/Main.qml src/resources.qrc .gitignore
git commit -m "feat: build skeleton — omapic window opens"
```

---

## Task 2: Cut engine — horizontal cut (TDD)

The pure, deterministic core. `Backend::cut` is `static` so tests call it without a running app.

**Files:**
- Create: `src/backend.h`, `src/backend.cpp`
- Create: `tests/backend_tests.cpp`, `tests/backend_tests.pro`

- [ ] **Step 1: Write `tests/backend_tests.pro`**

```pro
QT += core gui testlib
CONFIG += c++17 testcase
TARGET = backend_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h
SOURCES += backend_tests.cpp ../src/backend.cpp
```

- [ ] **Step 2: Write the failing test `tests/backend_tests.cpp`**

Builds a 4-wide × 6-tall image where each row is a distinct color keyed by its y index, cuts rows `[2,4)` (a 2-row horizontal band), and asserts the result is 4×4 with rows `0,1,4,5` preserved in order.

```cpp
#include <QtTest>
#include <QImage>
#include "backend.h"

// row y gets color rgb(y, 0, 0) so each row is uniquely identifiable.
static QImage stripes(int w, int h) {
    QImage img(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, qRgb(y, 0, 0));
    return img;
}

class BackendTests : public QObject {
    Q_OBJECT
private slots:
    void horizontalCutRemovesBandAndCollapses() {
        QImage src = stripes(4, 6);
        QImage out = Backend::cut(src, Backend::Horizontal, 2, 4);

        QCOMPARE(out.width(), 4);
        QCOMPARE(out.height(), 4);
        // Rows 0,1 unchanged; rows 4,5 pulled up to y=2,3.
        const int expectedRowSource[4] = {0, 1, 4, 5};
        for (int y = 0; y < 4; ++y)
            QCOMPARE(qRed(out.pixel(0, y)), expectedRowSource[y]);
    }
};

QTEST_MAIN(BackendTests)
#include "backend_tests.moc"
```

- [ ] **Step 3: Write `src/backend.h` (interface used by all later tasks)**

```cpp
#pragma once

#include <QImage>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class ImageProvider;
class FilePicker;

// Owns the current image and every operation on it. The cut() functions are
// static and pure so they can be unit-tested without a running application.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY imageChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY imageChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    enum Orientation { Horizontal, Vertical };
    Q_ENUM(Orientation)

    explicit Backend(QObject *parent = nullptr);
    Backend(ImageProvider *provider, FilePicker *picker, QObject *parent = nullptr);

    // --- pure cut engine (tested) ---
    // Removes the band [start, end) along the orientation axis and collapses.
    // Horizontal removes rows (reduces height); Vertical removes columns.
    static QImage cut(const QImage &src, Orientation o, int start, int end);

    int revision() const { return m_revision; }
    int imageWidth() const { return m_image.width(); }
    int imageHeight() const { return m_image.height(); }
    bool hasImage() const { return !m_image.isNull(); }
    bool canUndo() const { return !m_undo.isEmpty(); }
    QString status() const { return m_status; }
    const QImage &image() const { return m_image; }

    Q_INVOKABLE bool load(const QUrl &url);
    Q_INVOKABLE bool loadClipboard();
    Q_INVOKABLE void applyCut(int orientation, int start, int end);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void copyToClipboard();
    Q_INVOKABLE QString save();
    Q_INVOKABLE void openDialog();

signals:
    void imageChanged();
    void statusChanged();

private:
    void setImage(const QImage &img);   // pushes provider + bumps revision
    void setStatus(const QString &s);

    ImageProvider *m_provider = nullptr;
    FilePicker *m_picker = nullptr;
    QImage m_image;
    QUrl m_source;
    QList<QImage> m_undo;
    int m_revision = 0;
    QString m_status;
};
```

- [ ] **Step 4: Write the minimal `src/backend.cpp` needed to pass (cut + trivial ctor)**

Implement only `cut` and the constructors now; other methods are filled in later tasks. Provide empty/no-op bodies for the not-yet-tested invokables so the file compiles.

```cpp
#include "backend.h"

Backend::Backend(QObject *parent) : QObject(parent) {}
Backend::Backend(ImageProvider *provider, FilePicker *picker, QObject *parent)
    : QObject(parent), m_provider(provider), m_picker(picker) {}

QImage Backend::cut(const QImage &src, Orientation o, int start, int end) {
    if (src.isNull())
        return src;
    if (o == Horizontal) {
        const int h = src.height();
        start = qBound(0, start, h);
        end = qBound(start, end, h);
        const int band = end - start;
        if (band <= 0)
            return src;
        QImage out(src.width(), h - band, src.format());
        // rows [0, start)
        for (int y = 0; y < start; ++y)
            memcpy(out.scanLine(y), src.scanLine(y), src.bytesPerLine());
        // rows [end, h) pulled up
        for (int y = end; y < h; ++y)
            memcpy(out.scanLine(y - band), src.scanLine(y), src.bytesPerLine());
        return out;
    } else {
        // Vertical: implemented in Task 3.
        return src;
    }
}

// Stubs filled in later tasks:
void Backend::setImage(const QImage &img) { m_image = img; }
void Backend::setStatus(const QString &s) { m_status = s; }
bool Backend::load(const QUrl &) { return false; }
bool Backend::loadClipboard() { return false; }
void Backend::applyCut(int, int, int) {}
void Backend::undo() {}
void Backend::copyToClipboard() {}
QString Backend::save() { return {}; }
void Backend::openDialog() {}
```

- [ ] **Step 5: Run the test — expect FAIL first, then PASS**

Run: `./bin/test`
Expected first run before Step 4 code exists: compile error / fail. After Step 4: `PASS   : BackendTests::horizontalCutRemovesBandAndCollapses`, `Totals: ... passed`.

- [ ] **Step 6: Commit**

```bash
git add src/backend.h src/backend.cpp tests/backend_tests.cpp tests/backend_tests.pro
git commit -m "feat: horizontal cut engine + tests"
```

---

## Task 3: Cut engine — vertical cut (TDD)

**Files:**
- Modify: `src/backend.cpp` (fill the `Vertical` branch)
- Modify: `tests/backend_tests.cpp`

- [ ] **Step 1: Add a failing vertical-cut test to `tests/backend_tests.cpp`**

Column x gets color `rgb(x,0,0)`; cut columns `[1,3)` from a 5×3 image → 3×3 with columns `0,3,4` preserved.

```cpp
    void verticalCutRemovesBandAndCollapses() {
        // column x -> rgb(x,0,0)
        QImage src(5, 3, QImage::Format_RGB32);
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 5; ++x)
                src.setPixel(x, y, qRgb(x, 0, 0));

        QImage out = Backend::cut(src, Backend::Vertical, 1, 3);

        QCOMPARE(out.width(), 3);
        QCOMPARE(out.height(), 3);
        const int expectedColSource[3] = {0, 3, 4};
        for (int x = 0; x < 3; ++x)
            QCOMPARE(qRed(out.pixel(x, 0)), expectedColSource[x]);
    }
```

- [ ] **Step 2: Run — expect FAIL**

Run: `./bin/test`
Expected: `FAIL!  : BackendTests::verticalCutRemovesBandAndCollapses` (width is 5, not 3, because the branch returns src).

- [ ] **Step 3: Implement the `Vertical` branch in `src/backend.cpp`**

Replace the `// Vertical: implemented in Task 3.` block with:
```cpp
        const int w = src.width();
        start = qBound(0, start, w);
        end = qBound(start, end, w);
        const int band = end - start;
        if (band <= 0)
            return src;
        QImage out(w - band, src.height(), src.format());
        for (int y = 0; y < src.height(); ++y) {
            const QRgb *in = reinterpret_cast<const QRgb *>(src.scanLine(y));
            QRgb *o = reinterpret_cast<QRgb *>(out.scanLine(y));
            for (int x = 0; x < start; ++x)
                o[x] = in[x];
            for (int x = end; x < w; ++x)
                o[x - band] = in[x];
        }
        return out;
```
Note: the per-pixel `QRgb` copy assumes a 32-bit format; `load()` (Task 5) normalizes every image to `Format_RGB32`/`ARGB32` so this holds.

- [ ] **Step 4: Run — expect PASS**

Run: `./bin/test`
Expected: both cut tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/backend.cpp tests/backend_tests.cpp
git commit -m "feat: vertical cut engine + test"
```

---

## Task 4: applyCut, undo, and edge cases (TDD)

**Files:**
- Modify: `src/backend.cpp` (`applyCut`, `undo`, `setImage`)
- Modify: `tests/backend_tests.cpp`

- [ ] **Step 1: Add failing tests for applyCut + undo + edge band**

`applyCut` takes an `int` orientation (QML passes the `Q_ENUM` value). Seed the backend image via a small test-only helper: since `m_image` is private, drive through `load()` from a temp file is heavy — instead test the observable contract by calling `applyCut` after injecting an image through a friend-free path. Use `save`/`load`? Simpler: expose seeding via `load` of an in-memory temp PNG.

```cpp
    void applyCutThenUndoRestores() {
        // Write a 4x6 striped PNG to a temp file, load it, cut, undo.
        QTemporaryFile f;
        f.setFileTemplate(QDir::tempPath() + "/omapicXXXXXX.png");
        QVERIFY(f.open());
        stripes(4, 6).save(f.fileName(), "PNG");

        Backend b;
        QVERIFY(b.load(QUrl::fromLocalFile(f.fileName())));
        QCOMPARE(b.imageHeight(), 6);
        QVERIFY(!b.canUndo());

        b.applyCut(Backend::Horizontal, 2, 4);
        QCOMPARE(b.imageHeight(), 4);
        QVERIFY(b.canUndo());

        b.undo();
        QCOMPARE(b.imageHeight(), 6);
        QVERIFY(!b.canUndo());
    }

    void edgeBandActsLikeCrop() {
        QImage out = Backend::cut(stripes(4, 6), Backend::Horizontal, 0, 2);
        QCOMPARE(out.height(), 4);
        QCOMPARE(qRed(out.pixel(0, 0)), 2); // top two rows removed
    }
```
Add includes at the top of the test file: `#include <QTemporaryFile>`, `#include <QDir>`.

This test depends on `load()` (Task 5). If executing strictly in order, implement `load()` (Task 5 Step 3) before running. Otherwise reorder: do Task 5 first. Both are fine; note the dependency.

- [ ] **Step 2: Implement `applyCut`, `undo`, `setImage` in `src/backend.cpp`**

Replace the stub bodies:
```cpp
void Backend::setImage(const QImage &img) {
    m_image = img;
    ++m_revision;
    if (m_provider)
        m_provider->setImage(m_image);   // ImageProvider from Task 5
    emit imageChanged();
}

void Backend::applyCut(int orientation, int start, int end) {
    if (m_image.isNull())
        return;
    const Orientation o = static_cast<Orientation>(orientation);
    QImage next = cut(m_image, o, start, end);
    if (next.size() == m_image.size())
        return;                            // nothing removed
    m_undo.append(m_image);
    setImage(next);
    setStatus(QStringLiteral("Cut applied"));
}

void Backend::undo() {
    if (m_undo.isEmpty())
        return;
    setImage(m_undo.takeLast());
    setStatus(QStringLiteral("Undone"));
}
```
Add `#include "imageprovider.h"` at the top of `backend.cpp` (guarded — provider may be null in tests, which is why `setImage` null-checks `m_provider`).

- [ ] **Step 3: Run — expect PASS (after Task 5's `load()` exists)**

Run: `./bin/test`
Expected: all four tests PASS.

- [ ] **Step 4: Commit**

```bash
git add src/backend.cpp tests/backend_tests.cpp
git commit -m "feat: applyCut + undo with edge-band handling + tests"
```

---

## Task 5: Image loading + ImageProvider

**Files:**
- Create: `src/imageprovider.h`, `src/imageprovider.cpp`
- Modify: `src/backend.cpp` (`load`), `omapic.pro` already lists these
- Modify: `tests/backend_tests.pro` is unaffected (provider not linked in tests)

- [ ] **Step 1: Write `src/imageprovider.h`**

```cpp
#pragma once

#include <QImage>
#include <QQuickImageProvider>

// Serves the backend's current image to QML. QML requests
// "image://omapic/<revision>"; the <revision> path segment busts Qt's cache.
class ImageProvider : public QQuickImageProvider {
public:
    ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

    void setImage(const QImage &img) { m_image = img; }

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    QImage m_image;
};
```

- [ ] **Step 2: Write `src/imageprovider.cpp`**

```cpp
#include "imageprovider.h"

QImage ImageProvider::requestImage(const QString &id, QSize *size,
                                   const QSize &requestedSize) {
    Q_UNUSED(id);
    QImage img = m_image;
    if (size)
        *size = img.size();
    if (requestedSize.isValid() && !img.isNull())
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}
```

- [ ] **Step 3: Implement `Backend::load` in `src/backend.cpp`**

Replace the `load` stub:
```cpp
bool Backend::load(const QUrl &url) {
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QImage img(path);
    if (img.isNull()) {
        setStatus(QStringLiteral("Could not open: %1").arg(path));
        return false;
    }
    // Normalize to 32-bit so the cut engine's QRgb access is always valid.
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_source = url;
    m_undo.clear();
    setImage(img);
    setStatus(QStringLiteral("Loaded %1×%2").arg(img.width()).arg(img.height()));
    return true;
}
```

- [ ] **Step 4: Run tests — the Task 4 `applyCutThenUndoRestores` now passes**

Run: `./bin/test`
Expected: all tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/imageprovider.h src/imageprovider.cpp src/backend.cpp
git commit -m "feat: image loading + QQuickImageProvider"
```

---

## Task 6: Save to monthly folder (TDD)

`save()` writes a fresh timestamped PNG into `<pictures>/Printscreen/<YYYY-MM>/`, never touching the source. The Pictures root honors `XDG_PICTURES_DIR`, which makes it testable against a temp dir.

**Files:**
- Modify: `src/backend.cpp` (`save`)
- Modify: `tests/backend_tests.cpp`

- [ ] **Step 1: Add a failing save test**

```cpp
    void saveWritesTimestampedPngInMonthlyFolder() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("XDG_PICTURES_DIR", dir.path().toUtf8());

        QTemporaryFile f;
        f.setFileTemplate(QDir::tempPath() + "/omapicXXXXXX.png");
        QVERIFY(f.open());
        stripes(8, 8).save(f.fileName(), "PNG");

        Backend b;
        QVERIFY(b.load(QUrl::fromLocalFile(f.fileName())));
        const QString out = b.save();

        QVERIFY(!out.isEmpty());
        QVERIFY(QFile::exists(out));
        const QString month = QDate::currentDate().toString("yyyy-MM");
        QVERIFY(out.contains("/Printscreen/" + month + "/"));
        QVERIFY(out.endsWith(".png"));
        // Source file is untouched.
        QVERIFY(QFile::exists(f.fileName()));
    }
```
Add includes: `#include <QTemporaryDir>`, `#include <QDate>`, `#include <QFile>`.

- [ ] **Step 2: Run — expect FAIL**

Run: `./bin/test`
Expected: `FAIL!` — `save()` returns empty.

- [ ] **Step 3: Implement `Backend::save` in `src/backend.cpp`**

```cpp
QString Backend::save() {
    if (m_image.isNull())
        return {};

    QString pics = qEnvironmentVariable("XDG_PICTURES_DIR");
    if (pics.isEmpty())
        pics = QDir::homePath() + QStringLiteral("/Pictures");

    const QString month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
    const QString dir = pics + QStringLiteral("/Printscreen/") + month;
    QDir().mkpath(dir);

    const QString stamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
    const QString path = dir + QStringLiteral("/screenshot-") + stamp + QStringLiteral(".png");

    if (!m_image.save(path, "PNG")) {
        setStatus(QStringLiteral("Save failed"));
        return {};
    }
    setStatus(QStringLiteral("Saved %1").arg(path));
    return path;
}
```
Add includes to `backend.cpp` top: `#include <QDir>`, `#include <QDate>`, `#include <QDateTime>`, `#include <QStandardPaths>` (StandardPaths not strictly needed but harmless — omit if unused).

- [ ] **Step 4: Run — expect PASS**

Run: `./bin/test`
Expected: all tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/backend.cpp tests/backend_tests.cpp
git commit -m "feat: save cut image to monthly Printscreen folder + test"
```

---

## Task 7: Clipboard copy + clipboard load (via wl-clipboard)

Uses `wl-copy`/`wl-paste` through `QProcess` so the image survives after omapic exits. Verified manually (clipboard state is not unit-testable in CI).

**Files:**
- Modify: `src/backend.cpp` (`copyToClipboard`, `loadClipboard`)

- [ ] **Step 1: Implement `copyToClipboard` in `src/backend.cpp`**

```cpp
void Backend::copyToClipboard() {
    if (m_image.isNull())
        return;
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    m_image.save(&buf, "PNG");
    buf.close();

    QProcess proc;
    proc.start(QStringLiteral("wl-copy"),
               {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!proc.waitForStarted(2000)) {
        setStatus(QStringLiteral("wl-copy not available"));
        return;
    }
    proc.write(png);
    proc.closeWriteChannel();
    proc.waitForFinished(2000);
    setStatus(QStringLiteral("Copied to clipboard"));
}
```

- [ ] **Step 2: Implement `loadClipboard` in `src/backend.cpp`**

```cpp
bool Backend::loadClipboard() {
    QProcess proc;
    proc.start(QStringLiteral("wl-paste"),
               {QStringLiteral("--type"), QStringLiteral("image/png")});
    if (!proc.waitForStarted(2000) || !proc.waitForFinished(3000)) {
        setStatus(QStringLiteral("wl-paste not available"));
        return false;
    }
    const QByteArray data = proc.readAllStandardOutput();
    QImage img;
    if (!img.loadFromData(data, "PNG")) {
        setStatus(QStringLiteral("Clipboard has no image"));
        return false;
    }
    img = img.convertToFormat(QImage::Format_ARGB32);
    m_source = QUrl();
    m_undo.clear();
    setImage(img);
    setStatus(QStringLiteral("Loaded from clipboard"));
    return true;
}
```
Add includes to `backend.cpp` top: `#include <QBuffer>`, `#include <QProcess>`.

- [ ] **Step 3: Build and smoke-test the clipboard round-trip**

Run: `./bin/build`
Run: `grim -g "0,0 200x200" /tmp/omapic-smoke.png && wl-copy < /tmp/omapic-smoke.png`
This step is fully verified in Task 11 once keybindings exist; for now just confirm it compiles and links.
Expected: `./bin/build` succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/backend.cpp
git commit -m "feat: clipboard copy/paste via wl-clipboard"
```

---

## Task 8: Native Open dialog (portal file picker)

Adapt omacut's portal picker to open-only with image filters.

**Files:**
- Create: `src/filepicker.h`, `src/portalfilepicker.h`, `src/portalfilepicker.cpp`
- Modify: `src/backend.cpp` (`openDialog`, wire `openSelected`)

- [ ] **Step 1: Write `src/filepicker.h` (open-only interface)**

```cpp
#pragma once

#include <QObject>
#include <QUrl>

class FilePicker : public QObject {
    Q_OBJECT
public:
    explicit FilePicker(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~FilePicker() = default;
    virtual void openImage() = 0;

signals:
    void openSelected(const QUrl &url);
    void failed(const QString &message);
};
```

- [ ] **Step 2: Copy omacut's portal picker and reduce it to open-only**

```bash
cp ~/.cache/checkouts/github.com/omacom-io/omacut/src/portalfilepicker.h  src/portalfilepicker.h
cp ~/.cache/checkouts/github.com/omacom-io/omacut/src/portalfilepicker.cpp src/portalfilepicker.cpp
```
Then edit both files:
- In `portalfilepicker.h`: remove `exportVideo(...)` and the `Export` enum value; rename `openVideo()` → `openImage()`; drop `m_pendingExportStart`/`m_pendingExportEnd`; keep `Action { None, Open }`.
- In `portalfilepicker.cpp`:
  - Replace `videoFilter()`/`videoFilters()` with an image filter:
    ```cpp
    PortalFileFilter imageFilter() {
        return {
            QStringLiteral("Images"),
            {
                {1, QStringLiteral("image/*")},
                {0, QStringLiteral("*.png")},
                {0, QStringLiteral("*.jpg")},
                {0, QStringLiteral("*.jpeg")},
                {0, QStringLiteral("*.webp")},
                {0, QStringLiteral("*.bmp")},
            },
        };
    }
    PortalFileFilters imageFilters() {
        return { imageFilter(), {QStringLiteral("All files"), {{0, QStringLiteral("*")}}} };
    }
    ```
  - Delete `mp4Filter()`/`mp4Filters()` and the whole `exportVideo(...)` method.
  - Rename `openVideo()` → `openImage()`, its title to `"Open Image File"`, and its filters to `imageFilters()`/`imageFilter()`.
  - In `handleResponse`, remove the `Action::Export` branch (keep only `Action::Open` → `emit openSelected(url)`).
  - Change the `portalToken()` prefix from `omacut_` to `omapic_`.

- [ ] **Step 3: Implement `Backend::openDialog` and wire the picker**

In the `Backend(ImageProvider*, FilePicker*, ...)` constructor body, connect the picker:
```cpp
Backend::Backend(ImageProvider *provider, FilePicker *picker, QObject *parent)
    : QObject(parent), m_provider(provider), m_picker(picker) {
    if (m_picker) {
        connect(m_picker, &FilePicker::openSelected, this,
                [this](const QUrl &url) { load(url); });
        connect(m_picker, &FilePicker::failed, this,
                [this](const QString &m) { setStatus(m); });
    }
}
```
Replace the `openDialog` stub:
```cpp
void Backend::openDialog() {
    if (m_picker)
        m_picker->openImage();
}
```
Add `#include "filepicker.h"` at the top of `backend.cpp`.

- [ ] **Step 4: Build**

Run: `./bin/build`
Expected: compiles and links (`Built .../omapic`).

- [ ] **Step 5: Commit**

```bash
git add src/filepicker.h src/portalfilepicker.h src/portalfilepicker.cpp src/backend.cpp
git commit -m "feat: native Open dialog via xdg portal (image filters)"
```

---

## Task 9: main.cpp — wire backend, provider, picker, CLI args

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Rewrite `src/main.cpp` to register everything and parse args**

```cpp
// omapic — a dead-simple image cut-out tool. Qt Quick (QML) UI, QImage cuts.
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "imageprovider.h"
#include "portalfilepicker.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("omapic");
    app.setDesktopFileName("omapic");
    app.setWindowIcon(QIcon::fromTheme("omapic"));
    QQuickStyle::setStyle("Material");

    auto *provider = new ImageProvider();
    auto *picker = new PortalFilePicker(&app);
    Backend backend(provider, picker, &app);

    QQmlApplicationEngine engine;
    engine.addImageProvider("omapic", provider);   // engine takes ownership
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    // CLI: `omapic --clipboard` or `omapic <file>`
    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--clipboard"))) {
        backend.loadClipboard();
    } else if (args.size() > 1 && !args.at(1).startsWith(QStringLiteral("--"))) {
        backend.load(QUrl::fromLocalFile(args.at(1)));
    }

    return app.exec();
}
```

- [ ] **Step 2: Build and run**

Run: `./bin/build && ./build/omapic ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/omacut.svg 2>/dev/null || true`
(SVG won't load as a raster; expected status "Could not open" — this only confirms the arg path runs.)
Better: create a test PNG and open it:
```bash
magick -size 300x400 gradient: /tmp/omapic-test.png
./build/omapic /tmp/omapic-test.png &
```
Expected: window opens; the placeholder Main.qml still shows (canvas comes in Task 10). `kill %1`.

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: wire backend/provider/picker + CLI args (file, --clipboard)"
```

---

## Task 10: Main.qml — image canvas, scaling, status, keybindings

Manual verification (GUI). Displays the current image scaled-to-fit and wires every keybinding to the backend.

**Files:**
- Modify: `src/Main.qml`

- [ ] **Step 1: Rewrite `src/Main.qml`**

```qml
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
```

- [ ] **Step 2: Build (CutBand.qml doesn't exist yet — expect a QML error)**

Run: `./bin/build`
Expected: build succeeds (QML is loaded at runtime). Running now would warn "CutBand is not a type" — that's fixed in Task 11. Do not run yet.

- [ ] **Step 3: Commit**

```bash
git add src/Main.qml
git commit -m "feat: image canvas, fit-scaling, status bar, keybindings"
```

---

## Task 11: CutBand.qml — drag band, orientation auto-select, live collapse

The interaction. Adapted from omacut's `TrimBar.qml` two-handle pattern. Manual verification.

**Files:**
- Create: `src/CutBand.qml`

- [ ] **Step 1: Write `src/CutBand.qml`**

Coordinates: the item is the displayed image rectangle (size `dispW×dispH`). `scale = width/imgW`. The band is stored in **image pixels** (`bStart`,`bEnd`) so it converts cleanly for `applyCut`.

```qml
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

    // ---- live collapse preview: two clipped views that slide together ----
    // Horizontal band: top view = rows [0,bStart), bottom view = rows [bEnd,imgH)
    // drawn immediately below, so the band's rows vanish in real time.
    Item {
        anchors.fill: parent
        visible: root.active && root.orientation === 1
        Image {
            id: hTop
            source: root.src; cache: false; fillMode: Image.Stretch
            x: 0; y: 0
            width: root.width; height: root.bStart * root.sy
            sourceSize: Qt.size(root.imgW, root.imgH)
            sourceClipRect: Qt.rect(0, 0, root.imgW, root.bStart)
        }
        Image {
            id: hBot
            source: root.src; cache: false; fillMode: Image.Stretch
            x: 0; y: root.bStart * root.sy
            width: root.width; height: (root.imgH - root.bEnd) * root.sy
            sourceSize: Qt.size(root.imgW, root.imgH)
            sourceClipRect: Qt.rect(0, root.bEnd, root.imgW, root.imgH - root.bEnd)
        }
    }
    Item {
        anchors.fill: parent
        visible: root.active && root.orientation === 2
        Image {
            id: vLeft
            source: root.src; cache: false; fillMode: Image.Stretch
            x: 0; y: 0
            width: root.bStart * root.sx; height: root.height
            sourceSize: Qt.size(root.imgW, root.imgH)
            sourceClipRect: Qt.rect(0, 0, root.bStart, root.imgH)
        }
        Image {
            id: vRight
            source: root.src; cache: false; fillMode: Image.Stretch
            x: root.bStart * root.sx; y: 0
            width: (root.imgW - root.bEnd) * root.sx; height: root.height
            sourceSize: Qt.size(root.imgW, root.imgH)
            sourceClipRect: Qt.rect(root.bEnd, 0, root.imgW - root.bEnd, root.imgH)
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
```

Note on handles: v1 creates the band by a single drag and commits on Enter (drag again to redo before committing). Explicit re-draggable edge handles (the `TrimBar.qml` Loader/handle components) are a follow-up; the live preview already updates continuously during the initial drag, which satisfies "live collapse-while-dragging."

- [ ] **Step 2: Build and run the full interaction**

```bash
magick -size 400x600 gradient:blue-red /tmp/omapic-test.png
./bin/build && ./build/omapic /tmp/omapic-test.png &
```
Manual checks:
- Image appears centered, scaled to fit.
- Drag vertically in the middle → a horizontal band forms and the lower part slides up live (gap closes as you drag).
- Release, press **Enter** → image is now shorter (band removed); status shows "Cut applied".
- **Ctrl+Z** → image restored.
- Drag horizontally → vertical band; Enter → narrower image.
- **Ctrl+S** → file appears under `~/Pictures/Printscreen/<YYYY-MM>/`.
- **Ctrl+C** → `wl-paste --type image/png > /tmp/c.png` yields the cut image.
- **Ctrl+O** → native file dialog opens.
Then `kill %1`.

- [ ] **Step 3: Commit**

```bash
git add src/CutBand.qml
git commit -m "feat: CutBand — drag band, orientation auto-select, live collapse preview"
```

---

## Task 12: Packaging — PKGBUILD, desktop, icon, install

**Files:**
- Create: `pkgbuild/PKGBUILD`, `pkgbuild/omapic.desktop`, `pkgbuild/omapic.install`, `pkgbuild/omapic.svg`

- [ ] **Step 1: Copy omacut's packaging files as a starting point**

```bash
cp ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/PKGBUILD        pkgbuild/PKGBUILD
cp ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/omacut.desktop  pkgbuild/omapic.desktop
cp ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/omacut.install  pkgbuild/omapic.install
cp ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/omacut.svg      pkgbuild/omapic.svg
```

- [ ] **Step 2: Edit the packaging metadata**

- `pkgbuild/PKGBUILD`: set `pkgname=omapic`, update `pkgdesc` to "Cut horizontal/vertical bands out of images", set `depends=('qt6-base' 'qt6-declarative' 'wl-clipboard' 'xdg-desktop-portal')`, `makedepends=('qt6-base' 'qt6-declarative')`, and replace every `omacut` path/filename with `omapic` (build invocation `./bin/build`, install of `build/omapic`, `.desktop`, `.svg`, `.install`). Remove any `ffmpeg`/`qt6-multimedia` deps.
- `pkgbuild/omapic.desktop`: `Name=omapic`, `Exec=omapic %f`, `Icon=omapic`, `Comment=Cut bands out of images`, `MimeType=image/png;image/jpeg;image/webp;`, `StartupWMClass=omapic`.
- `pkgbuild/omapic.install`: replace `omacut` → `omapic` in any icon-cache hooks.
- `pkgbuild/omapic.svg`: leave the omacut icon for now (a custom icon is a nice-to-have; note it in README).

Read omacut's PKGBUILD first to match its exact structure:
```bash
cat ~/.cache/checkouts/github.com/omacom-io/omacut/pkgbuild/PKGBUILD
```

- [ ] **Step 3: Build the package locally**

Run: `./bin/install`
Expected: `makepkg -fsi` builds and installs the `omapic` package; `which omapic` resolves; `omapic /tmp/omapic-test.png` launches from PATH.

- [ ] **Step 4: Commit**

```bash
git add pkgbuild/
git commit -m "feat: Arch package (PKGBUILD, desktop, icon, install)"
```

---

## Task 13: Hyprland integration + README

**Files:**
- Create: `scripts/omapic-capture-cut.sh`
- Create: `README.md`

- [ ] **Step 1: Write `scripts/omapic-capture-cut.sh`**

Opens omapic on the most-recent screenshot (searches the Pictures root and the monthly Printscreen folder, matching the user's existing scheme).

```bash
#!/bin/bash
# Open omapic on the most-recent screenshot for a quick cut-out.

[[ -f ~/.config/user-dirs.dirs ]] && source ~/.config/user-dirs.dirs
PICS="${XDG_PICTURES_DIR:-$HOME/Pictures}"

# Newest screenshot-*.png anywhere under Pictures (root + monthly folders).
latest="$(find "$PICS" -type f -name 'screenshot-*.png' -printf '%T@ %p\n' 2>/dev/null \
          | sort -nr | head -1 | cut -d' ' -f2-)"

if [[ -n "$latest" ]]; then
    exec omapic "$latest"
else
    exec omapic --clipboard
fi
```
Make it executable: `chmod +x scripts/omapic-capture-cut.sh`.

- [ ] **Step 2: Document the Hyprland keybind in README**

The user adds this to their Hyprland config themselves (do not edit `~/.config` from the repo):
```
bindd = SUPER ALT, C, Cut-out image, exec, ~/git/images/omapic/scripts/omapic-capture-cut.sh
```

- [ ] **Step 3: Write `README.md`**

```markdown
# omapic

A dead-simple image **cut-out** tool for Hyprland/Arch. Open an image, drag a
horizontal or vertical band out of the middle, watch the gap collapse live,
press Enter. Snagit's "cut out", ported to Linux.

Built like [omacut](https://github.com/omacom-io/omacut): Qt Quick (QML) UI +
a C++ backend compiled to a single binary. Cuts use Qt's `QImage` — no ffmpeg,
no ImageMagick at runtime.

## Keys
- **drag** — create a band (drag direction picks horizontal vs vertical)
- **Enter** — apply the cut
- **Ctrl+Z** — undo
- **Ctrl+C** — copy result to clipboard
- **Ctrl+S** — save to `~/Pictures/Printscreen/<YYYY-MM>/`
- **Ctrl+O** — open a file
- **Esc** — clear the current band

## Run
    omapic image.png       # open a file
    omapic --clipboard     # load the current clipboard image

## Hyprland keybind
    bindd = SUPER ALT, C, Cut-out image, exec, ~/git/images/omapic/scripts/omapic-capture-cut.sh

## Build
    ./bin/build            # -> build/omapic
    ./bin/test             # run unit tests
    ./bin/install          # build + install the Arch package

Requires: `qt6-base`, `qt6-declarative`, `wl-clipboard`, `xdg-desktop-portal`.

## Not yet
- Multiple simultaneous cut lines (one at a time for now)
- Re-draggable band edge handles after the initial drag
- A custom app icon (currently reuses omacut's)
```

- [ ] **Step 4: Commit**

```bash
git add scripts/omapic-capture-cut.sh README.md
git commit -m "feat: Hyprland capture-cut wrapper + README"
```

---

## Task 14: Final verification pass

- [ ] **Step 1: Full clean build + tests**

Run: `rm -rf build && ./bin/build && ./bin/test`
Expected: build succeeds; all backend tests PASS (horizontal, vertical, applyCut+undo, edge band, save).

- [ ] **Step 2: End-to-end manual smoke**

```bash
magick -size 500x700 gradient:green-black /tmp/omapic-e2e.png
./build/omapic /tmp/omapic-e2e.png &
```
Verify each keybinding per Task 11 Step 2, plus that `omapic --clipboard` works after `wl-copy < /tmp/omapic-e2e.png`. Then `kill %1`.

- [ ] **Step 3: Confirm no leftover omacut references**

Run: `grep -rn "omacut\|ffmpeg\|multimedia\|thumb" src omapic.pro tests pkgbuild || echo "clean"`
Expected: `clean` (or only intentional references in README/comments). Fix any stragglers.

- [ ] **Step 4: Final commit / tag**

```bash
git add -A
git commit -m "chore: final verification pass" --allow-empty
```

---

## Self-review notes

- **Spec coverage:** load (file/clipboard/dialog) — T5,T7,T8; horizontal+vertical cut — T2,T3; live collapse — T11; one-at-a-time + Ctrl+Z undo — T4; Ctrl+C clipboard — T7; Ctrl+S monthly folder — T6; Hyprland keybind — T13; testing — T2–T6; packaging — T12; deps qt6/wl-clipboard/portal — T12. All spec sections mapped.
- **Type consistency:** `Backend::Orientation { Horizontal=0, Vertical=1 }`, `cut()`/`applyCut(int,…)`, `ImageProvider::setImage`, `image://omapic/<revision>`, `FilePicker::openImage`/`openSelected` are used identically across tasks. QML `CutBand.orientation` (1=horizontal band, 2=vertical band) is deliberately distinct from the backend enum and converted in `commit()` — noted inline to avoid confusion.
- **Ordering caveat:** Task 4's `applyCutThenUndoRestores` test uses `load()` from Task 5. Run Task 5 before executing Task 4's test (called out in Task 4 Step 1).
```
