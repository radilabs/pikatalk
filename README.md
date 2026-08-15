# PikaTalk

PikaTalk is a native Linux desktop client for PikaClaw.

This repository currently contains **Phase 0 — Native Foundation**.

It is a Qt 6 / Kirigami application that opens a Plasma window with the intended layout, fake/static chat content, XDG application paths, and a SQLite initialization proof. It is not yet a working chat client.

## Current status

Phase 0 provides:

* a native Plasma 6 window
* a project/chat sidebar area
* a conversation area
* a message input area
* an always-visible context area with placeholders for project, workspace, model, and gateway state
* fake/static conversation content
* XDG data, config, and cache paths
* a Phase 0-only SQLite marker database

The visible project, chat, workspace, model, and gateway values are placeholders.

* Real local project/chat state begins in Phase 1.
* Real PikaClaw communication begins in Phase 2.

## Target platform

* Linux
* openSUSE Tumbleweed
* KDE Plasma 6

## Stack

* Qt 6
* Kirigami
* Qt Quick / QML
* SQLite via Qt SQL

## Build and run

Development dependencies, configure/build/launch commands, and debugging notes are in:

* [`docs/development.md`](docs/development.md)

Local application paths are in:

* [`docs/local-storage.md`](docs/local-storage.md)

From the repository root:

```bash
sudo zypper install \
  cmake ninja gcc-c++ \
  kf6-extra-cmake-modules \
  kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel \
  kf6-kiconthemes-devel kf6-qqc2-desktop-style kf6-qqc2-desktop-style-devel \
  qt6-base-devel qt6-declarative-devel qt6-quickcontrols2-devel

cmake -B build -G Ninja --install-prefix "$HOME/.local"
cmake --build build
./build/bin/pikatalk
```

## License

MIT. See `LICENSE`.
