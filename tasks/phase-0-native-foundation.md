# Phase 0 Tasks — Native Foundation

Phase contract: `PHASES.md` → **Phase 0 — Native Foundation**

This file contains only implementation work for Phase 0.

## Execution Rules

1. Read `PROJECT.md`.
2. Read the Phase 0 contract in `PHASES.md`.
3. Work on tasks in order.
4. Work on only one task at a time.
5. Do not implement anything listed under Phase 0 exclusions.
6. Do not begin work from another phase.
7. If new work is discovered outside Phase 0, record it under **Deferred Work**.
8. Mark a task complete only after its test procedure passes.
9. Record completion evidence under the task.
10. Completing all tasks does **not** automatically complete Phase 0.
11. After all tasks are complete, perform the Phase 0 handoff validation from `PHASES.md`.
12. After handoff validation, STOP.

---

# P0-T1 — Create Native Application Skeleton

## Objective

Create the smallest valid PikaTalk Qt 6 / Kirigami desktop application.

## Instructions

Create the minimum project structure required to:

* configure
* build
* launch

a native Qt 6 application using Kirigami.

Target:

* Linux
* openSUSE
* KDE Plasma 6

Use Qt Quick / QML where appropriate.

Do not add application behavior beyond what is required to prove the application starts.

Do not add:

* PikaClaw communication
* chat persistence
* project management
* model logic
* gateway logic
* networking
* tool handling

Do not introduce abstractions for future phases.

## Deliverable

A buildable PikaTalk source tree.

The exact source layout may follow normal Qt/Kirigami conventions discovered during implementation.

## Test

1. Configure the project from a clean build directory.
2. Build successfully.
3. Launch PikaTalk.
4. Confirm a native application window opens.
5. Close and reopen it.
6. Confirm no blocking startup errors appear.

## Completion Evidence

* openSUSE version: openSUSE Tumbleweed 20260813
* Plasma version: plasmashell 6.7.4
* Qt version: 6.11.1
* Kirigami version: 6.28.0
* build commands used:
  * `cmake -B build -G Ninja --install-prefix "$HOME/.local"`
  * `cmake --build build`
* launch command used: `./build/bin/pikatalk`
* result: configure and build succeeded. First launch created a Wayland window `caption=PikaTalk class=org.radilabs.pikatalk name=pikatalk`. Process was closed and relaunched; KWin listed the same window again. No blocking startup failure. Journal warning on first launch: xdg-desktop-portal could not register app ID `org.radilabs.pikatalk` because the desktop file is not installed yet.
* meaningful files created:
  * `CMakeLists.txt`
  * `src/CMakeLists.txt`
  * `src/main.cpp`
  * `src/Main.qml`
  * `org.radilabs.pikatalk.desktop`
  * `.gitignore`

## Status

* [x] Complete

---

# P0-T2 — Create Main Application Layout

## Objective

Create the basic PikaTalk window structure that later phases will build on.

## Instructions

Create a simple main window containing:

* project/chat sidebar area
* conversation area
* message input area
* always-visible context area

The context area must contain placeholders for:

* project
* workspace
* model
* gateway state

Use fake/static labels only.

Example values may be:

```text
Project: PikaTalk
Workspace: ~/code/pikatalk
Model: example-model
Gateway: Offline
```

These values are only visual placeholders.

No buttons or controls need to perform real product actions.

Use Kirigami/native Qt components where appropriate.

Do not spend time on visual polish.

## Deliverable

A running PikaTalk window showing the intended basic structure.

## Test

1. Launch PikaTalk.
2. Confirm the sidebar is visible.
3. Confirm the conversation area is visible.
4. Confirm the message input area is visible.
5. Confirm project, workspace, model, and gateway placeholders are visible.
6. Resize the window.
7. Confirm the layout remains usable without obvious clipping or overlap.

## Completion Evidence

* layout components created:
  * `Kirigami.Page` main page
  * `contextArea` toolbar with static Project / Workspace / Model / Gateway labels
  * `sidebarArea` pane with static Projects and Chats labels
  * `conversationArea` pane
  * `inputArea` with a `TextArea` that accepts text visually only
  * `SplitView` between sidebar and conversation/input
* window sizes tested: 960x668 (default with frame) and 700x548
* any layout limitations discovered: Kirigami page title bar and the context toolbar both occupy vertical space; context labels use a wrapping `Flow` so they remain readable when the window is narrower. Sidebar is a static pane, not a collapsing Kirigami drawer, so it stays visible during resize.
* screenshots if useful: captured during local Plasma 6.7.4 Wayland run; all four required areas were visible at both sizes with no clipping or overlap.

