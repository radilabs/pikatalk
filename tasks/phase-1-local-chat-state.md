# Phase 1 Tasks — Local Chat State

Phase contract: `PHASES.md` → **Phase 1 — Local Chat State**

This file contains only implementation work for Phase 1.

## Execution Rules

1. Read `PROJECT.md`.
2. Read the Phase 1 contract in `PHASES.md`.
3. Work on tasks in order.
4. Work on only one task at a time.
5. Do not implement anything listed under Phase 1 exclusions.
6. Do not begin Phase 2 work.
7. Discoveries outside Phase 1 go under **Deferred Work**.
8. Mark a task complete only after its test passes.
9. Record completion evidence under each task.
10. Completing all tasks does **not** automatically complete Phase 1.
11. Perform the Phase 1 handoff validation after all implementation tasks.
12. After handoff validation, STOP.

## Phase 1 UI Guardrails

Two Phase 0 observations should be kept in mind:

* The context bar currently shares vertical space with the Kirigami page title. Adjust it only if real Phase 1 values make the layout clearly awkward.
* The static sidebar must become a real selectable project/chat list. Use appropriate Qt/Kirigami list components without redesigning the entire application.

These are observations, not permission for general UI redesign.

---

# P1-T1 — Define Local Persistence Schema

## Objective

Replace the Phase 0 proof database with the smallest persistent model required by Phase 1.

## Instructions

Define SQLite storage for:

* projects
* chats
* messages
* drafts

Projects must support:

* name
* default workspace
* default model

Chats must support:

* project association
* title/name
* workspace override
* model override
* archive/deleted state as required by the chosen implementation
* persistent ordering/history

Messages must support at minimum:

* chat association
* role: user / assistant
* content
* stable ordering

The message representation must be extensible enough that later phases can add tool-related records without replacing the basic conversation model.

Do not implement tool records now.

Define a minimal migration/versioning mechanism suitable for future schema changes.

Do not over-engineer an ORM or generic persistence framework.

The existing `phase0.sqlite` proof database must not silently become the production database schema unless deliberately migrated and documented.

## Deliverable

A persistent Phase 1 database schema and initialization/migration path.

## Test

1. Initialize from no existing Phase 1 database.
2. Confirm all required tables/state exist.
3. Insert and read one project, chat, message, and draft.
4. Restart/reopen the database.
5. Confirm data remains intact.
6. Confirm schema version/migration state can be identified.

## Completion Evidence

* database path: `$XDG_DATA_HOME/Radilabs/PikaTalk/pikatalk.sqlite` (resolved `/home/naorw/.local/share/Radilabs/PikaTalk/pikatalk.sqlite`)
* schema (version 1): `schema_version`, `projects`, `chats`, `messages`, `drafts`
* schema version mechanism: table `schema_version(version INTEGER PRIMARY KEY)`; current version `1`
* migration approach: empty/new files create version 1; unsupported versions fail openly; later versions add `n` → `n+1` migrations. `phase0.sqlite` is not migrated or reused.
* tests performed: `database_test` (init from missing file, tables exist, insert/read project+chat+message+draft, reopen persistence, schema version 1). Full `ctest` passed. App launch logged `PikaTalk sqlite database: .../pikatalk.sqlite` and `PikaTalk schema version: 1`.
* decisions created: `decisions/0002-local-sqlite-schema.md`

## Status

* [x] Complete

---

# P1-T2 — Implement Project Persistence

## Objective

Implement persisted Hermes-style projects/folders.

## Instructions

Support:

* create project
* select project
* rename project
* delete project where safe

Each project must store:

* name
* default workspace
* default model

Deletion behavior must avoid silently corrupting or orphaning unrelated data.

Choose simple predictable behavior for projects containing chats.

If the behavior is a compatibility choice future work must respect, record it as an ADR.

## Deliverable

Working persisted project management.

## Test

1. Create two projects.
2. Rename one.
3. Switch between them.
4. Restart PikaTalk.
5. Confirm both persist with the correct names.
6. Delete one safely.
7. Confirm the unrelated project remains intact.

## Completion Evidence

* operations tested: create two projects, rename one, switch selection, reopen store, delete one, confirm the other remains (`appcontroller_test`, `database_test::projectRenameAndDeleteAreIsolated`)
* restart result: after closing and reopening the database, both renamed/original names were present
* deletion behavior: `DELETE FROM projects` with `ON DELETE CASCADE` to chats/messages/drafts. Unrelated projects remain. Recorded in ADR 0002.
* failures found/fixed: `QString()` bound as SQL NULL and violated `NOT NULL` on `default_workspace`; create now stores empty strings
* UI: sidebar project `ListView` with New/Rename/Delete; context bar shows the selected project name. App launched without QML errors.

