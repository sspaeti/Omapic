# omapic — design

A dead-simple image **cut-out** tool for Hyprland/Arch. Open an image, drag a
horizontal or vertical band anywhere in the middle, watch the gap collapse in
real time, press Enter to apply. Snagit's "cut out" feature, ported to Linux.

Modeled structurally on [omacut](https://github.com/omacom-io/omacut) (the
omarchy video length trimmer): Qt Quick (QML) UI + C++ backend compiled to a
single binary with the QML embedded via Qt resources.

## Goal & scope

**In scope (v1):**
- Load an image from a file (CLI arg or file dialog) or the clipboard.
- Interactively cut a single horizontal **or** vertical band out of the middle
  of the image, collapsing the remaining pieces together.
- Live preview: the gap closes in real time while dragging.
- One cut at a time, repeatable on the result; `Ctrl+Z` undo.
- Copy result to clipboard (`Ctrl+C`); save result to disk (`Ctrl+S`).
- A Hyprland keybind to open omapic on the most-recent screenshot.

**Out of scope (v1):**
- Multiple simultaneous cut lines applied together (Snagit allows several).
- Annotations / drawing / text (Satty covers that).
- Any non-cut editing (crop-to-rectangle, resize, filters).
- Deep integration inside Satty (launching omapic from a Satty keybind — not a
  native Satty feature; revisit only if we ever fork Satty).

## Architecture

Reuse omacut's stack exactly, stripped of everything video-specific.

- **UI:** Qt Quick / QML, Material style.
- **Backend:** C++ `Backend` (`QObject`) exposed to QML as a context property.
- **Cut engine:** Qt's built-in `QImage` — no ffmpeg, no ImageMagick at
  runtime. Loading, cropping, compositing, and PNG/JPEG encode/decode are all
  native `QImage`/`QImageWriter`.
- **Single binary:** QML embedded via `resources.qrc`, built with `qmake6`.

Qt modules (from omacut, minus `multimedia`): `core gui qml quick
quickcontrols2 dbus`.

### File layout

```
omapic.pro                 # adapted from omacut.pro (video bits removed)
src/main.cpp               # near-identical to omacut's; sets app_id "omapic"
src/backend.h / backend.cpp# QImage load / cut / undo / copy / save (slim)
src/portalfilepicker.*     # reused as-is: native open/save via xdg portal
src/filepicker.h           # reused interface
src/Main.qml               # window, image canvas, keybindings, status
src/CutBand.qml            # adapted from TrimBar.qml: draggable band + handles
src/resources.qrc          # embeds Main.qml, CutBand.qml
pkgbuild/PKGBUILD          # local Arch package `omapic`
pkgbuild/omapic.desktop    # app_id omapic, icon
pkgbuild/omapic.install
pkgbuild/omapic.svg        # icon
bin/build / bin/install / bin/test   # reused from omacut
tests/backend_tests.cpp    # cut-math unit tests
tests/backend_tests.pro
```

Deleted from the omacut template: `ffmpeg.*`, `thumbworker.*`,
`thumbprovider.*`, `Format.js`, and all thumbnail/duration/playhead logic.

## The cut engine (Backend)

The core of the tool. Pure, deterministic `QImage` math — the part that gets
unit tests.

State:
- `QImage m_image` — the current image.
- `QUrl m_source` — where it was loaded from (may be empty for clipboard input).
- `QList<QImage> m_undo` — undo stack (previous images).

Key operations (all `Q_INVOKABLE` where QML needs them):

- `bool load(const QUrl &url)` — decode into `m_image`, clear undo stack, emit
  `imageChanged`.
- `bool loadClipboard()` — read an image from the clipboard (`wl-paste` or
  `QClipboard::image()`); same post-conditions as `load`.
- `void applyCut(Orientation o, int start, int end)` — commit a cut:
  - Horizontal: result height `= H - (end - start)`; blit rows `[0, start)`
    to the top, then rows `[end, H)` immediately below. Width unchanged.
  - Vertical: the same on the width axis; height unchanged.
  - Push the prior `m_image` onto `m_undo`, replace `m_image`, emit
    `imageChanged`.
  - `start`/`end` are in **image pixel coordinates** (QML converts from
    on-screen coordinates using the current scale factor).
- `void undo()` — pop `m_undo` into `m_image`; emit `imageChanged`.
- `void copyToClipboard()` — encode PNG, put on clipboard. Shell to `wl-copy`
  so the image survives after omapic exits (Wayland clipboard ownership).
- `void save()` — write a new timestamped PNG (see Output).

