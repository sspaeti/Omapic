# omapic

A minimal image **cut-out** tool for **Linux Wayland**. Open an image, drag a
horizontal or vertical band out of the middle, watch the gap collapse live,
press Enter. Snagit's ["cut out"](https://www.techsmith.com/learn/tutorials/snagit/cut-out/), ported to Linux.

Built like [omacut](https://github.com/omacom-io/omacut): Qt Quick (QML) UI +
a C++ backend compiled to a single binary. Cuts use Qt's `QImage` — no ffmpeg,
no ImageMagick at runtime.

## Demo

Short video of a cut in action:

![omapic video](images/omapic-vertical-cut.gif)

Cutting a long image — before and after:

|                     Image cut                      |
| :--------------------------------------------------: |
| ![omapic image](images/omapic-before.png) |
| ![omapic image](images/omapic-after.png) |

## Install

### Requirements

omapic is a **Qt 6 app for Wayland** — it runs on any Wayland session
(Hyprland, Sway, GNOME, KDE, …). Arch only matters for the AUR package; any
distro can build from source. What each feature needs:

- **Display:** a Wayland session. It is not built for X11 (the clipboard and
  file-dialog features are Wayland-only).
- **Clipboard** (`Ctrl+C`, `--clipboard`): `wl-clipboard` (`wl-copy` / `wl-paste`).
- **File dialogs** (`Ctrl+O`, `Ctrl+Shift+S`): `xdg-desktop-portal` **plus a
  backend that implements the FileChooser portal** — `xdg-desktop-portal-gtk`,
  `-kde`, or `-gnome`. Note: on wlroots compositors (Hyprland, Sway) the
  compositor's own portal (`xdg-desktop-portal-hyprland` / `-wlr`) only handles
  screenshots/screencast — **not** file dialogs — so install
  `xdg-desktop-portal-gtk` for those.
- **The cut itself** and **`Ctrl+S`** (save to `~/Pictures/Printscreen/…`) need
  none of the above; they work as long as the app runs.

### AUR (Arch & derivatives)

With an AUR helper:

    yay -S omapic        # or: paru -S omapic

Or manually:

    git clone https://aur.archlinux.org/omapic.git
    cd omapic
    makepkg -si

### Other Linux (build from source)

Install the build + runtime dependencies, then build with `qmake6`.

- **Debian / Ubuntu:**

      sudo apt install build-essential qmake6 qt6-base-dev qt6-declarative-dev \
          qml6-module-qtquick-controls qml6-module-qtquick-templates \
          wl-clipboard xdg-desktop-portal xdg-desktop-portal-gtk

- **Fedora:**

      sudo dnf install gcc-c++ make qt6-qtbase-devel qt6-qtdeclarative-devel \
          wl-clipboard xdg-desktop-portal xdg-desktop-portal-gtk

(Exact QML-module package names vary by distro; you need Qt 6 Base, Qt
Declarative/Qt Quick, and Qt Quick Controls.)

Then build and run:

    git clone https://github.com/sspaeti/Omapic.git
    cd Omapic
    ./bin/build              # -> build/omapic
    ./build/omapic image.png

Install into your user prefix (no root; `make install` is Arch-only):

    install -Dm755 build/omapic ~/.local/bin/omapic
    install -Dm644 pkgbuild/omapic.desktop ~/.local/share/applications/omapic.desktop
    install -Dm644 pkgbuild/omapic.svg ~/.local/share/icons/hicolor/scalable/apps/omapic.svg

Make sure `~/.local/bin` is on your `PATH`.

## Usage

Open an image from a file or the clipboard:

    omapic image.png       # open a file
    omapic --clipboard     # load the current clipboard image

### Keys

- **drag** — create a band (drag direction picks horizontal vs vertical)
- **Enter** — apply the cut
- **Ctrl+Z** — undo
- **Ctrl+C** — copy result to clipboard
- **Ctrl+S** — save to `~/Pictures/Printscreen/<YYYY-MM>/`
- **Ctrl+Shift+S** — save as… (native file dialog)
- **Ctrl+O** — open a file
- **Esc** — clear the current band

### Hyprland keybind

To open the most recent screenshot straight into omapic, add this to your
Hyprland binds (mine live in [bindings.conf](https://github.com/sspaeti/dotfiles/blob/master/hypr/.config/hypr/bindings.conf)):

```sh
bindd = SUPER ALT, C, Cut-out image, exec, ~/git/images/omapic/scripts/omapic-capture-cut.sh
```

## Development

    ./bin/build            # compile -> build/omapic
    ./bin/test             # run unit tests

A root `Makefile` wraps the common workflows (`make help` for all):

    make install                 # build + install the system Arch package (makepkg -fsi)
    make uninstall               # remove the installed package
    make run ARGS=image.png      # build then run on the given image
    make test                    # run unit tests
    make release VERSION=v0.1.0  # tag+push, bump aur/PKGBUILD, publish to the AUR (all in one)
    make aur-bump VERSION=v0.1.0 # (manual) point aur/PKGBUILD at a tag + refresh checksum
    make aur-publish             # (manual) push aur/PKGBUILD + .SRCINFO to the AUR
    make help                    # list all targets

### Cut a release

One command (tags use the form `v0.1.0` — no dot after `v`):

    make release VERSION=v0.1.0

That tags + pushes, waits for GitHub's tag tarball, writes the pkgver + sha256
into `aur/PKGBUILD`, regenerates `.SRCINFO`, and pushes it to the AUR. The first
run creates the `omapic` AUR package; later runs update it. (`make aur-bump` /
`make aur-publish` remain available to run those steps by hand.)

## Not yet

- A custom app icon (currently reuses omacut's)

## Roadmap

See my second brain at [Roadmap](https://www.ssp.sh/brain/omapic#roadmap).