## Status

* [x] Complete

---

# P1-T3 — Implement Chat Persistence and Selection

## Objective

Replace static chat labels with real persisted chats.

## Instructions

Support:

* create chat
* select/switch chat
* rename chat
* archive or delete chat
* display existing chats
* persist ordering/history

Chats belong to projects.

The sidebar should use an appropriate real Qt/Kirigami list/selection model rather than static labels.

Keep the existing overall sidebar concept.

Do not redesign navigation into an unrelated application structure.

## Deliverable

A working project/chat sidebar backed by persisted local data.

## Test

1. Create multiple chats under one project.
2. Create chats under another project.
3. Switch between projects.
4. Confirm only relevant chats are presented.
5. Rename a chat.
6. Archive/delete a chat.
7. Restart PikaTalk.
8. Confirm expected chats and selection/history survive.

## Completion Evidence

* list/model mechanism used: `QVariantList` of `{id, title}` from `LocalDatabase::listChatIds` (ordered by `last_active_at DESC`) exposed as `app.chats`; sidebar `ListView` (`chatList`) with `ItemDelegate` selection
* operations tested: create multiple chats per project, switch projects (filtered lists), rename, archive, delete (`database_test::chatsAreFilteredRenamedArchivedAndDeleted`, `appcontroller_test::chatsAreScopedToProjectAndSurviveRestart`)
* restart behavior: `openStore` restores the most recently active non-archived chat and its project
* archive/delete behavior: archive hides from the default list but keeps the row; delete removes the chat (CASCADE to messages/drafts). Unrelated project chats remain.

## Status

* [x] Complete

---

# P1-T4 — Persist Local Messages

## Objective

Make chats contain persistent local conversation history.

## Instructions

Support locally stored:

* user messages
* assistant messages

At this phase messages are local data only.

The user must be able to add a local user message so persistence and chat behavior can be tested.

A fake/local assistant message may be added by an explicit development/test action if necessary.

Do not:

* contact PikaClaw
* send to a model
* simulate streaming
* build retry/regenerate
* build tool UX

Replace the Phase 0 hardcoded conversation display with messages loaded from the selected chat.

## Deliverable

Conversation view backed by persistent local messages.

## Test

1. Add messages to Chat A.
2. Add different messages to Chat B.
3. Switch between chats.
4. Confirm each chat shows only its own messages.
5. Restart PikaTalk.
6. Confirm messages persist in correct order.
7. Delete/archive or modify one chat.
8. Confirm unrelated chat messages remain intact.

## Completion Evidence

* message structure used: `messages` table with `role` CHECK (`user`/`assistant`), `content`, `created_at`, and `position`
* ordering mechanism: `position` starting at 1 per chat (`MAX(position)+1`); `listMessageIds` orders by `position, id`
* persistence tests: add user/assistant messages, reopen store, messages remain in order (`database_test::messagesAreOrderedAndIsolatedPerChat`, `appcontroller_test::messagesAreIsolatedPerChatAndSurviveRestart`)
* isolation tests: Chat A messages do not appear in Chat B; deleting Chat A leaves Chat B messages intact
* UI: conversation `ListView` bound to `app.messages`; **Send** stores a local user message; **Local reply** stores a local assistant message. No PikaClaw/streaming.

## Status

* [x] Complete

---

# P1-T5 — Implement Workspace Defaults and Overrides

## Objective

Make workspace context persistent and project/chat-aware.

## Instructions

Each project may define a default workspace directory.

Each chat may override it.

When a chat is created:

* copy/inherit the project default into the chat's effective starting context as defined by the chosen persistence model

The user must be able to:

* set/change project default workspace
* set/change chat workspace override
* remove/reset an override to use project behavior if supported by the chosen model

The active workspace must be clearly visible in the context area.

Do not:

* open terminals
* open editors
* open file managers
* execute commands in the workspace
* send workspace information to PikaClaw

Those belong to later phases.

## Deliverable

Persistent project workspace defaults and per-chat overrides.

## Test

1. Set Project A default workspace.
2. Create Chat A.
3. Confirm it receives the expected workspace.
4. Override Chat A workspace.
5. Confirm the active context changes.
6. Create Chat B.
7. Confirm it follows project default behavior.
8. Restart PikaTalk.
9. Confirm all values persist correctly.

## Completion Evidence

* inheritance semantics: `chats.workspace_override` is NULL at creation, so the chat inherits the project's `default_workspace`. Changing the project default updates inheriting chats. Recorded in ADR 0002.
* override semantics: a non-NULL `workspace_override` is the effective workspace until cleared back to NULL ("Use project workspace")
* tests performed: `appcontroller_test::workspaceDefaultsAndOverridesPersist` — project default, new chat inherits, chat override, second chat still inherits, project default update, clear override, restart
* any compatibility decision recorded: none beyond ADR 0002

