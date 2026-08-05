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

## Makefile

A root `Makefile` wraps the common workflows (run `make help` for the full list):

    make install               # build + install to ~/.local (binary, desktop entry, icon — no root)
    make install-pkg           # build + install the system Arch package (makepkg -fsi)
    make run ARGS=image.png    # build then run on the given image
    make test                  # run backend unit tests
    make release VERSION=v0.1.0  # tag + push a release
    make aur-publish           # push aur/PKGBUILD + .SRCINFO to the AUR
    make help                  # list all targets

## AUR

The AUR package is **`omapic-git`** (a VCS package that builds from source).
`aur/PKGBUILD` and `aur/.SRCINFO` are published with `make aur-publish`
(requires your AUR SSH key registered and the package to already exist on the AUR).

> **Note:** The GitHub URL `github.com/ssp-data/omapic` and the AUR package name
> `omapic-git` are assumptions — the maintainer should confirm/adjust these before
> publishing.

## Not yet
- Multiple simultaneous cut lines (one at a time for now)
- Re-draggable band edge handles after the initial drag
- A custom app icon (currently reuses omacut's)

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for what's new.

## Roadmap

See at my second brain at [Roadmap](https://www.ssp.sh/brain/omapic#roadmap).
