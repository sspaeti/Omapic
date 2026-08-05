BINARY  := omapic
BUILD   := build/$(BINARY)
PREFIX  := $(HOME)/.local
DESKTOP := $(HOME)/.local/share/applications
ICONS   := $(HOME)/.local/share/icons/hicolor/scalable/apps

.PHONY: build test run install uninstall install-pkg clean aur-srcinfo aur-publish release help
.DEFAULT_GOAL := install

## build: compile the omapic binary (qmake6 + make)
build:
	./bin/build

## test: run backend unit tests
test:
	./bin/test

## run: build then run (usage: make run ARGS=path/to/image.png)
run: build
	$(BUILD) $(ARGS)

## install: build and install to ~/.local (binary + desktop entry + icon, no root)
install: build
	install -Dm755 $(BUILD) $(PREFIX)/bin/$(BINARY)
	install -Dm644 pkgbuild/omapic.desktop $(DESKTOP)/omapic.desktop
	install -Dm644 pkgbuild/omapic.svg $(ICONS)/omapic.svg
	@echo "Installed omapic to $(PREFIX)/bin/$(BINARY)"

## uninstall: remove the ~/.local install
uninstall:
	rm -f $(PREFIX)/bin/$(BINARY) $(DESKTOP)/omapic.desktop $(ICONS)/omapic.svg
	@echo "Removed omapic from $(PREFIX)"

## install-pkg: build and install the system Arch package (makepkg -fsi)
install-pkg:
	./bin/install

## clean: remove build artifacts
clean:
	rm -rf build

## aur-srcinfo: regenerate aur/.SRCINFO from aur/PKGBUILD
aur-srcinfo:
	cd aur && makepkg --printsrcinfo > .SRCINFO

## aur-publish: push aur/PKGBUILD + .SRCINFO to the AUR
aur-publish: aur-srcinfo
	./scripts/aur-publish.sh

## release: tag and push a new version (usage: make release VERSION=v0.1.0)
release:
	@test -n "$(VERSION)" || { echo "Usage: make release VERSION=v0.1.0"; exit 1; }
	git tag -a $(VERSION) -m "Release $(VERSION)"
	git push origin $(VERSION)
	@echo "Tagged $(VERSION)."

## help: print this list
help:
	@grep -E '^## ' Makefile | sed 's/^## //'