## Status

* [x] Complete

---

# P0-T3 — Add Fake Conversation Content

## Objective

Prove the intended chat presentation without implementing chat behavior.

## Instructions

Add a small amount of hardcoded conversation content.

At minimum display:

* one user message
* one assistant message

The content exists only to demonstrate layout.

The message input may accept text visually, but must not send messages or persist chat state.

Do not build:

* message models intended for final use
* chat persistence
* message sending
* streaming
* regenerate
* stop generation
* tool-call rendering

Do not prematurely design Phase 1 or Phase 2 architecture.

## Deliverable

A basic conversation view using fake/static messages.

## Test

1. Launch PikaTalk.
2. Confirm user and assistant messages are visually distinguishable.
3. Confirm conversation content is readable.
4. Confirm the input area remains usable.
5. Resize the window and check for obvious layout breakage.

## Completion Evidence

* fake messages used:
  * user (right, selection/highlight styling): "Can you show me the intended PikaTalk layout?"
  * assistant (left, view styling): "This is a static assistant reply. The sidebar, context bar, and input field are layout placeholders only."
* layout behavior observed: user and assistant bubbles are visually distinguishable by alignment and color. Conversation sits above the unchanged message input. Input placeholder "Message" remains visible. Default window 960x640 showed no overlap.
* obvious defects found and fixed: none after the static bubbles replaced the conversation placeholder. No QML runtime errors. Recurring journal warning remains the uninstalled desktop-file portal registration from P0-T1.

## Status

* [x] Complete

---

# P0-T4 — Establish XDG Application Paths

## Objective

Define and verify the local filesystem locations PikaTalk will use for application data and configuration.

## Instructions

Use normal Linux/XDG conventions.

Determine appropriate locations for:

* application data
* configuration
* cache, if required by the framework

Do not create a custom directory hierarchy under the user's home directory when an XDG location is appropriate.

Document the chosen paths.

Do not introduce actual chat/project storage yet.

## Deliverable

PikaTalk resolves and can expose/log its intended local application paths.

Document them under:

`docs/local-storage.md`

## Test

1. Launch PikaTalk.
2. Confirm resolved application data/config paths are valid.
3. Confirm paths respect XDG environment overrides where practical.
4. Confirm required directories can be created without elevated privileges.
5. Confirm no application data is written into the source repository.

## Completion Evidence

* resolved paths (default session):
  * data: `/home/naorw/.local/share/Radilabs/PikaTalk`
  * config: `/home/naorw/.config/Radilabs/PikaTalk`
  * cache: `/home/naorw/.cache/Radilabs/PikaTalk`
* XDG mechanism/API used: Qt `QStandardPaths::AppDataLocation`, `AppConfigLocation`, and `CacheLocation`, with organization `Radilabs` and application `PikaTalk`
* override test performed: launched with `XDG_DATA_HOME=/tmp/pikatalk-xdg-t4/data`, `XDG_CONFIG_HOME=/tmp/pikatalk-xdg-t4/config`, `XDG_CACHE_HOME=/tmp/pikatalk-xdg-t4/cache`
* result: directories were created at `/tmp/pikatalk-xdg-t4/{data,config,cache}/Radilabs/PikaTalk` without elevated privileges. `applicationpaths_test` passed. No application data was written into the source repository. Decision recorded in `decisions/0001-application-identity-and-xdg-paths.md`. Paths documented in `docs/local-storage.md`.

## Status

* [x] Complete

---

# P0-T5 — Initialize SQLite

## Objective

Prove that PikaTalk can create and access its local SQLite database.

## Instructions

Create the smallest SQLite initialization path required to prove local persistent storage works.

The database must live under the application data location established in P0-T4.

The database may contain a tiny Phase 0-only test table or equivalent initialization marker.

Do not implement the Phase 1 data model.

Do not create tables for:

* projects
* chats
* messages
* drafts
* models
* tool calls

unless absolutely required only as a temporary proof, in which case they must not be treated as final schema.

Avoid designing future migrations beyond what Phase 0 needs.

## Deliverable

PikaTalk can initialize and access a local SQLite database successfully.

## Test

1. Remove the Phase 0 database from a test environment if safe.
2. Launch PikaTalk.
3. Confirm the database is created.
4. Confirm a simple write succeeds.
5. Confirm a simple read succeeds.
6. Restart PikaTalk.
7. Confirm the database remains accessible.
8. Confirm a database error produces a visible/logged diagnostic rather than silent failure.

