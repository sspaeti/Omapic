# Changelog

All notable changes to omapic are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added
- Interactive horizontal/vertical **cut-out**: drag a band out of the middle of
  an image and the remaining pieces collapse together.
- **Live collapse preview** while dragging (clip-window technique).
- Image input: `omapic <file>`, `omapic --clipboard`, and an in-app Open
  dialog (`Ctrl+O`) via the xdg desktop portal.
- Output: `Ctrl+C` copies the result to the clipboard (`wl-copy`); `Ctrl+S`
  saves a timestamped PNG to `~/Pictures/Printscreen/<YYYY-MM>/`;
  **`Ctrl+Shift+S` save-as** via a native file dialog.
- `Ctrl+Z` undo, one cut at a time, repeatable.
- Hyprland wrapper `scripts/omapic-capture-cut.sh` to open omapic on the
  most-recent screenshot.
- Packaging: local `pkgbuild/PKGBUILD`, `Makefile`, and an `omapic-bin` AUR
  PKGBUILD.

### Fixed
- Live drag preview showed the full image stretched instead of the actual cut
  region (replaced `Image.sourceClipRect` with a clip-window + offset).
- `make install` failed against a shadowing `install` shell function; it now
  routes through `./bin/install` (path-based).
