BINARY  := omapic
BUILD   := build/$(BINARY)

.PHONY: build test run install uninstall clean aur-srcinfo aur-publish release help
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

## install: build and install the system Arch package (makepkg -fsi)
install:
	./bin/install

## uninstall: remove the installed omapic package
uninstall:
	sudo pacman -Rns omapic

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
