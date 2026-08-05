#!/usr/bin/env bash
# Publish aur/PKGBUILD + aur/.SRCINFO to the AUR. Requires your AUR SSH key
# registered and the `omapic-bin` package to exist on the AUR.
set -euo pipefail

PKG=omapic-bin
AUR_URL="ssh://aur@aur.archlinux.org/${PKG}.git"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if ! git clone "$AUR_URL" "$TMP/$PKG" 2>/dev/null; then
    echo "Clone of $AUR_URL failed."
    echo "Ensure the AUR package '$PKG' exists and your SSH key is registered at aur.archlinux.org."
    exit 1
fi

cp aur/PKGBUILD aur/.SRCINFO "$TMP/$PKG/"
cd "$TMP/$PKG"
git add PKGBUILD .SRCINFO
if git diff --cached --quiet; then
    echo "No changes to publish."
    exit 0
fi
git commit -m "Update ${PKG}"
git push
echo "Pushed ${PKG} to the AUR."
