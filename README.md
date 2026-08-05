# omapic

A simple image **cut-out** tool for Hyprland/Arch. Open an image, drag a
horizontal or vertical band out of the middle, watch the gap collapse live,
press Enter. Snagit's ["cut out"](https://www.techsmith.com/learn/tutorials/snagit/cut-out/), ported to Linux.

Built like [omacut](https://github.com/omacom-io/omacut): Qt Quick (QML) UI +
a C++ backend compiled to a single binary. Cuts use Qt's `QImage` — no ffmpeg,
no ImageMagick at runtime.


## Screenshots

|                     Image Selection                      |                     Editing                      |
| :--------------------------------------------------: | :------------------------------------------------------: |
| ![omapic image selection]() | ![omapic editing]() |
|                     Filters                      |                     Saving                      |
| ![edit filters]() | ![omapic saving]() |


## Keys
- **drag** — create a band (drag direction picks horizontal vs vertical)
- **Enter** — apply the cut
- **Ctrl+Z** — undo
- **Ctrl+C** — copy result to clipboard
- **Ctrl+S** — save to `~/Pictures/Printscreen/<YYYY-MM>/`
- **Ctrl+Shift+S** — save as… (native file dialog)
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

## Makefile

A root `Makefile` wraps the common workflows (run `make help` for the full list):

    make install                 # build + install the system Arch package (makepkg -fsi)
    make uninstall               # remove the installed package
    make run ARGS=image.png      # build then run on the given image
    make test                    # run backend unit tests
    make release VERSION=v0.1.0  # tag + push a release
    make aur-bump VERSION=v0.1.0 # point aur/PKGBUILD at the tag + refresh checksum
    make aur-publish             # push aur/PKGBUILD + .SRCINFO to the AUR
    make help                    # list all targets

## AUR

The AUR package is **`omapic`** — a *source* package: `makepkg` builds it from
the GitHub tag tarball on the user's machine (qt6 `makedepends`). No CI,
no prebuilt binaries.

**One-time setup**
1. Register your SSH public key at aur.archlinux.org (Account → My Account).
2. The GitHub repo (`github.com/sspaeti/Omapic`) exists with `origin` set.

**Each release** (tags use the form `v0.1.0` — note: no dot after `v`):

    make release VERSION=v0.1.0    # git tag + push; GitHub then serves the source tarball for the tag
    make aur-bump VERSION=v0.1.0   # set pkgver + sha256 in aur/PKGBUILD, regen .SRCINFO
    git add aur/ && git commit -m "aur: v0.1.0" && git push   # keep the repo copy in sync (optional)
    make aur-publish               # clone the AUR repo, copy PKGBUILD + .SRCINFO, commit, push

The first `make aur-publish` creates the `omapic` package on the AUR (pushing to
the empty repo). Subsequent runs just update it.

## Not yet
- Multiple simultaneous cut lines (one at a time for now)
- Re-draggable band edge handles after the initial drag
- A custom app icon (currently reuses omacut's)

## Roadmap

See at my second brain at [Roadmap](https://www.ssp.sh/brain/omapic#roadmap).
