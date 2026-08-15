# PikaTalk

PikaTalk is a native KDE Plasma desktop client for a **local PicoClaw (PikaClaw) gateway**. It keeps projects, chats, drafts, and tool activity on this computer and sends conversations through PicoClaw. PicoClaw remains the execution layer; PikaTalk does not talk to model providers itself.

**Version:** 1.0.0 (from `project(pikatalk VERSION …)` in `CMakeLists.txt`; also shown in **Help → About PikaTalk**).

## What v1 can do

* Organize work into projects and chats (create, rename, archive, delete)
* Persist conversation history, drafts, workspace/model defaults, and tool activity in local SQLite
* Filter the sidebar by **project name and chat title** (not message text)
* Send, stream, stop, retry/regenerate; copy message text and fenced code
* Show the active project, workspace, model, and gateway state
* Open the active workspace in the file manager, terminal, or editor
* Start, stop, and restart the local PicoClaw gateway when the PicoClaw launcher is available
* Keyboard shortcuts: **Ctrl+N** new chat, **Ctrl+F** title filter, **Ctrl+L** message input, **Escape** stop generation while a reply is in progress

## What PikaTalk is not

PikaTalk is not a general AI frontend, a PicoClaw installer, or a remote-gateway admin tool.

v1 does **not** include:

* system tray integration
* desktop notifications
* full-text search of message bodies
* Flatpak, Snap, AppImage, RPM release, or Windows/macOS builds
* cloud sync, multi-user accounts, or talking to providers without PicoClaw

## Supported platform

v1 is built and documented for:

* Linux
* openSUSE Tumbleweed
* KDE Plasma 6

Other distributions and operating systems are not a supported v1 target.

## PicoClaw requirement

You need a local PicoClaw installation (the `picoclaw` gateway and `picoclaw-launcher`). PikaTalk does not install or upgrade PicoClaw.

Default local ports (do not treat these as configurable “any host” endpoints):

| Role | Default |
| --- | --- |
| Chat gateway (Pico Protocol WebSocket) | `ws://127.0.0.1:18790/pico/ws` |
| Launcher (start/stop/restart) | `http://127.0.0.1:18800` |

PikaTalk reads the Pico channel token from PicoClaw’s same-user secrets when the token setting is empty (`~/.picoclaw/.security.yml`). Model names come from PicoClaw `~/.picoclaw/config.json`.

Start/Stop/Restart need the launcher dashboard password in local config (see below). Chat still uses the gateway on port **18790**; the launcher on **18800** is not the chat socket.

## Build dependencies

On openSUSE Tumbleweed:

```bash
sudo zypper install \
  cmake ninja gcc-c++ \
  kf6-extra-cmake-modules \
  kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel \
  kf6-kiconthemes-devel kf6-qqc2-desktop-style kf6-qqc2-desktop-style-devel \
  qt6-base-devel qt6-declarative-devel qt6-quickcontrols2-devel \
  qt6-network-devel
```

`qt6-sql-devel` and the SQLite driver are pulled in by `qt6-base-devel` on Tumbleweed.

## Build

From the repository root. Always pass the install prefix at configure time (a build directory configured without it would install to `/usr`):

```bash
cmake -B build -G Ninja --install-prefix "$HOME/.local"
cmake --build build
```

The uninstalled binary is `./build/bin/pikatalk`. You can run that for development without installing.

Optional checks:

```bash
ctest --test-dir build --output-on-failure
```

## Local install

```bash
cmake -B build -G Ninja --install-prefix "$HOME/.local"
cmake --build build
cmake --install build
```

This installs:

* `$HOME/.local/bin/pikatalk`
* `$HOME/.local/share/applications/org.radilabs.pikatalk.desktop`
* `$HOME/.local/share/metainfo/org.radilabs.pikatalk.metainfo.xml`
* `$HOME/.local/share/icons/hicolor/scalable/apps/org.radilabs.pikatalk.svg`

Ensure `$HOME/.local/bin` is on your `PATH` (a normal Plasma session usually has it). Then launch **PikaTalk** from the application menu, or run `pikatalk`.

If the menu entry or icon does not appear immediately:

```bash
update-desktop-database "$HOME/.local/share/applications"
kbuildsycoca6 --noincremental
```

## First run

1. Install PicoClaw so `picoclaw` and `picoclaw-launcher` exist (typically `/usr/bin`).
2. Start PikaTalk (`pikatalk` or the desktop entry).
3. Create a project, then a chat. Set a workspace directory if you want agent tools to run somewhere other than PicoClaw’s default.
4. For chat: the gateway must be listening on **18790**. You can start it from PikaTalk when the launcher on **18800** is running, or start PicoClaw yourself (`picoclaw gateway` / your usual launcher).
5. For Start/Stop/Restart from PikaTalk, put the launcher dashboard password in `~/.config/Radilabs/PikaTalk/pikatalk.conf` (`picoClaw/launcherPassword`). Do not commit that file.
6. Send a message. Version is under **Help → About PikaTalk**.

If the gateway is stopped or unreachable, drafts and local history stay on disk.

## Where local data and config live

PikaTalk uses XDG paths (organization `Radilabs`, application `PikaTalk`). It does not create `~/.pikatalk`.

| Kind | Default path |
| --- | --- |
| Data (SQLite) | `~/.local/share/Radilabs/PikaTalk/pikatalk.sqlite` |
| Config | `~/.config/Radilabs/PikaTalk/pikatalk.conf` |
| Cache | `~/.cache/Radilabs/PikaTalk` |

`XDG_DATA_HOME`, `XDG_CONFIG_HOME`, and `XDG_CACHE_HOME` override these. Schema and config keys are documented in [`docs/local-storage.md`](docs/local-storage.md).

## Known limitations

* Supported only on openSUSE Tumbleweed + Plasma 6 as above.
* Requires a local PicoClaw; PikaTalk will not fetch models or install the gateway for you.
* Sidebar filter matches titles only. There is no full-text message search.
* No tray icon and no notifications.
* PicoClaw’s own session logs are not PikaTalk history; PikaTalk stores conversations locally.
* Gateway Start/Stop/Restart need `picoclaw-launcher` on **18800** and the dashboard password; they do not use systemd.
* There is no packaged distro update channel in v1.

## Developer documentation

Build/debug cycle, tests, and **launcher hygiene** (do not leave a temporary process on port 18800):

* [`docs/development.md`](docs/development.md)

Also:

* [`docs/local-storage.md`](docs/local-storage.md) — XDG paths and SQLite schema
* [`docs/pikaclaw-api.md`](docs/pikaclaw-api.md) — observed PicoClaw chat and launcher API
* [`docs/handoffs/`](docs/handoffs/) — per-phase completion records
* [`decisions/`](decisions/) — architectural decisions later work must respect

## License

MIT. See `LICENSE`.
