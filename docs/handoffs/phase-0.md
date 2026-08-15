# Phase 0 Handoff — Native Foundation

## Deliverables

Meaningful files produced:

* `CMakeLists.txt` — Qt 6 / KF6 Kirigami CMake project
* `src/CMakeLists.txt` — executable and QML module `org.radilabs.pikatalk`
* `src/main.cpp` — application entry, XDG path setup, SQLite init, Qt/QML logging
* `src/Main.qml` — Kirigami window with sidebar, conversation, input, and context placeholders
* `src/applicationpaths.h` / `src/applicationpaths.cpp` — XDG path resolution
* `src/database.h` / `src/database.cpp` — Phase 0 SQLite marker database
* `org.radilabs.pikatalk.desktop` — Plasma desktop entry
* `tests/applicationpaths_test.cpp` — XDG path tests
* `tests/database_test.cpp` — SQLite create/write/read/error tests
* `tests/CMakeLists.txt`
* `.gitignore`
* `README.md`
* `docs/development.md`
* `docs/local-storage.md`
* `decisions/0001-application-identity-and-xdg-paths.md`

Built artifact on the development machine:

* `build/bin/pikatalk`

## Tests Performed

* Configured from a clean out-of-source directory: `cmake -B build -G Ninja --install-prefix "$HOME/.local"`
* Built: `cmake --build build`
* Automated: `ctest --test-dir build --output-on-failure` (appstreamtest, applicationpaths_test, database_test)
* Launched `./build/bin/pikatalk` under Plasma 6 Wayland
* Closed and relaunched; KWin listed window `caption=PikaTalk class=org.radilabs.pikatalk`
* Visual layout check at 960x668 and 700x548
* Maximized window measured at 1920x1034 on a 1920x1080 output
* XDG override launch with `XDG_DATA_HOME` / `XDG_CONFIG_HOME` / `XDG_CACHE_HOME` under `/tmp/pikatalk-xdg-t4`
* Removed `phase0.sqlite`, relaunched, confirmed recreate/write/read, relaunched again
* Forced path failure with `XDG_DATA_HOME=/dev/null/not-a-dir` and confirmed a logged error
* Changed a visible QML string, rebuilt, confirmed the new string, restored it
* Introduced a QML property error, confirmed stderr/journal diagnostics, reverted it
* Followed README configure/build/launch commands from the repository root
* Grep of application sources found no PikaClaw network client code

## Results

Phase 0 acceptance criteria from `PHASES.md`:

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | PikaTalk builds successfully on the target openSUSE development machine | PASS | `cmake --build build` on openSUSE Tumbleweed 20260813 |
| 2 | PikaTalk launches as a normal Plasma desktop application | PASS | Kirigami `ApplicationWindow` with Breeze / `org.kde.desktop`; Wayland window class `org.radilabs.pikatalk` |
| 3 | The main window renders correctly | PASS | Window opened and displayed the shell at multiple sizes |
| 4 | The sidebar, conversation area, input area, and context area are visible | PASS | Visual inspection of the running window |
| 5 | Fake/static content demonstrates the intended layout | PASS | Static project/chats, user/assistant messages, context placeholders |
| 6 | SQLite can be initialized and accessed successfully | PASS | `phase0.sqlite` created; marker write/read survived restart; `database_test` passed |
| 7 | Application data uses appropriate local/XDG paths | PASS | `QStandardPaths` under `~/.local/share/Radilabs/PikaTalk` and XDG overrides |
| 8 | Local build/run/debug instructions are documented | PASS | `README.md`, `docs/development.md`, `docs/local-storage.md` |
| 9 | No PikaClaw network communication exists | PASS | No network client code; gateway label is a static placeholder |
| 10 | No later-phase functionality has been implemented | PASS | No real chats, projects, persistence model, streaming, tools, search, tray, notifications, or branching |

## Known Limitations

* Running from `build/bin/pikatalk` without `cmake --install build` logs `xdg-desktop-portal` warning `App info not found for 'org.radilabs.pikatalk'`. The window still opens. Installing the desktop file into `~/.local/share/applications` removes that warning.
* Context placeholders and conversation content are fake/static.
* The SQLite file is `phase0.sqlite` with a `phase0_init` marker table only. It is not the Phase 1 schema.
* Kirigami page title and the context toolbar both occupy vertical space.
* Sidebar entries are labels, not working navigation.
* QML changes require rebuild; there is no live reload.
* CMake/ECM may print `fatal: no upstream configured for branch ...` on a local-only git branch. Configure still succeeds.

## Deferred Work

Copied from `tasks/phase-0-native-foundation.md`:

* Interactive project/chat sidebar and real local persistence — Phase 1
* Custom application icon and AppStream metainfo — Phase 5
* Live QML reload during development — Future

## Decisions

* `decisions/0001-application-identity-and-xdg-paths.md` — organization `Radilabs`, application `PikaTalk`, app ID `org.radilabs.pikatalk`, XDG paths via `QStandardPaths`

## Environment

* openSUSE Tumbleweed 20260813
* Plasma 6.7.4
* Qt 6.11.1
* Kirigami 6.28.0

```text
PHASE 0 HANDOFF: PASS
```
