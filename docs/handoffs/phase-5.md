# Phase 5 Handoff — Daily-Use Polish / First Release

## Deliverables

Meaningful Phase 5 files and artifacts:

* `CMakeLists.txt` — application version `0.1.0` (`project(pikatalk VERSION 0.1.0 …)`), passed as `PIKATALK_VERSION`
* `src/main.cpp` — `QCoreApplication::setApplicationVersion(PIKATALK_VERSION)`
* `src/AboutPikaTalk.qml` — Help → About PikaTalk (`Kirigami.AboutPage`, `Qt.application.version`)
* `src/titlefilter.h` / `src/titlefilter.cpp` — in-memory case-insensitive project-name / chat-title filter
* `src/errorcopy.h` / `src/errorcopy.cpp` — display-only sanitizer for gateway/request errors
* `src/Main.qml` — title filter field; Ctrl+N / Ctrl+F / Ctrl+L / Escape-while-generating shortcuts; empty-state placeholders
* `org.radilabs.pikatalk.desktop`, `org.radilabs.pikatalk.metainfo.xml`, `icons/org.radilabs.pikatalk.svg` — local Plasma install artifacts
* Tests: `applicationversion_test`, `titlefilter_test`, `shortcuts_test`, `errorcopy_test`, `uistates_test`, `longchat_test`, `packaging_test`
* `README.md` — first-release user documentation
* `docs/development.md`, `docs/local-storage.md`, `docs/pikaclaw-api.md` — Phase 5-aligned developer notes
* `tasks/phase-5-daily-use-polish.md` — execution evidence
* `docs/handoffs/phase-5.md` — this report

Built / installed on the development machine:

* `build/bin/pikatalk`
* `$HOME/.local/bin/pikatalk` plus desktop entry, AppStream metainfo, and hicolor SVG

No tray feature was introduced. No notifications were introduced. No full-text message search was introduced.

## Selected Scope

Phase 5 selected:

* title filtering
* keyboard shortcuts
* state/reliability cleanup
* versioning
* local packaging/install
* release docs

Phase 5 did **not** select:

* full-text message search
* tray
* notifications

Those optional items in `PHASES.md` remain deferred. Application sources under `src/` contain no `SystemTray`, `QSystemTray`, `KNotification`, notification, tray, FTS, or message-body search implementation.

## Tests Performed

* Fresh `ctest --test-dir build --output-on-failure` (this handoff session) — **13/13 PASS** in 5.53s: `appstreamtest`, `applicationpaths_test`, `applicationversion_test`, `titlefilter_test`, `shortcuts_test`, `errorcopy_test`, `longchat_test`, `packaging_test`, `uistates_test`, `database_test`, `appcontroller_test`, `pikaclawlifecycle_test`, `pikaclawclient_test`
* P5-T1–T7 task evidence: version About UI, title filter (project + chat, clear, no persistence mutation), shortcuts unit tests, empty/error placeholders + sanitizer, 120-message long-chat ListView/restart/drafts (no product-code change), `cmake --install` into `$HOME/.local` on openSUSE Tumbleweed + Plasma 6, `desktop-file-validate` / `appstreamcli validate --no-net`, `gtk-launch org.radilabs.pikatalk`
* P5-T8 live (existing user `picoclaw-launcher` PID 71127 / gateway PID 71155; no temp launcher spawned):
  * `PIKATALK_LIVE_GATEWAY=1 … liveGatewaySendIfEnabled` — PASS (`LIVEOK`)
  * `PIKATALK_LIVE_GATEWAY=1 … liveGatewayToolActivityIfEnabled` — PASS (`TOOLOK`)
  * `PIKATALK_LIVE_DESKTOP=1 … openWorkspaceActionsLaunchAgainstRealDirectory` — PASS
  * Installed offscreen `gtk-launch org.radilabs.pikatalk` — started `$HOME/.local/bin/pikatalk`
  * `PIKATALK_LIVE_GATEWAY=1 … liveGatewayLifecycleIfEnabled` — **not re-run** in P5-T8: no `launcherPassword` in `~/.config/Radilabs/PikaTalk/pikatalk.conf`. Fake-launcher `gatewayLifecycleControlsPreserveLocalState` still PASS via `ctest`. Phase 4 handoff already recorded live stop → start → restart → reconnect **PASS**. User launcher/gateway were left running.

## Results