## Status

* [x] Complete

---

# P1-T6 — Implement Model Defaults and Overrides

## Objective

Make model selection persistent and project/chat-aware without contacting the gateway.

## Instructions

Each project may define a default model identifier.

Each chat may override it.

At this phase the model is only a locally stored string/identifier.

The user must be able to:

* set/change project default model
* set/change chat model override

New chats must inherit the project's model behavior.

The active model must be clearly visible in the context area.

Do not:

* query PikaClaw
* discover available models
* validate model availability
* contact model providers

Those belong to Phase 2.

## Deliverable

Persistent project model defaults and per-chat overrides.

## Test

1. Set Project A default model.
2. Create Chat A.
3. Confirm expected model inheritance.
4. Override Chat A model.
5. Create Chat B.
6. Confirm project default behavior.
7. Restart PikaTalk.
8. Confirm all values persist.

## Completion Evidence

* inheritance behavior: new chats have NULL `model_override` and use `projects.default_model`. Updating the project default updates inheriting chats.
* override behavior: non-NULL `model_override` is the effective model until cleared. Local string only; no gateway/discovery.
* tests performed: `appcontroller_test::modelDefaultsAndOverridesPersist` — project default, inherit, override, second chat inherits updated default, restart

## Status

* [x] Complete

---

# P1-T7 — Persist Drafts

## Objective

Prevent unfinished user input from being lost during normal local navigation or restart.

## Instructions

Persist message drafts per chat.

Draft behavior must support:

* typing into Chat A
* switching to Chat B
* returning to Chat A
* recovering Chat A draft
* application restart
* recovering unfinished drafts

Draft persistence must not create actual conversation messages until the user explicitly submits them.

Do not introduce autosend behavior.

## Deliverable

Persistent per-chat drafts.

## Test

1. Type an unfinished draft in Chat A.
2. Switch to Chat B.
3. Type a different draft.
4. Return to Chat A.
5. Confirm original draft is restored.
6. Restart PikaTalk.
7. Confirm both drafts survive.
8. Submit or clear one draft.
9. Confirm the unrelated draft remains intact.

## Completion Evidence

* persistence behavior: `drafts` table keyed by `chat_id`; `AppController::setCurrentDraft` writes on input change; switching chats loads that chat's draft
* save strategy: upsert on every draft change (`INSERT ... ON CONFLICT DO UPDATE`); empty content after Send
* switching test: `appcontroller_test::draftsSurviveSwitchAndRestartWithoutCreatingMessages` — Chat A/B drafts restore on switch; `messages` stay empty until Send
* restart test: both drafts survive reopen; submitting Chat B clears only B's draft

## Status

* [x] Complete

---

# P1-T8 — Integrate Real Local State into UI

## Objective

Replace remaining Phase 0 placeholders with real Phase 1 local state.

## Instructions

The normal application UI must now expose:

* real project selection
* real chat selection
* real conversation history
* current project
* current workspace
* current model
* message draft/input

The gateway indicator remains a static/non-functional placeholder because gateway discovery belongs to later phases.

If the context bar becomes visibly awkward now that values are real, make the smallest layout adjustment needed.

Do not perform a general redesign.

The sidebar should feel like a real selectable list rather than labels, but do not add drawers, search, tree systems, animations, or unrelated navigation concepts unless strictly required by Phase 1.

## Deliverable

A coherent local-only PikaTalk application using persisted state end-to-end.

## Test

Run a normal local workflow:

1. Create Project A.
2. Configure its workspace/model.
3. Create multiple chats.
4. Add local messages.
5. Override one chat's workspace/model.
6. Switch between chats/projects.
7. Leave drafts in multiple chats.
8. Restart PikaTalk.
9. Confirm the expected local state is restored.
10. Confirm project/workspace/model context remains clearly visible.

## Completion Evidence

* workflow tested: `appcontroller_test::localWorkflowRestoresState` — create project, set workspace/model, two chats, local messages, override one chat, drafts, reopen and restore. App launched (`./build/bin/pikatalk`) without QML errors.
* UI changes made: sidebar `ListView`s for projects and chats; conversation `ListView` for persisted messages; context bar shows live project/workspace/model; input bound to per-chat drafts; Send / Local reply
* context bar behavior: status labels stay on the first row (Project, Workspace, Model, Gateway: Offline). Workspace/model actions wrap on a second row so values stay readable. Gateway remains a static Offline placeholder.
* sidebar behavior: selectable `ItemDelegate` lists; New/Rename/Delete for projects; New/Rename/Archive/Delete for chats; chats filtered by selected project
* defects found/fixed: none in this task

## Status

* [x] Complete

---

