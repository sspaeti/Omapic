#!/usr/bin/env bash
# Bump aur/PKGBUILD to a released tag: set pkgver, recompute the source
# tarball checksum from the GitHub tag archive, and regenerate aur/.SRCINFO.
# The tag must already be pushed to GitHub (run `make release` first).
# Usage: scripts/aur-bump.sh v0.1.0
set -euo pipefail

VER="${1:?Usage: aur-bump.sh v0.1.0}"
case "$VER" in
    v[0-9]*.[0-9]*.[0-9]*) : ;;
    *) echo "Bad version '$VER'. Use vMAJOR.MINOR.PATCH, e.g. v0.1.0 (no dot after v)."; exit 1 ;;
esac
VER="${VER#v}"                       # normalize to bare 0.1.0

# Read repo owner/name straight from the PKGBUILD so this stays in sync.
_pkgauthor="$(sed -n 's/^_pkgauthor=//p' aur/PKGBUILD)"
_repo="$(sed -n 's/^_repo=\([^ ]*\).*/\1/p' aur/PKGBUILD)"
TARBALL="https://github.com/${_pkgauthor}/${_repo}/archive/refs/tags/v${VER}.tar.gz"

# GitHub can take a moment to serve the tarball right after a tag push; retry.
echo "Fetching ${TARBALL} ..."
SHA=""
for attempt in 1 2 3 4 5; do
    if SHA="$(curl -fsSL "$TARBALL" | sha256sum | cut -d' ' -f1)" && [ -n "$SHA" ]; then
        break
    fi
    echo "  not ready yet (attempt ${attempt}/5), retrying in 3s..."
    sleep 3
    SHA=""
done
[ -n "$SHA" ] || { echo "Failed to fetch tarball — is tag v${VER} pushed to GitHub?"; exit 1; }

sed -i "s/^pkgver=.*/pkgver=${VER}/" aur/PKGBUILD
sed -i "s/^sha256sums=.*/sha256sums=('${SHA}')/" aur/PKGBUILD
( cd aur && makepkg --printsrcinfo > .SRCINFO )

echo "aur/PKGBUILD -> pkgver=${VER}, sha256=${SHA}"
echo "Review the diff, then: make aur-publish"