Phase 5 acceptance criteria from `PHASES.md`:

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Normal project/chat navigation is comfortable for daily use | PASS | Title filter, empty-state placeholders, existing project/chat CRUD; long-chat switch/restore in `longchat_test` |
| 2 | Chat/project title search works | PASS | `titlefilter_test` 10/10; in-memory name/title substring only; message bodies not searched |
| 3 | Important keyboard operations work reliably | PASS | `shortcuts_test`: Ctrl+N new chat (project required), Ctrl+F filter, Ctrl+L input, Escape stops only while generating |
| 4 | Long conversations remain reasonably usable | PASS | `longchat_test`: 120 messages + code + tools; scroll/copy/switch/restart; existing Qt `ListView` adequate; no virtualization rewrite |
| 5 | Gateway and request errors remain understandable | PASS | `errorcopy_test` + `uistates_test`; JSON/socket noise sanitized at display time; connecting/stopped/error labels + existing Start/Retry |
| 6 | Application restart restores expected local state | PASS | `longchat_test` reopen restores selected chat, history, tools, drafts; Phase 1–4 persistence tests still in `ctest` |
| 7 | Any notifications/tray features introduced are useful and non-annoying | **PASS — no notifications or tray features were introduced in the selected Phase 5 scope.** | |
| 8 | Installation documentation works from a clean environment | PASS | P5-T6 install to `$HOME/.local`; P5-T7 README commands verified against the repo; `packaging_test` + AppStream/desktop validation |
| 9 | Versioning is defined | PASS | Single source `0.1.0` in root CMake; `applicationversion_test`; About page |
| 10 | Core functionality from Phases 0–4 remains intact | PASS | Fresh `ctest` 13/13; live chat + tool activity; fake-launcher lifecycle; Phase 4 live lifecycle already PASS. P5-T8 did not re-execute live stop/start/restart (no local launcher password); that is an environment gap, not a product regression. |
| 11 | No excluded platform features have slipped into scope | PASS | No tray, notifications, full-text search, Flatpak/Snap/AppImage, branching, cloud sync, RAG, telemetry, or other-OS packaging in product code |

## Known Limitations

* v1 is documented only for Linux / openSUSE Tumbleweed / KDE Plasma 6.
* Requires a local PicoClaw gateway and launcher; PikaTalk does not install or upgrade PicoClaw.
* Sidebar filter is title/name substring only. There is no full-text message search.
* No tray icon and no notifications.
* Gateway Start/Stop/Restart need `picoclaw-launcher` on port 18800 and `launcherPassword` (or `PIKATALK_LAUNCHER_PASSWORD`). Chat can still work if the gateway is already up on 18790.
* This session did not re-run live gateway stop/start/restart (no password in local PikaTalk config). Automated fake-launcher lifecycle still passes. Phase 4 recorded live lifecycle PASS.
* No RPM/Flatpak/Snap/AppImage and no automatic update channel.
* PicoClaw session logs are not PikaTalk history; conversations live in local SQLite.
* Long-chat verification used 120 messages; no custom virtualization was added because ListView remained adequate at that size.

## Deferred Work

From `tasks/phase-5-daily-use-polish.md` (not implemented):

* full-text message search → Future
* tray integration → Future
* notifications → Future
* conversation branching/forking → Future
* richer project instructions/context → Future
* additional Plasma integration → Future
* deeper AppController/UI structural cleanup → only if later feature pressure justifies it
* PicoClaw per-request workspace execution → gateway capability / Future

Phase 6 was not created. Future Phase Candidates in `PHASES.md` were not implemented.

## Decisions

No new ADR for Phase 5. Existing decisions still apply:

* `decisions/0001-application-identity-and-xdg-paths.md` — identity and XDG paths unchanged by packaging
* `decisions/0002-local-sqlite-schema.md` — no search/FTS schema change
* `decisions/0003-pico-protocol-chat-transport.md` — chat transport unchanged
* `decisions/0004-tool-activity-persistence.md` — tool activity ownership unchanged
* `decisions/0005-picoclaw-launcher-lifecycle.md` — launcher HTTP remains the only lifecycle API

## Release Readiness

The repository state constitutes a **usable first PikaTalk release** (version **0.1.0**) for a local openSUSE Tumbleweed + Plasma 6 user with PicoClaw installed: projects/chats persist, title filter and shortcuts work, empty/error states are readable, long chats reload, the app installs as a normal Plasma application, and documentation matches the tree.

P5-T8’s live lifecycle item was not repeated here; core Phase 4 lifecycle remains evidenced by that phase’s live PASS plus current fake-launcher tests and live chat.

```text
PHASE 5 HANDOFF: PASS
```