Exposing the image to QML: a small `QQuickImageProvider` (id `img`) serves the
current `m_image`; a `Q_PROPERTY int revision` bumps on every `imageChanged` so
QML `Image` elements reload. (Same pattern as omacut's `ThumbProvider`, far
simpler — one image, no async workers.)

## Interaction (Main.qml + CutBand.qml)

The image is displayed scaled to fit the window (letterboxed). A single scale
factor maps screen ↔ image pixels.

**Creating a band.** Drag anywhere on the image. Orientation auto-selects from
the dominant drag axis:
- Drag mostly **vertical** → a **horizontal** band spanning the full width
  (cuts height).
- Drag mostly **horizontal** → a **vertical** band spanning the full height
  (cuts width).

The band renders as a dimmed region with dashed seam edges and a draggable
handle on each edge (adapted from omacut's `TrimBar.qml` two-handle pattern).

**Live collapse-while-dragging.** The canvas is composed of **two clipped
`Image` views** of the same source, plus the band overlay. As you drag a
handle, the "below/right" segment slides toward the "above/left" segment in
real time, so the gap visibly closes before you commit. This is a pure QML view
transform — no per-frame pixel re-encoding. The backend commits the real
`QImage` cut only on **Enter**, then the two views reset to the new collapsed
image.

**Keybindings:**

| Key | Action |
| --- | --- |
| drag | create / resize the band (orientation auto) |
| Enter | apply the cut (commit; collapses for real) |
| Ctrl+Z | undo last cut |
| Ctrl+C | copy current image to clipboard |
| Ctrl+S | save current image to disk |
| Ctrl+O | open file dialog |
| Esc | clear the current (uncommitted) band |

## Input / output

**Input:**
- `omapic <file>` — open a file passed on the command line (like omacut).
- `omapic --clipboard` — load the current clipboard image (screenshot workflow).
- File-open dialog (`Ctrl+O`) via `portalfilepicker` — fallback.

**Output:**
- **Ctrl+C** — copy the current (possibly-cut) image to the clipboard as PNG,
  via `wl-copy` for post-exit persistence.
- **Ctrl+S** — **always write a new file**, never touching the original:
  `~/Pictures/Printscreen/$(date +%Y-%m)/screenshot-<timestamp>.png`
  (mirrors the existing monthly-folder scheme from
  `auto-organize-screenshot.sh`). The monthly directory is created if missing.
  The Pictures root respects `XDG_PICTURES_DIR` when set.

## Workflow integration

The existing pipeline: `SUPER ALT P` → capture wrapper → Satty (edits the
screenshot in place, Enter = clipboard) → `auto-organize-screenshot.sh` moves
it to `~/Pictures/Printscreen/YYYY-MM/`.

omapic adds one Hyprland keybind, e.g. `SUPER ALT C`, bound to a small wrapper
that opens omapic on the most-recent screenshot (the same file Satty saved).
From there: cut → `Ctrl+C` (clipboard) and/or `Ctrl+S` (monthly folder).

Launching omapic from *inside* Satty via a keybind is not a native Satty
capability and is explicitly out of scope for v1; the global keybind achieves
the same "one keystroke to cut" goal without patching Satty.

## Testing

`tests/backend_tests.cpp` (reusing omacut's `bin/test` + `.pro` harness) covers
the cut engine on synthetic images:

- Horizontal cut reduces height by exactly `(end - start)`; width unchanged.
- Vertical cut reduces width by exactly `(end - start)`; height unchanged.
- Pixels above/left of the band are unchanged; pixels below/right map to the
  correct source rows/columns after collapse (row/column identity check).
- Edge bands (band touching top/bottom/left/right) behave (degenerates to a
  plain crop).
- `undo()` restores the exact prior image (pixel-equal).
- Repeated cuts compose correctly (cut, then cut the result).

The GUI (drag, orientation auto-select, live collapse) is verified manually.

## Dependencies

- **Build:** `qt6-base`, `qt6-declarative` (Qt Quick + Controls), a C++17
  compiler, `qmake6`.
- **Runtime:** `xdg-desktop-portal` (+ a portal backend, for file dialogs),
  `wl-clipboard` (`wl-copy` / `wl-paste`).
- **Not needed:** ffmpeg, ImageMagick.

## Packaging

Local Arch package `omapic` via `pkgbuild/PKGBUILD` and `./bin/install`
(reused from omacut). `omapic.desktop` sets the Wayland `app_id` to `omapic`
so the compositor and taskbar pick up the installed icon.