## Completion Evidence

* database path: `/home/naorw/.local/share/Radilabs/PikaTalk/phase0.sqlite`
* SQLite/Qt mechanism used: Qt `QSqlDatabase` with the `QSQLITE` driver and a Phase 0-only `phase0_init(key, value)` table
* initialization result: first launch after removing the database created `phase0.sqlite` (12288 bytes)
* write/read test result: startup wrote marker `initialized=1` and read it back; `database_test` passed (create/write/read, reopen, unusable-path error)
* restart result: second launch reused the same database and still read `initialized=1`
* error diagnostic: launching with `XDG_DATA_HOME=/dev/null/not-a-dir` logged `Failed to create directory: /dev/null/not-a-dir/Radilabs/PikaTalk` instead of failing silently

## Status

* [x] Complete

---

# P0-T6 — Establish Development Workflow

## Objective

Make PikaTalk quick and predictable to build, run, and debug locally.

## Instructions

Document the normal development cycle for the target openSUSE + Plasma environment.

The workflow must cover:

1. installing required development dependencies
2. configuring the build
3. building
4. launching PikaTalk
5. making a visible UI change
6. rebuilding/reloading
7. finding useful application/QML logs when something breaks

Do not add product functionality during this task.

## Deliverable

Create:

`docs/development.md`

## Test

1. Follow the documented dependency/build steps.
2. Change a visible string in the UI.
3. Rebuild.
4. Launch PikaTalk.
5. Confirm the changed string appears.
6. Introduce a harmless temporary QML/application error.
7. Confirm the documented debugging method exposes the error.
8. Revert the intentional error.

## Completion Evidence

* dependencies installed: cmake, ninja, gcc-c++, kf6-extra-cmake-modules, kf6-kirigami-devel, kf6-ki18n-devel, kf6-kcoreaddons-devel, kf6-kiconthemes-devel, kf6-qqc2-desktop-style, kf6-qqc2-desktop-style-devel, qt6-base-devel, qt6-declarative-devel, qt6-quickcontrols2-devel
* configure/build commands: `cmake -B build -G Ninja --install-prefix "$HOME/.local"` then `cmake --build build`
* visible change tested: sidebar label changed to `Example chat (rebuild)`, rebuilt, launched; the new string was visible in the sidebar. The label was restored afterward.
* debugging/logging method tested: introduced `thisIdentifierDoesNotExist` on `Kirigami.ApplicationWindow`. `./build/bin/pikatalk 2> pikatalk.log` (and the captured stderr) showed `qrc:/qt/qml/org/radilabs/pikatalk/Main.qml:14:5: Cannot assign to non-existent property "thisIdentifierDoesNotExist"` and `Failed to load the PikaTalk QML interface`. The intentional error was reverted and the app launched again.
* result: documented workflow in `docs/development.md` matches the commands that were actually run.

## Status

* [x] Complete

---

# P0-T7 — Basic Plasma UI Sanity Check

## Objective

Ensure the Phase 0 native UI is usable enough that Phase 1 can build on it without immediately replacing the shell.

## Instructions

Run PikaTalk under normal Plasma use.

Fix only clear Phase 0 layout problems such as:

* clipping
* broken resizing
* unreadable text
* inaccessible input field
* unusable sidebar sizing
* missing context information

Do not redesign the application.

Do not add:

* themes
* custom styling systems
* animations
* tray behavior
* notifications
* keyboard shortcut systems
* search
* settings UI
* real chat controls

## Deliverable

Phase 0 UI with no obvious blocking layout defects.

## Test

1. Launch PikaTalk under Plasma.
2. Test normal window resize behavior.
3. Test maximized and restored window states.
4. Confirm all required Phase 0 areas remain visible and usable.
5. Confirm fake conversation content remains readable.
6. Confirm context placeholders remain visible.
7. Check logs for recurring QML/runtime errors.

## Completion Evidence

* environment tested: openSUSE Tumbleweed 20260813, Plasma 6.7.4, Qt 6.11.1, Kirigami 6.28.0, KWin Wayland session on a 1920x1080 display
* window states tested:
  * default 960x668
  * narrower 700x548
  * maximized 1920x1034 (full output width minus Plasma panel)
* defects found: none that blocked the Phase 0 layout. Sidebar, conversation, input, and context placeholders remained visible and usable. Fake user/assistant messages remained readable.
* fixes made: none required for layout. Recurring log noise is the xdg-desktop-portal app-ID warning when the desktop file is not installed; `cmake --install build` is documented in `docs/development.md`.
* remaining cosmetic limitations:
  * Kirigami page title and the context toolbar both occupy vertical space
  * sidebar is static labels, not interactive
  * no live QML reload

