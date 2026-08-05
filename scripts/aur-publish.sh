#!/usr/bin/env bash
# Publish aur/PKGBUILD + aur/.SRCINFO to the AUR. Requires your AUR SSH key
# registered at aur.archlinux.org (Account > SSH Public Key).
#
# A brand-new AUR package can't be cloned (it doesn't exist until the first
# push), so this inits a local repo, pulls existing history if the package is
# already there, then pushes — creating it on the first run, updating it after.
set -euo pipefail

PKG=omapic
AUR_URL="ssh://aur@aur.archlinux.org/${PKG}.git"

[ -f aur/PKGBUILD ] && [ -f aur/.SRCINFO ] || {
    echo "Run from the repo root (need aur/PKGBUILD and aur/.SRCINFO)."; exit 1;
}
PKGVER="$(sed -n 's/^pkgver=//p' aur/PKGBUILD)"

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

git -C "$TMP" init -q -b master
git -C "$TMP" remote add origin "$AUR_URL"

# Pull existing package history if it exists; a brand-new package has none, so
# ignore a failed fetch and start fresh (the push then creates the package).
if git -C "$TMP" fetch -q --depth=1 origin master 2>/dev/null; then
    git -C "$TMP" reset -q --hard FETCH_HEAD
fi

cp aur/PKGBUILD aur/.SRCINFO "$TMP/"
git -C "$TMP" add PKGBUILD .SRCINFO
if git -C "$TMP" diff --cached --quiet; then
    echo "No changes to publish (AUR already at ${PKGVER})."
    exit 0
fi

git -C "$TMP" commit -q -m "Update to ${PKGVER}"
git -C "$TMP" push origin HEAD:master
echo "Pushed ${PKG} ${PKGVER} to the AUR."
