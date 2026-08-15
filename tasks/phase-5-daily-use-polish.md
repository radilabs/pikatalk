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

Recorded 2026-08-15.

* **Exact shortcuts** (window-level `Shortcut` items in `Main.qml`, no configuration UI):
  * **Ctrl+N** (`StandardKey.New`, `newChatShortcut`) — `app.createChat(i18n("New chat"))` when `app.currentProjectId > 0`; otherwise disabled so no empty-project chat is created.
  * **Ctrl+F** (`StandardKey.Find`, `titleFilterShortcut`) — focuses and selects `titleFilterField`.
  * **Ctrl+L** (`messageInputShortcut`) — focuses `messageInput`. No `StandardKey` maps to “composer focus”; explicit `Ctrl+L` used.
  * **Escape** (`stopGenerationShortcut`) — `app.stopGeneration()` only while `app.isGenerating`; disabled when idle so Escape is not consumed for unrelated behavior.

* **Conflicts resolved:** None required a different chord. Escape is gated on generation so it does not fight dialog dismiss / SearchField clear / text editing while idle. Ctrl+F uses Qt/Plasma Find (`StandardKey.Find`) to focus the existing title filter rather than introducing a second find UI.

* **Tests:**
  1. Ctrl+N — `ShortcutsTest::ctrlNCreatesChatWhenProjectIsSelected` (wired to `createChat`, enabled only with a current project).
  2. Ctrl+F — `ShortcutsTest::ctrlFFocusesTitleFilter`.
  3. Ctrl+L — `ShortcutsTest::ctrlLFocusesMessageInput`.
  4. Escape while generating — `ShortcutsTest::escapeStopsGenerationOnlyWhileActive` (`enabled: app.isGenerating` + `stopGeneration()`). Idle Escape does not activate that shortcut. `AppController::stopGeneration()` already returns false when not generating.
  5. Text editing — shortcuts are window-level and do not override TextArea/TextField editing keys (Ctrl+N/F/L and Escape-while-generating only).

### Tests

TDD RED (`Main.qml` had no Shortcut objectNames):

```text
FAIL!  : ShortcutsTest::ctrlNCreatesChatWhenProjectIsSelected() '!block.isEmpty()' returned FALSE. (Main.qml must declare Shortcut objectName newChatShortcut)
FAIL!  : ShortcutsTest::ctrlFFocusesTitleFilter() '!block.isEmpty()' returned FALSE. (Main.qml must declare Shortcut objectName titleFilterShortcut)
FAIL!  : ShortcutsTest::ctrlLFocusesMessageInput() '!block.isEmpty()' returned FALSE. (Main.qml must declare Shortcut objectName messageInputShortcut)
FAIL!  : ShortcutsTest::escapeStopsGenerationOnlyWhileActive() '!block.isEmpty()' returned FALSE. (Main.qml must declare Shortcut objectName stopGenerationShortcut)
Totals: 2 passed, 4 failed
```

GREEN after `Shortcut` items on `Kirigami.ApplicationWindow`:

```text
./build/bin/shortcuts_test
PASS   : ShortcutsTest::ctrlNCreatesChatWhenProjectIsSelected()
PASS   : ShortcutsTest::ctrlFFocusesTitleFilter()
PASS   : ShortcutsTest::ctrlLFocusesMessageInput()
PASS   : ShortcutsTest::escapeStopsGenerationOnlyWhileActive()
Totals: 6 passed, 0 failed
```

Full suite:

```text
cmake --build build && ctest --test-dir build --output-on-failure
100% tests passed, 9 tests passed
```

## Status

* [x] Complete

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

### States changed

* **Empty projects list:** `Kirigami.PlaceholderMessage` overlay on `projectList` (`emptyProjectsPlaceholder`).
* **Empty chats list:** overlay on `chatList` (`emptyChatsPlaceholder`).
* **Empty conversation:** overlay on `conversationArea` (`emptyMessagesPlaceholder`); hidden while `app.isGenerating` so it does not fight the generating label.
* **Title-filter miss:** distinct “No matching …” copy when the unfiltered list is non-empty.
* **Request error:** still `Error: %1`, but `%1` is `errorCopy.sanitize(app.requestError)` (display-only; `AppController::requestError` unchanged).
* **Gateway toolbar error / gateway action line:** same sanitizer. Connecting / reconnecting / stopped / connected labels were already clear and were left as-is.
* **Generation:** existing `Generating…` (or streaming text) labeled `generationStatusLabel`. Existing Stop and Retry / Regenerate buttons unchanged.

### Text / actions used

| State | Title | Explanation (points at existing UI) |
|---|---|---|
| No projects | No projects | Create a project (sidebar **New**) |
| Filter misses projects | No matching projects | (filter field) |
| No chats, no project | No chats | Create a project |
| No chats, project selected | No chats | Create a chat (sidebar **New**, Ctrl+N) |
| Filter misses chats | No matching chats | (filter field) |
| No messages, chat selected | No messages | Send a message |
| No messages, no chat | No messages | Create a chat / Create a project |
| Gateway connecting | Gateway: Connecting | existing toolbar |
| Gateway reconnecting | Gateway: Reconnecting… | existing toolbar |
| Gateway stopped | Gateway: Stopped | existing **Start gateway** |
| Gateway error | Gateway: Error — \<sanitized\> | existing Start/Restart |
| Request failure | Error: \<sanitized\> | existing **Retry / Regenerate** |
| Generating | Generating… | existing **Stop** / Escape |

