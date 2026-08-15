# Phase 1 Handoff — Local Chat State

## Deliverables

Meaningful files produced or updated:

* `src/database.h` / `src/database.cpp` — schema version 1 store (`pikatalk.sqlite`)
* `src/appcontroller.h` / `src/appcontroller.cpp` — project/chat/message/draft/workspace/model UI state
* `src/Main.qml` — selectable project/chat lists, persisted conversation, context bar, drafts
* `src/main.cpp` — opens `pikatalk.sqlite` and exposes `app` to QML
* `src/CMakeLists.txt` — includes AppController
* `tests/database_test.cpp` — schema, round-trip, reopen, project isolation, chat/message isolation
* `tests/appcontroller_test.cpp` — project/chat/message/workspace/model/draft/workflow/isolation tests
* `tests/CMakeLists.txt` — `database_test` and `appcontroller_test`
* `decisions/0002-local-sqlite-schema.md`
* `docs/local-storage.md` — Phase 1 database location, schema, inheritance, drafts
* `docs/development.md` — current database path and tests
* `README.md` — Phase 1 status
* `tasks/phase-1-local-chat-state.md` — execution evidence
* `docs/handoffs/phase-1.md` — this report

Built artifact on the development machine:

* `build/bin/pikatalk`

Production database:

* `$XDG_DATA_HOME/Radilabs/PikaTalk/pikatalk.sqlite`
* default `/home/naorw/.local/share/Radilabs/PikaTalk/pikatalk.sqlite`

`phase0.sqlite` is not used or migrated.

## Tests Performed

* `cmake --build build`
* `ctest --test-dir build --output-on-failure` — appstreamtest, applicationpaths_test, database_test, appcontroller_test (4/4 passed)
* Launched `./build/bin/pikatalk`; logged `pikatalk.sqlite`; no QML load failure
* Automated coverage for create/rename/select/delete projects; create/rename/select/archive/delete chats; local user/assistant messages; workspace and model inheritance/overrides; drafts across switch and restart; isolation across projects

## Results

Phase 1 acceptance criteria from `PHASES.md`:

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Projects can be created, selected, renamed, and persisted | PASS | `appcontroller_test::createRenameSwitchDeleteAndReopen`; sidebar New/Rename/Delete |
| 2 | Chats can be created, selected, renamed, archived/deleted, and persisted | PASS | `chatsAreScopedToProjectAndSurviveRestart`; sidebar New/Rename/Archive/Delete |
| 3 | Existing chats are available after restarting PikaTalk | PASS | reopen restores most recently active non-archived chat and its project |
| 4 | Local messages are persisted correctly | PASS | `messagesAreIsolatedPerChatAndSurviveRestart`; `messages` table `role`/`position` |
| 5 | Project default workspace is inherited by new chats | PASS | `workspaceDefaultsAndOverridesPersist`; NULL `workspace_override` |
| 6 | A chat can override its workspace | PASS | `setCurrentChatWorkspaceOverride`; context bar shows override |
| 7 | Project default model is inherited by new chats | PASS | `modelDefaultsAndOverridesPersist`; NULL `model_override` |
| 8 | A chat can override its model | PASS | `setCurrentChatModelOverride`; local string only, no discovery |
| 9 | Active project, workspace, and model are clearly visible | PASS | context bar labels; gateway remains static Offline |
| 10 | Draft text survives chat switching and application restart | PASS | `draftsSurviveSwitchAndRestartWithoutCreatingMessages` |
| 11 | Removing or changing one chat does not corrupt unrelated chats | PASS | `isolationAcrossProjectsSurvivesRestart`; CASCADE scoped to the deleted row |
| 12 | Phase 0 functionality does not regress | PASS | `applicationpaths_test` still passes; app still builds/launches as a Plasma Kirigami window; XDG paths unchanged |

## Known Limitations

* Running from `build/bin/pikatalk` without `cmake --install build` logs `xdg-desktop-portal` warning `App info not found for 'org.radilabs.pikatalk'`. The window still opens.
* There is no unarchive UI. Archived chats remain in SQLite (`archived = 1`) but are hidden from the default list.
* Assistant replies are local only via **Local reply**. Nothing is sent to a model or gateway.
* Model and workspace values are free-form local strings. They are not validated or discovered.
* Selected project/chat after restart is the globally most recently active non-archived chat, not a separate session-settings table.
* QML changes require rebuild; there is no live reload.
* CMake/ECM may print `fatal: no upstream configured for branch ...` on a local-only git branch. Configure still succeeds.

## Deferred Work

From `tasks/phase-1-local-chat-state.md` (not implemented):

* PikaClaw API/gateway communication — Phase 2
* Real model discovery — Phase 2
* Streaming / stop / regenerate — Phase 2
* Tool calls/results — Phase 3
* Open workspace in terminal/editor/file manager — Phase 3
* Gateway start/stop/restart — Phase 4
* Search, tray, notifications, polish — Phase 5
* Conversation branching — Future
* Unarchive UI for archived chats — Future
* Custom application icon and AppStream metainfo — Phase 5 (from Phase 0)

## Decisions

* `decisions/0001-application-identity-and-xdg-paths.md` — organization `Radilabs`, application `PikaTalk`, XDG paths
* `decisions/0002-local-sqlite-schema.md` — `pikatalk.sqlite`, schema version 1, NULL overrides mean inherit, project delete cascades

```text
PHASE 1 HANDOFF: PASS
```
