#!/usr/bin/env bash
# Publish aur/PKGBUILD + aur/.SRCINFO to the AUR. Requires your AUR SSH key
# registered at aur.archlinux.org. Works for the first publish too: cloning an
# unused name yields an empty repo you push to, which creates the package.
set -euo pipefail

PKG=omapic
AUR_URL="ssh://aur@aur.archlinux.org/${PKG}.git"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if ! git clone "$AUR_URL" "$TMP/$PKG" 2>/dev/null; then
    echo "Clone of $AUR_URL failed."
    echo "Register your SSH key at aur.archlinux.org (Account > SSH public key) first."
    exit 1
fi

cp aur/PKGBUILD aur/.SRCINFO "$TMP/$PKG/"
cd "$TMP/$PKG"
git add PKGBUILD .SRCINFO
if git diff --cached --quiet; then
    echo "No changes to publish."
    exit 0
fi
PKGVER="$(sed -n 's/^pkgver=//p' PKGBUILD)"
git commit -m "Update to ${PKGVER}"
git push
echo "Pushed ${PKG} ${PKGVER} to the AUR."
