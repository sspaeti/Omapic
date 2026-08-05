BINARY  := omapic
BUILD   := build/$(BINARY)

.PHONY: build test run install uninstall clean aur-srcinfo aur-bump aur-publish release help
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

## aur-bump: point aur/PKGBUILD at a released tag + refresh checksum (usage: make aur-bump VERSION=v0.1.0)
aur-bump:
	@test -n "$(VERSION)" || { echo "Usage: make aur-bump VERSION=v0.1.0"; exit 1; }
	./scripts/aur-bump.sh $(VERSION)

## aur-publish: push aur/PKGBUILD + .SRCINFO to the AUR
aur-publish: aur-srcinfo
	./scripts/aur-publish.sh

## release: tag+push, create the GitHub release, bump aur/PKGBUILD, publish to AUR (usage: make release VERSION=v0.1.0)
release:
	@test -n "$(VERSION)" || { echo "Usage: make release VERSION=v0.1.0"; exit 1; }
	@echo "$(VERSION)" | grep -Eq '^v[0-9]+\.[0-9]+\.[0-9]+$$' || \
		{ echo "Bad version '$(VERSION)'. Use vMAJOR.MINOR.PATCH, e.g. v0.1.0 (no dot after v)."; exit 1; }
	@if git rev-parse -q --verify "refs/tags/$(VERSION)" >/dev/null; then \
		echo "Tag $(VERSION) already exists — skipping tag/push, continuing to AUR."; \
		git push origin $(VERSION) || true; \
	else \
		git tag -a $(VERSION) -m "Release $(VERSION)"; \
		git push origin HEAD $(VERSION); \
	fi
	@if command -v gh >/dev/null 2>&1; then \
		gh release view $(VERSION) >/dev/null 2>&1 \
			&& echo "GitHub release $(VERSION) already exists — skipping." \
			|| gh release create $(VERSION) --title "$(VERSION)" --generate-notes; \
	else \
		echo "gh CLI not found — skipping GitHub Release (tag/tarball still pushed). Install 'github-cli' to enable."; \
	fi
	./scripts/aur-bump.sh $(VERSION)
	git add aur/PKGBUILD aur/.SRCINFO && git commit -m "aur: $(VERSION)" && git push origin HEAD || true
	./scripts/aur-publish.sh
	@echo "Released $(VERSION) and published to the AUR."

## help: print this list
help:
	@grep -E '^## ' Makefile | sed 's/^## //'