# P1-T9 — Isolation and Regression Tests

## Objective

Prove that local state operations do not damage unrelated data and that Phase 0 foundation remains healthy.

## Instructions

Test project/chat isolation and restart behavior thoroughly.

Add automated tests where they provide useful coverage, particularly for:

* database operations
* inheritance
* overrides
* deletion/archive behavior
* draft persistence

Do not create a large generic testing framework.

## Test

At minimum:

1. Maintain at least two projects.
2. Maintain multiple chats in each.
3. Give chats different messages, drafts, workspaces, and models.
4. Rename one chat.
5. Delete/archive another.
6. Change one project's defaults.
7. Restart.
8. Confirm unrelated project/chat data remains unchanged.
9. Run the full existing test suite.
10. Confirm Phase 0 path/database/application startup functionality has not regressed.

## Completion Evidence

* automated tests added: `appcontroller_test::isolationAcrossProjectsSurvivesRestart`; existing `database_test` and `applicationpaths_test` still run
* manual scenarios tested: application launch after T8; automated coverage for two projects, per-chat messages/drafts/workspace/model, rename, archive+delete, project default change, restart
* full test-suite result: `ctest --test-dir build --output-on-failure` — 4/4 passed (appstreamtest, applicationpaths_test, database_test, appcontroller_test)
* regressions found/fixed: none

## Status

* [x] Complete

---

# P1-T10 — Phase 1 Documentation Check

## Objective

Ensure future work can understand the local persistence behavior without reverse-engineering it.

## Instructions

Document only durable knowledge future phases need.

At minimum document:

* Phase 1 database location
* schema/versioning approach
* project/chat relationship
* message representation
* workspace inheritance/override semantics
* model inheritance/override semantics
* draft behavior

Do not document PikaClaw API assumptions.

Do not document future functionality as implemented.

Update existing README/development documentation only where Phase 1 changes actual behavior.

## Deliverable

Current documentation matching Phase 1 reality.

## Test

Review documentation against the running implementation and current schema.

Confirm no documentation claims Phase 2 functionality exists.

## Completion Evidence

* docs created/updated: `README.md` (Phase 1 status), `docs/development.md` (database path and tests), `docs/local-storage.md` (schema, inheritance, drafts)
* schema documentation location: `docs/local-storage.md` and `decisions/0002-local-sqlite-schema.md`
* inaccuracies corrected: removed Phase 0 claims that `phase0.sqlite` is the active database and that conversation content is fake/static

## Status

* [x] Complete

---

# P1-T11 — Phase Handoff Validation

## Objective

Determine whether Phase 1 satisfies its immutable contract.

This task adds no functionality.

## Instructions

Read **Phase 1 — Local Chat State** in `PHASES.md`.

Validate every acceptance criterion individually.

Create:

`docs/handoffs/phase-1.md`

The report must contain:

### Deliverables

Meaningful files/artifacts produced.

### Tests Performed

Actual tests executed.

### Results

Pass/fail for every Phase 1 acceptance criterion.

### Known Limitations

Known incomplete or fragile behavior.

### Deferred Work

All valid discoveries outside Phase 1.

### Decisions

References to relevant ADRs.

## Test

Compare the completed implementation against all 12 Phase 1 acceptance criteria.

If any criterion fails:

* Phase 1 remains active.
* Record the failure.
* Create/refine a Phase 1 task needed to fix it.
* Do not weaken the Phase 1 contract.

Final result must be:

```text
PHASE 1 HANDOFF: PASS
```

or:

```text
PHASE 1 HANDOFF: FAIL
```

## Completion Evidence

Final result: `PHASE 1 HANDOFF: PASS`

Handoff document: `docs/handoffs/phase-1.md`

All 12 Phase 1 acceptance criteria passed. Phase 2 was not started.

## Status

* [x] Complete

---

# Deferred Work

Record valid discoveries outside Phase 1 here.

Do not implement them.

Format:

```text
- [date] Short description
  - Discovered while: P1-Tx
  - Suggested phase: Phase X / Future
  - Reason deferred: outside Phase 1 contract
```

Examples of things that must be deferred if encountered:

* PikaClaw API/gateway communication → Phase 2
* real model discovery → Phase 2
* streaming / stop / regenerate → Phase 2
* tool calls/results → Phase 3
* open workspace in terminal/editor/file manager → Phase 3
* gateway start/stop/restart → Phase 4
* search, tray, notifications, polish → Phase 5
* conversation branching → Future

---

# STOP CONDITION

When `P1-T11` passes:

**STOP.**

Do not create the Phase 2 task file.

Do not investigate or connect to PikaClaw.

Do not implement model discovery, streaming, tools, or gateway management.

Do not modify the Phase 2 contract.

Phase 2 requires a new explicit execution instruction.
