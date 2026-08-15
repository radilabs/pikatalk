# PikaTalk

PikaTalk is a native Linux desktop client for PikaClaw.

This repository currently contains **Phase 4 — Gateway Management**.

It is a Qt 6 / Kirigami application that stores projects, chats, messages, drafts, workspace/model context, and tool activity locally in SQLite, sends real conversations through the local PicoClaw (PikaClaw) gateway, and can start/stop/restart that local gateway via the PicoClaw launcher.

## Current status

Phase 4 builds on Phase 3 and adds:

* a native Plasma 6 window
* selectable project and chat lists
* persisted conversation history owned by PikaTalk
* project default workspace and model, with per-chat overrides
* model discovery from PicoClaw `config.json`
* real Send / streaming / Stop / Retry-Regenerate through Pico Protocol WebSocket
* gateway communication state in the context area
* drafts preserved when the gateway is unavailable
* persisted PicoClaw tool activity (compact expandable UI; not stored as assistant messages)
* copy message text and fenced code blocks
* open the active workspace in the file manager, terminal, or editor
* local gateway Start / Stop / Restart via PicoClaw launcher HTTP (endpoint + version visible)

PikaTalk does not install/upgrade PicoClaw and does not call model providers directly.

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
* [`docs/local-storage.md`](docs/local-storage.md) — XDG paths and SQLite schema (v2 includes tool activity)
* [`docs/pikaclaw-api.md`](docs/pikaclaw-api.md) — observed PicoClaw chat and tool API
* [`docs/handoffs/phase-1.md`](docs/handoffs/phase-1.md) — Phase 1 completion evidence
* [`docs/handoffs/phase-2.md`](docs/handoffs/phase-2.md) — Phase 2 completion evidence
* [`docs/handoffs/phase-3.md`](docs/handoffs/phase-3.md) — Phase 3 completion evidence
* [`docs/handoffs/phase-4.md`](docs/handoffs/phase-4.md) — Phase 4 completion evidence
* [`decisions/0002-local-sqlite-schema.md`](decisions/0002-local-sqlite-schema.md) — schema decisions later phases must respect
* [`decisions/0003-pico-protocol-chat-transport.md`](decisions/0003-pico-protocol-chat-transport.md) — chat transport decision
* [`decisions/0004-tool-activity-persistence.md`](decisions/0004-tool-activity-persistence.md) — tool activity persistence decision
* [`decisions/0005-picoclaw-launcher-lifecycle.md`](decisions/0005-picoclaw-launcher-lifecycle.md) — local gateway lifecycle decision

## License

MIT. See `LICENSE`.