## Status

* [x] Complete

---

# P0-T8 — Phase 0 Documentation Check

## Objective

Ensure another implementation agent can build and understand the Phase 0 project without reconstructing the environment.

## Instructions

Review current documentation.

At minimum it must explain:

* what PikaTalk currently is
* target platform
* Qt/Kirigami stack
* how to install development dependencies
* how to configure/build
* how to launch
* how to debug
* where local application data lives
* that current chat/project/model/gateway content is fake/static
* that real local state begins in Phase 1
* that real PikaClaw communication begins in Phase 2

Do not describe future capabilities as though they already exist.

## Deliverable

Documentation matching the actual Phase 0 implementation.

## Test

Follow documented setup/build/run instructions from the repository root.

Confirm they work without relying on undocumented commands or assumptions.

## Completion Evidence

* documentation files reviewed: `README.md`, `docs/development.md`, `docs/local-storage.md`, `decisions/0001-application-identity-and-xdg-paths.md`
* commands validated from the repository root:
  * `cmake -B build -G Ninja --install-prefix "$HOME/.local"`
  * `cmake --build build`
  * `ctest --test-dir build --output-on-failure`
  * `./build/bin/pikatalk`
* corrections made: filled the previously empty `README.md` with Phase 0 status, stack, and pointers to the detailed docs; noted the harmless ECM git upstream warning in `docs/development.md`
* result: documented commands built the project, passed tests, and launched PikaTalk without undocumented extra steps

## Status

* [x] Complete

---

# P0-T9 — Phase Handoff Validation

## Objective

Determine whether Phase 0 actually satisfies its immutable contract.

This task does not add functionality.

## Instructions

Read the Phase 0 contract in `PHASES.md`.

Validate every acceptance criterion individually.

Do not mark an acceptance criterion as passed based only on code inspection when it can reasonably be tested by running PikaTalk.

Create:

`docs/handoffs/phase-0.md`

The report must contain:

### Deliverables

List meaningful files and artifacts produced.

### Tests Performed

List actual tests executed.

### Results

Record pass/fail for every Phase 0 acceptance criterion.

### Known Limitations

Record anything known to be incomplete or fragile.

### Deferred Work

Copy or reference all work discovered during Phase 0 that belongs outside its scope.

### Decisions

Reference any records created under `decisions/`.

## Deliverable

`docs/handoffs/phase-0.md`

with sufficient evidence to determine whether Phase 0 is complete.

## Test

Compare the implementation and handoff report against every Phase 0 acceptance criterion in `PHASES.md`.

All criteria must pass.

If any criterion fails:

* Phase 0 remains active.
* Record the failure.
* Create or refine a Phase 0 task required to fix it.
* Do not weaken or edit the Phase 0 contract to make it pass.

## Completion Evidence

Final result:

```text
PHASE 0 HANDOFF: PASS
```

Handoff report: `docs/handoffs/phase-0.md`

All ten Phase 0 acceptance criteria passed by running PikaTalk and the documented tests, not by code inspection alone.

## Status

* [x] Complete

---

# Deferred Work

Record discoveries that do not belong to Phase 0 here.

Do not implement them.

Format:

```text
- [date] Short description
  - Discovered while: P0-Tx
  - Suggested phase: Phase X / Future
  - Reason deferred: outside Phase 0 contract
```

- [2026-08-15] Interactive project/chat sidebar and real local persistence
  - Discovered while: P0-T2 / P0-T3
  - Suggested phase: Phase 1
  - Reason deferred: outside Phase 0 contract

- [2026-08-15] Custom application icon and AppStream metainfo
  - Discovered while: P0-T1 / P0-T8
  - Suggested phase: Phase 5
  - Reason deferred: packaging/polish, not required to prove the native shell

- [2026-08-15] Live QML reload during development
  - Discovered while: P0-T6
  - Suggested phase: Future
  - Reason deferred: not a Phase 0 product requirement; rebuild/relaunch is sufficient

---

# STOP CONDITION

When `P0-T9` passes:

**STOP.**

Do not create the Phase 1 task file.

Do not implement projects, chats, workspace state, model state, drafts, or real persistence models.

Do not investigate or connect to the PikaClaw gateway.

Do not implement gateway controls.

Do not modify the Phase 1 contract.

Phase 1 requires a new explicit execution instruction.