Sanitizer maps JSON dumps → `Request failed`; `QAbstractSocket` / `WebSocket` noise → `Connection failed`; known `gateway unavailable` / `connection lost` / `gateway error` → short human copy; long dumps truncated to 120 characters with collapsed whitespace.

### Before / after rough edges

* Before: empty ListViews were blank; user had no in-list hint that **New** / composer was the next step.
* After: placeholders name the empty state and the existing next action.
* Before: `Error: %1` could show raw JSON or Qt socket enums.
* After: protocol noise is replaced or truncated at display time.
* Gateway connecting/stopped copy was already in `gatewayPlaceholder`; not redesigned.

### Layout sanity

* Window still `960x640`, min `640x480`. SplitView / input pane / toolbar layout unchanged.
* Placeholder icons disabled (`icon.name: ""`) so the short project ListView (~8 grid units) and min-width sidebar (~12 grid units) do not overflow.
* Placeholder width is `parent.width - 2 * largeSpacing` with wrapping.
* Offscreen smoke load (`QT_QPA_PLATFORM=offscreen timeout 4 ./build/bin/pikatalk`) started without QML errors (exit 124 = timeout). Interactive 640×480 click-through was not run in this session.

### Actual tests

TDD: `errorcopy_test` failed on JSON/protocol/truncation while `sanitizeUserFacingError` only trimmed. `uistates_test` failed on missing placeholder `objectName`s before QML overlays existed.

```text
./build/bin/errorcopy_test
Totals: 8 passed, 0 failed

./build/bin/uistates_test
Totals: 8 passed, 0 failed

cmake --build build && ctest --test-dir build --output-on-failure
100% tests passed, 11 tests passed
```

Did not implement P5-T5+, tray, notifications, or full-text search. Did not refactor `AppController`.

## Status

* [x] Complete

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

Recorded 2026-08-15.

* **Conversation size tested:** 60 user/assistant turns (**120 messages**) in one chat, plus **6 fenced C++ code blocks** (every 10th assistant reply), **8 tool activities** (7 ok + 1 error, ~400-character results) attached to the last assistant message, and a second **2-message** chat for switching. After send-another-message: **121 messages**.
* **Behaviors exercised:**
  1. Offscreen `ListView` `positionViewAtBeginning` / `positionViewAtEnd` / beginning again on 128 rows (120 messages + 8 tools) — no freeze (`offscreenListViewScrollsLongConversationWithoutFreeze`).
  2. Copy full message text (`copyText` / `lastCopiedText`).
  3. Copy fenced code via `messageSegments` (`kind: code` contains `int fib(int n)`).
  4. Tool rows stay in the timeline; QML details start collapsed (`toolActivityDetails` `visible: false`, summary toggles).
  5. `addUserMessage` after the long history (`one more question after the long history`).
  6. Switch to short chat (2 messages, 0 tools) and back (120 messages + 8 tools restored).
  7–8. `openStore` restart: most recently active chat is the long chat; full history, tools, and extra user message return.
  9. Drafts: long-chat draft survives switch and restart; short-chat draft isolated.
  10. Offscreen `./build/bin/pikatalk` with seeded `XDG_DATA_HOME` started in ~2.5s, no QML/SQLite load failure, no `picoclaw-launcher` spawn in process output.
* **Reproducible issue found:** none that blocked daily use. Existing Qt `ListView` + full `QVariantList` load is adequate at this size (`openStore` well under 2s).
* **Fix applied:** none (no pagination, virtualization framework, or `AppController` rewrite).
* **Restart result:** selected chat is the globally most recently active non-archived chat (existing Phase 1 semantics). Long-chat content, tools, and drafts restore when that chat is selected.

### Tests

New `longchat_test` (no product-code change):

```text
QT_QPA_PLATFORM=offscreen ./build/bin/longchat_test
PASS   : LongChatTest::longConversationReloadsAfterRestartAndChatSwitch()
PASS   : LongChatTest::draftsSurviveOnLongChatAcrossSwitchAndRestart()
PASS   : LongChatTest::copyTextAndCodeSegmentsRemainUsableOnLongHistory()
PASS   : LongChatTest::conversationListViewStaysPlainQtListView()
PASS   : LongChatTest::offscreenListViewScrollsLongConversationWithoutFreeze()
PASS   : LongChatTest::offscreenPikaTalkLoadsSeededLongChat()
Totals: 8 passed, 0 failed
```

Full suite:

```text
cmake --build build && ctest --test-dir build --output-on-failure
100% tests passed, 12 tests passed
```

Launcher hygiene: did not start a temp `picoclaw-launcher` on 18800. User launcher pid 71127 was already running and was left untouched.

Did not implement P5-T6+, tray, notifications, or full-text search. Did not refactor `AppController` or split `Main.qml`.

## Status

* [x] Complete

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
