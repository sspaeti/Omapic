# omapic

A simple image **cut-out** tool for Hyprland/Arch. Open an image, drag a
horizontal or vertical band out of the middle, watch the gap collapse live,
press Enter. Snagit's ["cut out"](https://www.techsmith.com/learn/tutorials/snagit/cut-out/), ported to Linux.

Built like [omacut](https://github.com/omacom-io/omacut): Qt Quick (QML) UI +
a C++ backend compiled to a single binary. Cuts use Qt's `QImage` — no ffmpeg,
no ImageMagick at runtime.


## Video
Short video above cut in action:
![omapic video](images/omapic-vertical-cut.gif) 

### Screenshots

Cutting image by importing a long image - see before and after:
|                     Image cut                      |
| :--------------------------------------------------: |
| ![omapic image](images/omapic-before.png) |
| ![omapic image](images/omapic-after.png) |


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
    make release VERSION=v0.1.0  # tag+push, bump aur/PKGBUILD, publish to the AUR (all in one)
    make aur-bump VERSION=v0.1.0 # (manual) point aur/PKGBUILD at a tag + refresh checksum
    make aur-publish             # (manual) push aur/PKGBUILD + .SRCINFO to the AUR
    make help                    # list all targets

## AUR

The AUR package is **`omapic`** — a *source* package: `makepkg` builds it from
the GitHub tag tarball on the user's machine (qt6 `makedepends`). No CI,
no prebuilt binaries, no GitHub Action — the AUR push happens locally from
`make release`.

**One-time setup**
1. Register your SSH public key at aur.archlinux.org (Account → My Account →
   SSH Public Key). This is required for the AUR push, and can't be skipped.
2. The GitHub repo (`github.com/sspaeti/Omapic`) exists with `origin` set.

**Cut a release — one command** (tags use the form `v0.1.0`; no dot after `v`):

    make release VERSION=v0.1.0

That tags + pushes, waits for GitHub's tag tarball, writes the pkgver + sha256
into `aur/PKGBUILD`, regenerates `.SRCINFO`, and pushes it to the AUR. The first
run creates the `omapic` AUR package; later runs update it. (`make aur-bump` /
`make aur-publish` remain available to run those steps by hand.)

## Not yet
- Multiple simultaneous cut lines (one at a time for now)
- Re-draggable band edge handles after the initial drag
- A custom app icon (currently reuses omacut's)

## Roadmap

See at my second brain at [Roadmap](https://www.ssp.sh/brain/omapic#roadmap).
