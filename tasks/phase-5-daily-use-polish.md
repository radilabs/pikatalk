# Phase 5 Tasks — Daily-Use Polish

Phase contract: `PHASES.md` → **Phase 5 — Daily-Use Polish**

This file contains the complete selected scope for Phase 5.

Phase 5 is the final contracted v1 phase.

## Selected Phase 5 Scope

Implement only:

* application versioning
* project/chat title filtering
* defined keyboard shortcuts
* empty/loading/error-state cleanup
* long-chat usability verification and narrowly scoped fixes
* restart/state reliability verification
* clean openSUSE/Plasma local installation path
* first-release README/install documentation
* desktop packaging metadata needed for a normal local installation

Explicitly **not selected**:

* full-text message search
* tray integration
* notifications

Those optional items in `PHASES.md` are not authorized merely because Phase 5 permits them.

---

# Hard Guardrails

1. Work only from this task file.
2. Work one task at a time.
3. Do not implement optional Phase 5 features not selected above.
4. Do not implement full-text message search.
5. Do not implement tray integration.
6. Do not implement notifications.
7. Do not refactor `AppController` merely to improve architecture.
8. Do not split/rewrite `Main.qml` merely for cleanliness.
9. Small task-local extraction is allowed only when directly required to implement a selected task safely and simply.
10. Do not introduce new architectural layers for hypothetical future work.
11. Do not add new platform capabilities.
12. Do not begin any Future Phase Candidate.
13. Any idea outside this task list goes to **Deferred Work**, not code.
14. Mark a task complete only after its test procedure passes.
15. Record actual completion evidence.
16. After Phase 5 handoff, STOP.

## Code-Quality Guardrail

`AppController` is already a large coordinator.

Do not automatically place new unrelated subsystems into it.

For the selected Phase 5 work:

* add only the smallest controller/QML surface necessary
* prefer existing state and existing Qt/Kirigami mechanisms
* do not perform preventative architecture cleanup
* do not refactor merely to reduce file size

Similarly, `Main.qml` is already large.

Do not perform a broad component split.

If one selected feature clearly benefits from a tiny dedicated QML component, extraction must remain local to that feature and must not become a general UI rewrite.

---

# P5-T1 — Define Application Version

## Objective

Give PikaTalk a simple, visible, reproducible application version suitable for a first release.

## Instructions

Define one application version in the build system.

Expose it through the normal Qt application version mechanism.

Use one source of truth.

The initial version should represent the first usable PikaTalk release.

Do not invent:

* automatic semantic-release machinery
* build servers
* release bots
* complex Git-derived version systems

Show the version somewhere appropriate and low-noise, such as an About surface or existing application information UI.

Do not clutter the main chat window title merely to satisfy the task.

## Deliverable

PikaTalk has a defined version available at runtime and documented for release use.

## Test

1. Clean configure/build.
2. Confirm runtime application version matches the build version.
3. Confirm the user can find the version from the normal application UI.
4. Confirm changing the build version has one obvious source of truth.

## Completion Evidence

Recorded 2026-08-15.

* **Version chosen:** `1.0.0` (first usable PikaTalk release)
* **Source of truth:** `project(pikatalk VERSION 1.0.0 …)` in root `CMakeLists.txt`. CMake passes `${PROJECT_VERSION}` as `PIKATALK_VERSION` into the app and version test.
* **Runtime exposure:** `configureApplicationIdentity()` calls `QCoreApplication::setApplicationVersion(PIKATALK_VERSION)` from `main.cpp`. QML reads `Qt.application.version`.
* **UI location:** Help → About PikaTalk (`Kirigami.AboutPage` in `src/AboutPikaTalk.qml`). Window title is unchanged.

### Tests

TDD RED (before `setApplicationVersion` / before bumping CMake version):

```text
./build/bin/applicationversion_test
FAIL!  : ApplicationVersionTest::runtimeVersionMatchesBuildVersion()
   Actual   (QCoreApplication::applicationVersion()): ""
   Expected (QStringLiteral(PIKATALK_VERSION))      : "0.1.0"
```

GREEN after implementation:

```text
./build/bin/applicationversion_test
PASS   : ApplicationVersionTest::runtimeVersionMatchesBuildVersion()
Totals: 3 passed, 0 failed
```

Clean configure/build and full suite:

```text
rm -rf build && cmake -B build -G Ninja && cmake --build build && ctest --test-dir build --output-on-failure
100% tests passed, 7 tests passed
```

UI: About page `aboutData.version` is `Qt.application.version` (not a second hardcoded string). Changing the version requires editing only the root `project(… VERSION …)` line.

