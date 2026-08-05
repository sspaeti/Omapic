#!/usr/bin/env bash
# Bump aur/PKGBUILD to a released tag: set pkgver, recompute the source
# tarball checksum from the GitHub tag archive, and regenerate aur/.SRCINFO.
# The tag must already be pushed to GitHub (run `make release` first).
# Usage: scripts/aur-bump.sh v0.1.0
set -euo pipefail

VER="${1:?Usage: aur-bump.sh v0.1.0}"
VER="${VER#v}"                       # accept v0.1.0 or 0.1.0

# Read repo owner/name straight from the PKGBUILD so this stays in sync.
_pkgauthor="$(sed -n 's/^_pkgauthor=//p' aur/PKGBUILD)"
_repo="$(sed -n 's/^_repo=\([^ ]*\).*/\1/p' aur/PKGBUILD)"
TARBALL="https://github.com/${_pkgauthor}/${_repo}/archive/refs/tags/v${VER}.tar.gz"

echo "Fetching ${TARBALL} ..."
SHA="$(curl -fsSL "$TARBALL" | sha256sum | cut -d' ' -f1)"
[ -n "$SHA" ] || { echo "Failed to hash tarball — is tag v${VER} pushed to GitHub?"; exit 1; }

sed -i "s/^pkgver=.*/pkgver=${VER}/" aur/PKGBUILD
sed -i "s/^sha256sums=.*/sha256sums=('${SHA}')/" aur/PKGBUILD
( cd aur && makepkg --printsrcinfo > .SRCINFO )

echo "aur/PKGBUILD -> pkgver=${VER}, sha256=${SHA}"
echo "Review the diff, then: make aur-publish"
