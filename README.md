# PikaTalk

PikaTalk is a native Linux desktop client for PikaClaw.

This repository currently contains **Phase 2 — PikaClaw Chat**.

It is a Qt 6 / Kirigami application that stores projects, chats, messages, workspace/model context, and drafts locally in SQLite, and sends real conversations through the local PicoClaw (PikaClaw) gateway.

## Current status

Phase 2 provides:

* a native Plasma 6 window
* selectable project and chat lists
* persisted conversation history owned by PikaTalk
* project default workspace and model, with per-chat overrides
* model discovery from PicoClaw `config.json`
* real Send / streaming / Stop / Retry-Regenerate through Pico Protocol WebSocket
* gateway communication state in the context area
* drafts preserved when the gateway is unavailable

PikaTalk does not start/stop the gateway and does not call model providers directly.

## Target platform

* Linux
* openSUSE Tumbleweed
* KDE Plasma 6

## Stack

* Qt 6
* Kirigami
* Qt Quick / QML
* SQLite via Qt SQL
* Qt Network (Pico Protocol WebSocket)

## Build and run

Development dependencies, configure/build/launch commands, and debugging notes are in:

* [`docs/development.md`](docs/development.md)

Local application paths and the SQLite schema are in:

* [`docs/local-storage.md`](docs/local-storage.md)

PikaClaw/PicoClaw chat API behavior:

* [`docs/pikaclaw-api.md`](docs/pikaclaw-api.md)

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
* [`docs/pikaclaw-api.md`](docs/pikaclaw-api.md) — observed PicoClaw chat API
* [`docs/handoffs/phase-1.md`](docs/handoffs/phase-1.md) — Phase 1 completion evidence
* [`docs/handoffs/phase-2.md`](docs/handoffs/phase-2.md) — Phase 2 completion evidence
* [`decisions/0002-local-sqlite-schema.md`](decisions/0002-local-sqlite-schema.md) — schema decisions later phases must respect
* [`decisions/0003-pico-protocol-chat-transport.md`](decisions/0003-pico-protocol-chat-transport.md) — chat transport decision

## License

MIT. See `LICENSE`.