## Status

* [x] Complete

---

# P5-T2 — Add Project and Chat Title Filter

## Objective

Make daily navigation comfortable when the sidebar contains more projects and chats.

## Instructions

Add fast filtering using project and chat **titles/names only**.

The filter must:

* make matching projects/chats easy to find
* be case-insensitive unless Qt's normal behavior strongly suggests otherwise
* update without mutating persisted project/chat state
* clear cleanly
* preserve normal selection behavior where practical

This is an in-memory/UI title filter.

Do not:

* search message bodies
* add SQLite FTS
* change the database schema for search
* build advanced query syntax
* add tags/folders/search indexes

The implementation may use one search field or a similarly simple UX, but both project and chat title search required by `PHASES.md` must work.

## Test

Create enough test data to make filtering meaningful.

Verify:

1. Project-name match.
2. Chat-title match.
3. Non-match disappears from filtered view.
4. Clearing filter restores lists.
5. Search does not modify persistence.
6. Restart with no filtering-related state corruption.
7. Archived/deleted semantics remain unchanged.

## Completion Evidence

Recorded 2026-08-15.

* **Filtering behavior:** In-memory, case-insensitive substring match on project `name` and chat `title` only. Empty/whitespace filter shows the full lists. `Kirigami.SearchField` (`titleFilterField`) filters both sidebar ListViews via standalone `filterItemsByTitle` / `TitleMatch::apply`. Persisted `app.projects` / `app.chats` are not replaced. Message bodies are not searched. No SQLite FTS or schema change.
* **Project test:** `TitleFilterTest::projectNameMatchIsCaseInsensitive` — `"kItChEn"` keeps `"Kitchen Reno"` and drops `"Garden"` / `"Work Notes"`.
* **Chat test:** `TitleFilterTest::chatTitleMatchIsCaseInsensitive` — `"SHOP"` keeps `"Shopping list"`. Non-matches: `nonMatchingTitlesAreOmitted`. Clear: `clearingFilterRestoresFullList`.
* **Persistence / restart / archive:** `TitleFilterTest::persistenceAndArchiveAreUnchangedByFiltering` — three projects and three chats (one archived, one deleted). Filtering does not change `controller.projects()` / `controller.chats()`. Reopen restores 3 projects and 2 active chats; archived chat remains only in `listChatIds(..., includeArchived=true)`.

### Tests

TDD RED (test compiled before `titlefilter.h` existed):

```text
/home/naorw/dev/radilabs/pikatalk/tests/titlefilter_test.cpp:1:10: fatal error: titlefilter.h: No such file or directory
    1 | #include "titlefilter.h"
```

GREEN after helper + QML wiring:

```text
./build/bin/titlefilter_test
PASS   : TitleFilterTest::emptyFilterKeepsAllItems()
PASS   : TitleFilterTest::projectNameMatchIsCaseInsensitive()
PASS   : TitleFilterTest::chatTitleMatchIsCaseInsensitive()
PASS   : TitleFilterTest::nonMatchingTitlesAreOmitted()
PASS   : TitleFilterTest::clearingFilterRestoresFullList()
PASS   : TitleFilterTest::messageBodiesAreNotSearched()
PASS   : TitleFilterTest::filterDoesNotMutateSourceList()
PASS   : TitleFilterTest::persistenceAndArchiveAreUnchangedByFiltering()
Totals: 10 passed, 0 failed
```

Full suite:

```text
cmake --build build && ctest --test-dir build --output-on-failure
100% tests passed, 8 tests passed
```

## Status

* [x] Complete

---

# P5-T3 — Add Fixed Keyboard Shortcuts

## Objective

Make common PikaTalk operations usable from the keyboard without creating a configurable shortcut system.

## Selected Shortcuts

Implement exactly these operations, following normal Plasma/Qt conventions where practical:

* **Ctrl+N** — create a new chat in the current project
* **Ctrl+F** — focus/select the title filter
* **Ctrl+L** — focus the message input
* **Escape** — stop generation **only while generation is active**

If an exact shortcut conflicts with established Qt/KDE behavior in this application, document the conflict and use the smallest sensible alternative rather than inventing a shortcut framework.

## Instructions

Use normal Qt/QML shortcut facilities.

Do not implement:

* shortcut configuration UI
* global shortcuts
* custom keybinding profiles
* command palette
* Vim mode

Shortcuts must respect existing action preconditions.

Examples:

* New Chat should not create a chat with no current project.
* Escape should not perform unrelated behavior when nothing is generating.

## Test

Verify each shortcut manually and where useful automatically:

