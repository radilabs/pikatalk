# PikaTalk

PikaTalk is a native Linux desktop client for PikaClaw.

This repository currently contains **Phase 1 — Local Chat State**.

It is a Qt 6 / Kirigami application that stores projects, chats, messages, workspace/model context, and drafts locally in SQLite. It is not yet connected to PikaClaw.

## Current status

Phase 1 provides:

* a native Plasma 6 window
* selectable project and chat lists
* persisted conversation history (local user and assistant messages)
* project default workspace and model, with per-chat overrides
* per-chat message drafts
* an always-visible context area for project, workspace, model, and gateway
* XDG data, config, and cache paths
* SQLite database `pikatalk.sqlite` (schema version 1)

The gateway indicator is still a static Offline placeholder.

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

Local application paths and the SQLite schema are in:

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
ctest --test-dir build --output-on-failure
./build/bin/pikatalk
```

## Documentation

* [`docs/development.md`](docs/development.md) — build, run, and debug
* [`docs/local-storage.md`](docs/local-storage.md) — XDG paths and SQLite schema version 1
* [`docs/handoffs/phase-1.md`](docs/handoffs/phase-1.md) — Phase 1 completion evidence
* [`decisions/0002-local-sqlite-schema.md`](decisions/0002-local-sqlite-schema.md) — schema decisions later phases must respect

## License

MIT. See `LICENSE`.