1. Ctrl+N creates/selects a new chat.
2. Ctrl+F focuses the filter.
3. Ctrl+L focuses message input.
4. Escape stops active generation.
5. Escape while idle does not damage state.
6. Existing normal text editing remains usable.

## Completion Evidence

Record:

* exact shortcuts
* any conflict resolved
* test result for each

## Status

* [ ] Complete

---

# P5-T4 — Clean Up Empty, Loading, Offline, and Error States

## Objective

Remove obvious daily-use rough edges without redesigning the application.

## Instructions

Review the existing normal UI and improve only clearly weak state presentation.

Cover at minimum:

### Empty states

* no projects
* project with no chats
* chat with no messages

### Gateway states

* connecting/reconnecting
* stopped
* unavailable/error

### Request state

* request failure
* active generation/loading indication

Keep error copy concise and understandable.

Prefer useful actions already supported by the application.

Examples:

* “Create a project”
* “Create a chat”
* “Gateway stopped”
* existing Start/Retry behavior where appropriate

Do not introduce new functionality to make an empty state interesting.

Do not redesign the application layout.

Do not add dashboards, status pages, onboarding flows, or tutorials.

## Test

Exercise each listed state.

Verify:

1. User can understand what is happening.
2. User can identify the next existing action where appropriate.
3. Error messages do not expose unnecessary protocol noise by default.
4. State changes return cleanly to normal chat UI.
5. Normal-sized and minimum-sized window remain usable.

## Completion Evidence

Record:

* states changed
* text/actions used
* before/after rough edges addressed
* layout sanity result

## Status

* [ ] Complete

---

# P5-T5 — Verify Long-Chat and Restart Usability

## Objective

Satisfy the Phase 5 reliability criteria without inventing performance architecture.

## Instructions

Exercise PikaTalk with a realistically long local conversation.

Include:

* many user/assistant messages
* several code blocks
* tool activity
* enough content to require substantial scrolling

Verify:

* scrolling remains practically usable
* message controls remain usable
* input remains responsive
* tool details do not make navigation unusable
* switching chats remains reasonable
* returning to a long chat restores expected content
* restart restores the expected selected/local state according to existing semantics

Only fix **observed, reproducible daily-use problems**.

Allowed fixes should be small and directly tied to the observed issue.

Do not proactively implement:

* custom virtualization frameworks
* pagination architecture
* lazy-loading storage architecture
* message-windowing systems
* background indexing
* controller rewrite

If the existing Qt `ListView` behavior is adequate, record that and make no structural change.

## Test

Create/use a long conversation and test:

1. Scroll from recent to old content and back.
2. Copy normal text.
3. Copy code.
4. Expand/collapse tool activity.
5. Send another message.
6. Switch to another chat and back.
7. Restart PikaTalk.
8. Confirm persisted history and expected local context return.
9. Confirm drafts still behave correctly.
10. Confirm no serious UI freeze or obvious rendering failure.

## Completion Evidence

Record:

* approximate conversation size tested
* behaviors exercised
* any reproducible issue found
* smallest fix applied, if any
* restart result

## Status

* [ ] Complete

---

# P5-T6 — Clean Local openSUSE / Plasma Installation

## Objective

Make PikaTalk install like a real local Plasma application from the repository.

## Instructions

Validate and complete the existing CMake install path.

The supported v1 target remains:

* Linux
* openSUSE Tumbleweed
* KDE Plasma 6

A normal local installation must install the required application artifacts into the chosen prefix.

Verify as applicable:

* executable
* `.desktop` file
* application icon
* AppStream/metainfo

Phase 0 deferred custom icon/AppStream work may be completed here because it directly supports first-release packaging.

Keep packaging small.

Do not:

* add Flatpak
* add Snap
* add AppImage
* add cross-distro packaging matrix
* add Windows/macOS packaging
* build an OBS release pipeline
* add automatic update machinery

An RPM/spec is **not required** unless an actual clean local installation cannot reasonably satisfy the Phase 5 contract without one.

## Deliverable

A documented local installation path that results in a normal launchable Plasma application.

## Test

From a clean build directory:

```bash
cmake -B build -G Ninja --install-prefix "$HOME/.local"
cmake --build build
cmake --install build
```

Then verify:

1. Installed executable runs.
2. PikaTalk appears/launches through the installed desktop entry.
3. Installed desktop entry resolves application identity correctly.
4. Icon resolves if supplied.
5. AppStream metadata validates if supplied.
6. XDG data/config paths remain unchanged.
7. Uninstalled build-tree development workflow still works.

Where practical, perform the documented dependency/install process from a clean or freshly validated openSUSE environment.

## Completion Evidence

Record:

* required packages
* configure/build/install commands
* installed artifacts
* desktop launch result
* validation commands/results

## Status

* [ ] Complete

---

# P5-T7 — First-Release Documentation

## Objective

Make the repository useful to somebody installing PikaTalk without knowledge from this development session.

## Instructions

Bring user-facing documentation to first-release quality.

README should clearly cover:

* what PikaTalk is
* current v1 capabilities
* what PikaTalk is **not**
* supported platform
* PicoClaw requirement
* build dependencies
* build
* local install
* first run
* where local data/config live
* launcher/gateway requirements
* version
* relevant known limitations

Keep developer-only detail in `docs/`.

Update developer documentation where Phase 5 changes require it.

Do not turn README into internal architecture history.

Do not claim unsupported portability.

## Test

Read documentation as a first-time openSUSE/Plasma user.

Verify every command/path against the actual repository.

Remove stale Phase 0–4 language that incorrectly describes current behavior.

## Completion Evidence

Record:

* README sections updated
* docs updated
* commands verified
* stale information removed

## Status

* [ ] Complete

---

# P5-T8 — Full v1 Regression Pass

## Objective

Prove daily-use polish did not damage the working product.

## Instructions

Run the full automated suite and a deliberate normal-use workflow covering Phases 0–5.

Include:

* projects
* chats
* persistence
* drafts
* workspace/model selection
* real PicoClaw chat
* streaming
* stop
* retry/regenerate
* tool activity
* workspace actions
* gateway lifecycle
* title filtering
* keyboard shortcuts
* restart
* installed application launch

Use gated live tests according to the established launcher hygiene rules.

Do not leave temporary launchers/processes behind.

## Test

At minimum:

1. Full `ctest`.
2. Real normal chat.
3. Real tool-using chat where practical.
4. Gateway stop/start/restart/reconnect.
5. Workspace file manager/terminal/editor actions.
6. Project/chat title filter.
7. Selected keyboard shortcuts.
8. Long conversation workflow.
9. Restart/state restore.
10. Installed desktop application launch.

## Completion Evidence

Record:

* full automated result
* live tests
* launcher hygiene result
* regressions found/fixed

## Status

* [ ] Complete

---

# P5-T9 — Phase 5 Handoff / First Release Validation

## Objective

Determine whether the repository represents a usable first PikaTalk release.

This task adds no functionality.

## Instructions

Read **Phase 5 — Daily-Use Polish** in `PHASES.md`.

Create:

`docs/handoffs/phase-5.md`

Validate all 11 Phase 5 acceptance criteria individually.

## Required Handoff Sections

### Deliverables

Meaningful Phase 5 files/artifacts.

### Selected Scope

Explicitly state that Phase 5 selected:

* title filtering
* keyboard shortcuts
* state/reliability cleanup
* versioning
* local packaging/install
* release docs

Explicitly state that it did **not** select:

* full-text message search
* tray
* notifications

### Tests Performed

Actual automated/manual/live/install tests.

### Results

Pass/fail for all 11 Phase 5 acceptance criteria.

For criterion 7:

> Any notifications/tray features introduced are useful and non-annoying.

Record:

**PASS — no notifications or tray features were introduced in the selected Phase 5 scope.**

### Known Limitations

Current genuine v1 limitations.

### Deferred Work

Remaining ideas not implemented.

### Decisions

Reference ADRs only where needed.

### Release Readiness

State whether repository state constitutes a usable first PikaTalk release.

## Test

Compare implementation against every Phase 5 acceptance criterion.

If any selected requirement fails:

* Phase 5 remains active.
* Fix it within Phase 5 if it is inside the selected scope.
* Do not weaken the contract.
* Do not add unrelated optional features.

Final result:

```text
PHASE 5 HANDOFF: PASS
```

or:

```text
PHASE 5 HANDOFF: FAIL
```

## Status

* [ ] Complete

---

# Deferred Work

Record valid ideas here rather than implementing them.

Likely candidates:

* full-text message search → Future
* tray integration → Future
* notifications → Future
* conversation branching/forking → Future
* richer project instructions/context → Future
* additional Plasma integration → Future
* deeper AppController/UI structural cleanup → only if later feature pressure justifies it
* PicoClaw per-request workspace execution → gateway capability / Future

Do not convert these into implementation work during Phase 5.

---

# STOP CONDITION

When `P5-T9` passes:

**STOP.**

PikaTalk v1 is complete under the current phase plan.

Do not invent Phase 6.

Do not implement anything from Future Phase Candidates.

Any significant new capability requires a deliberately written new phase contract before implementation.
