# Phase 3 Tasks — Agent and Workspace UX

Phase contract: `PHASES.md` → **Phase 3 — Agent and Workspace UX**

This file contains only implementation work for Phase 3.

## Execution Rules

1. Read `PROJECT.md`.
2. Read the Phase 3 contract in `PHASES.md`.
3. Read `docs/handoffs/phase-2.md`.
4. Read `docs/pikaclaw-api.md`.
5. Read ADR 0002 and ADR 0003.
6. Work on tasks in order.
7. Work on only one task at a time.
8. Do not implement anything listed under Phase 3 exclusions.
9. Do not begin Phase 4 work.
10. Discoveries outside Phase 3 go under **Deferred Work**.
11. Mark a task complete only after its test passes.
12. Record completion evidence under each task.
13. Completing all tasks does **not** automatically complete Phase 3.
14. Perform Phase 3 handoff validation after all implementation tasks.
15. After handoff validation, STOP.

## Phase 3 Guardrails

Phase 3 is intentionally narrow.

It adds only:

* persistence of PicoClaw tool activity
* compact tool activity UI
* copy message text
* copy code blocks
* open active workspace in:

  * file manager
  * terminal
  * editor

Do **not** use this phase to solve PicoClaw 0.3.1 ignoring `payload.workspace`.

The workspace used by desktop actions is the existing PikaTalk active workspace shown in the context area.

Do not introduce a second workspace concept.

Do not create:

* gateway lifecycle controls
* plugin/tool framework
* tool configuration UI
* approval/workflow system
* generic remote filesystem support
* RAG or knowledge-base features
* search
* branching
* notifications
* tray integration

---

# P3-T1 — Discover PicoClaw Tool Event Behavior

## Objective

Determine the actual PicoClaw 0.3.1 wire behavior needed to preserve tool activity.

## Instructions

Extend the Phase 2 protocol investigation only as far as required for Phase 3.

Observe real tool-using turns and determine where available:

* tool-call event type / `payload.kind`
* tool name
* tool inputs/arguments
* call identifier
* tool result
* success/failure state
* error information
* relationship between tool activity and the user-visible assistant message
* event ordering
* whether calls/results arrive as create/update events

Use real gateway evidence where possible.

Do not build the UI yet.

Do not investigate generic plugin/tool configuration.

## Deliverable

Update:

`docs/pikaclaw-api.md`

with the observed tool-event behavior.

Create an ADR only if a persistence/protocol decision future work must respect is required.

## Test

1. Run at least one real request that invokes a tool.
2. Capture sanitized tool-call information.
3. Capture sanitized tool-result information.
4. Exercise a failed tool call where practical.
5. Confirm which protocol fields are stable enough to persist.

## Completion Evidence

Recorded 2026-08-15.

* PicoClaw version tested: 0.3.1 (`picoclaw gateway -E` on `127.0.0.1:18790`)
* Tool event types observed: WebSocket `message.create` with `payload.kind = "tool_calls"`; no WebSocket tool-result event
* Identifiers observed: `payload.message_id`; `tool_calls[].id` (e.g. `chatcmpl-tool-…`); session JSONL `tool_call_id` matches that id
* Call/result relationship: call on Pico WS; result only in `~/.picoclaw/workspace/sessions/*.jsonl` as `role:tool`
* Success/error representation: success = tool listing/text in session `content`; failure example `failed to read directory: path escapes workspace: /home/naorw` (no separate status field)
* Fields deliberately ignored: thoughts; session user/assistant history rows as chat source of truth
* Protocol limitations: documented in `docs/pikaclaw-api.md`; ADR `decisions/0004-tool-activity-persistence.md`

## Status

* [x] Complete

---

# P3-T2 — Add Tool Activity Persistence Schema

## Objective

Persist tool activity without replacing the existing message model.

## Instructions

Extend `pikatalk.sqlite` using the existing schema migration mechanism.

Persist enough raw information to reconstruct useful tool activity after restart.

Where PicoClaw exposes them, preserve:

* associated chat/message or turn relationship
* tool call identifier
* tool name
* raw input/arguments
* raw result
* execution status
* relevant error information
* ordering

Do not flatten away useful raw fields merely because the first UI does not display them.

Prefer a small dedicated table/table set linked to existing conversation records.

Do not replace `messages`.

Do not build a generic tool schema intended for arbitrary future providers.

## Deliverable

A documented schema migration supporting durable PicoClaw tool activity.

## Test

1. Migrate a Phase 2 schema database forward.
2. Confirm existing projects/chats/messages/drafts survive unchanged.
3. Persist one successful tool call/result.
4. Persist one failed/error tool event.
5. Reopen the database.
6. Confirm all raw stored information survives.
7. Confirm deleting the owning chat cleans up its tool records appropriately.

## Completion Evidence

Recorded 2026-08-15.

* Schema version: **2**
* Migration: v1 → v2 creates `tool_activities` and updates `schema_version` (`migratesSchemaV1ToV2AndPersistsToolActivity`)
* Tables/columns: `tool_activities(id, chat_id, message_id, tool_call_id, tool_name, arguments_json, raw_call_json, result_text, status, error_text, created_at, position)`
* Relationship: FK to `chats` CASCADE; optional FK to `messages` SET NULL
* Success/error round-trip: ok and error rows survive reopen; chat delete clears tool rows
* ADR: `decisions/0004-tool-activity-persistence.md`

## Status

* [x] Complete

---

# P3-T3 — Capture Tool Activity from Pico Protocol

## Objective

Stop discarding PicoClaw tool events and persist them during real chat execution.

## Instructions

Extend the existing isolated Pico Protocol/client/controller path to recognize the tool behavior documented in P3-T1.

When PicoClaw reports tool activity:

* preserve the useful raw event information
* associate it with the correct active chat/turn
* persist it through the Phase 3 schema

Normal assistant message persistence must continue to behave as in Phase 2.

Thought/reasoning content remains outside Phase 3 unless strictly required to associate tool activity correctly.

Do not turn tool events into normal assistant messages.

Do not implement tool configuration or execution controls.

## Deliverable

Tool calls/results from a real PikaClaw conversation are stored locally.

## Test

1. Run a real tool-using conversation.
2. Confirm the tool call is persisted.
3. Confirm the tool result is persisted.
4. Confirm ordering/association is correct.
5. Reload PikaTalk.
6. Confirm the same records remain available.
7. Confirm a normal non-tool chat still behaves exactly as before.

## Completion Evidence

Record:

* real tool tested
* event sequence
* persisted data
* reload result
* Phase 2 chat regression result

**Evidence (2026-08-15):**

* Unit: `./build/bin/appcontroller_test persistsToolCallsAndResultsFromGateway` — PASS. Fake gateway emits `message.create`/`kind=tool_calls`; controller persists `list_dir`; session JSONL `role=tool` enriches `resultText`/`status=ok`; reload via new `AppController` keeps the same record; assistant message remains separate (`messages().size()==2`).
* Live: `PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewayToolActivityIfEnabled` — PASS (~16s). Real PicoClaw 0.3.1 tool call persisted (`toolName` non-empty, `status` ok/error, `resultText` non-empty); then assistant reply containing `TOOLOK`.
* Phase 2 regression: `PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewaySendIfEnabled` — PASS (`LIVEOK` assistant reply, no tool activity required).
* Event sequence (observed): WS `tool_calls` → SQLite `tool_activities` (`running`) → poll session JSONL by `tool_call_id` → update `result_text`/`status` → assistant text message persisted separately; `message_id` linked when assistant row lands.

## Status

* [x] Complete

---

# P3-T4 — Display Compact Tool Activity

## Objective

Make tool activity understandable without exposing raw Pico Protocol noise by default.

## Instructions

Display persisted tool activity in the conversation near the relevant turn/message.

Default presentation should be compact.

At minimum make understandable:

* that a tool was used
* tool name
* success / failure state

Provide an expandable/collapsible detail view where useful.

Expanded details may show:

* input/arguments
* result
* error information

Raw protocol fields may remain stored without all being rendered.

Do not display reasoning/thought events as part of this task.

Do not build:

* tool configuration
* approvals
* plugin controls
* workflow visualization
* execution replay

## Deliverable

Compact persisted tool activity visible in the normal conversation UI.

## Test

1. View a successful tool call.
2. Confirm default representation is concise.
3. Expand it.
4. Confirm useful input/result information is available.
5. View a failed tool call.
6. Confirm failure is understandable.
7. Restart PikaTalk.
8. Confirm tool activity renders from local persistence.

## Completion Evidence

Record:

* UI representation used
* success case
* failure case
* reload behavior
* raw fields retained but not displayed

**Evidence (2026-08-15):**

* UI: conversation `ListView` interleaves `app.toolActivities` with messages (tools linked by `messageId` appear before the assistant bubble). Compact summary button `Tool: <name> — ok|failed|running`; click expands Input/Result/Error. `rawCallJson` is loaded on the model but not rendered.
* Success/failure/reload fields: `./build/bin/appcontroller_test toolActivityExposesCompactSuccessAndFailureFields` — PASS (`list_dir` ok + error with escapes-workspace text; reload via new controller; `rawCallJson` present).
* Build: `pikatalk` rebuilt with updated `Main.qml`.

## Status

* [x] Complete

---

# P3-T5 — Copy Message Text

## Objective

Allow normal conversation text to be copied easily.

## Instructions

Add a simple copy action for persisted user and assistant messages.

Use the native Qt/KDE clipboard.

Copy the actual message text, not presentation decoration.

Do not add sharing/export features.

## Deliverable

Message text can be copied to the system clipboard.

## Test

1. Copy a user message.
2. Paste into another application.
3. Confirm exact useful text.
4. Copy an assistant message.
5. Confirm the same behavior.
6. Confirm copy actions do not mutate chat history.

## Completion Evidence

Record:

* clipboard mechanism
* user-message test
* assistant-message test

**Evidence (2026-08-15):**

* Mechanism: `AppController::copyText` → `QGuiApplication::clipboard()->setText` when a GUI app exists; always records `lastCopiedText` for verification.
* UI: per-message **Copy** button in `Main.qml` copies `modelData.data.content` (raw message text).
* Test: `./build/bin/appcontroller_test copyTextPutsExactMessageOnClipboardBufferWithoutMutatingHistory` — PASS (user + assistant exact text; message list unchanged).

## Status

* [x] Complete

---

# P3-T6 — Copy Code Blocks

## Objective

Allow code shown inside assistant messages to be copied without manually selecting surrounding prose.

## Instructions

Identify code blocks in displayed message content using the existing message rendering approach.

Provide a copy action for each code block where practical.

Copy only the code contents.

Keep implementation small.

Do not introduce a general Markdown/document engine unless the existing rendering genuinely requires it.

Do not add syntax-highlighting infrastructure unless already available cheaply and required for the current UI.

## Deliverable

Visible code blocks have a usable copy action.

## Test

1. Display a message containing one fenced code block.
2. Copy the code.
3. Confirm surrounding prose/fences are not copied.
4. Display a message with multiple code blocks.
5. Confirm the intended individual block can be copied.
6. Confirm ordinary messages remain unaffected.

## Completion Evidence

Record:

* code-block detection/rendering approach
* single-block test
* multiple-block test
* clipboard result

**Evidence (2026-08-15):**

* Approach: `splitMessageSegments()` parses fenced ``` blocks; QML renders text vs `codeBlock` frames with **Copy code** calling `app.copyText(code only)`.
* Test: `./build/bin/appcontroller_test messageSegmentsSplitFencedCodeBlocks` — PASS (single: before/code/after; multiple: two code segments; plain text unchanged).

## Status

* [x] Complete

---

# P3-T7 — Open Active Workspace in File Manager

## Objective

Open exactly the workspace currently shown in PikaTalk using the local desktop file manager.

## Instructions

Use the existing effective active workspace from Phase 1/2.

Use an appropriate Linux/KDE desktop mechanism.

Prefer desktop-native URL/file handling rather than hardcoding Dolphin if a standard mechanism correctly opens the directory.

Before launching:

* confirm an active workspace exists
* confirm it refers to a usable local directory where practical

Failure should be visible and non-destructive.

Do not introduce remote filesystem support.

## Deliverable

A quick action opens the active workspace in the desktop file manager.

## Test

1. Use a real project directory.
2. Trigger Open in File Manager.
3. Confirm the opened directory is exactly the active workspace.
4. Test a chat workspace override.
5. Confirm the override directory is opened.
6. Test missing/invalid path behavior.

## Completion Evidence

Record:

* launch mechanism
* project-default test
* chat-override test
* invalid-path behavior

**Evidence (2026-08-15):**

* Mechanism: `prepareOpenWorkspaceInFileManager` → `xdg-open` / `QDesktopServices::openUrl(fromLocalFile)` on `currentWorkspace`.
* Unit: `workspaceLaunchersUseActiveWorkspacePath` — PASS (project default + chat override absolute paths; invalid path error).
* Real dir: `xdg-open /tmp/pikatalk-ws-*` exit 0.

## Status

* [x] Complete

---

# P3-T8 — Open Active Workspace in Terminal

## Objective

Open a terminal rooted in the active PikaTalk workspace.

## Instructions

Use the existing active workspace.

Use an appropriate local terminal integration for the target openSUSE / KDE Plasma environment.

Keep the launcher isolated and simple.

If terminal choice requires configuration, use only the minimum local setting needed rather than creating a general application-launch framework.

The launched terminal must start in the active workspace.

Do not execute arbitrary project commands automatically.

## Deliverable

A quick action opens a local terminal in the active workspace.

## Test

1. Set a real workspace.
2. Trigger Open in Terminal.
3. Confirm the terminal starts in that directory.
4. Test a chat workspace override.
5. Confirm it opens in the overridden directory.
6. Test invalid-path behavior.

## Completion Evidence

Record:

* terminal mechanism
* terminal tested
* working-directory evidence
* override test
* failure behavior

**Evidence (2026-08-15):**

* Mechanism: default `konsole --workdir <activeWorkspace>` (override via `desktop/terminalCommand` in QSettings).
* Unit: `workspaceLaunchersUseActiveWorkspacePath` — PASS (`--workdir` includes absolute workspace; empty path rejected).
* Real: `konsole --workdir /tmp/pikatalk-ws-*` started; `/proc/<pid>/cmdline` contains `--workdir` and that directory.

## Status

* [x] Complete

---

# P3-T9 — Open Active Workspace in Editor

## Objective

Open the active workspace using the configured/default local editor.

## Instructions

Support one simple editor launch path.

Use either:

* an explicitly configured local editor command, or
* an appropriate desktop/default-editor mechanism if it reliably opens a directory as a project/workspace

Do not build an editor marketplace or integrations for many editors.

If configuration is needed, keep it minimal and local.

The action must operate on the same active workspace shown in PikaTalk.

Do not execute arbitrary shell strings unsafely.

## Deliverable

A quick action opens the active workspace in the user's configured/default editor.

## Test

1. Configure/use the local editor.
2. Open a real workspace.
3. Confirm the correct directory is opened.
4. Test a per-chat workspace override.
5. Confirm the overridden directory is used.
6. Test missing editor / invalid path behavior.

## Completion Evidence

Record:

* editor selection mechanism
* editor tested
* project-default workspace result
* chat-override result
* failure behavior

**Evidence (2026-08-15):**

* Mechanism: default `kate <activeWorkspace>` (override via `desktop/editorCommand`); same prepare/validate path as other launchers.
* Unit: `workspaceLaunchersUseActiveWorkspacePath` — PASS (project + override paths; invalid directory rejected).
* Real: `kate /tmp/pikatalk-ws-*` launched (process handoff to existing Kate session acceptable).

## Status

* [x] Complete

---

# P3-T10 — Integrate Workspace Actions into UI

## Objective

Expose the three workspace actions without cluttering or redesigning PikaTalk.

## Instructions

Provide clear actions for:

* Open in File Manager
* Open in Terminal
* Open in Editor

Place them where they naturally relate to the visible active workspace.

The context shown to the user and the directory passed to every launcher must come from the same active workspace state.

Do not add a second workspace selector.

Do not redesign the whole context bar.

Do not implement gateway workspace mutation.

## Deliverable

All three workspace actions are accessible from the normal PikaTalk UI.

## Test

1. Set project workspace.
2. Confirm context shows it.
3. Launch all three actions.
4. Confirm all three use that path.
5. Apply a chat override.
6. Confirm context changes.
7. Launch all three again.
8. Confirm all three now use the override.

## Completion Evidence

Record:

* UI placement
* path consistency test
* project-default behavior
* chat-override behavior

**Evidence (2026-08-15):**

* UI: context toolbar buttons **Open folder** / **Open terminal** / **Open editor** next to workspace/model controls; enabled when `app.currentWorkspace` is set; errors via `workspaceActionError` label. All three call AppController methods that use `m_currentWorkspace` only (same as context label).
* Path consistency: `workspaceLaunchersUseActiveWorkspacePath` — PASS for project default then chat override.

## Status

* [x] Complete

---

# P3-T11 — Regression and Integration Tests

## Objective

Prove Phase 3 additions do not damage Phase 2 chat behavior or local-state isolation.

## Instructions

Add focused automated tests for:

* tool-event parsing
* tool persistence
* tool reload
* successful/failed tool activity
* workspace action path selection
* copy behavior where practical

Use the existing fake Pico server and test architecture where appropriate.

Run real PicoClaw tool evidence in addition to automated tests.

Do not create a generic testing framework.

## Test

At minimum:

1. Run complete existing `ctest` suite.
2. Perform a normal non-tool real chat.
3. Perform a real tool-using chat.
4. Confirm tool data persists.
5. Restart and reload it.
6. Exercise failed tool representation where available.
7. Verify copy message.
8. Verify copy code block.
9. Verify file manager / terminal / editor against a real project directory.
10. Confirm drafts, model selection, workspace inheritance, streaming, stop, retry and reconnect still behave correctly.

## Completion Evidence

Record:

* tests added
* full `ctest` result
* real gateway/tool tests
* real workspace-launch tests
* regressions found/fixed

**Evidence (2026-08-15):**

* Tests added: `persistsToolCallsAndResultsFromGateway`, `toolActivityExposesCompactSuccessAndFailureFields`, `copyTextPutsExactMessageOnClipboardBufferWithoutMutatingHistory`, `messageSegmentsSplitFencedCodeBlocks`, `workspaceLaunchersUseActiveWorkspacePath`, `openWorkspaceActionsLaunchAgainstRealDirectory` (PIKATALK_LIVE_DESKTOP), `liveGatewayToolActivityIfEnabled`.
* `ctest --test-dir build`: **5/5 PASS** (~1.9s).
* Live gateway: `PIKATALK_LIVE_GATEWAY=1` `liveGatewaySendIfEnabled` + `liveGatewayToolActivityIfEnabled` — PASS.
* Real workspace: `PIKATALK_LIVE_DESKTOP=1 openWorkspaceActionsLaunchAgainstRealDirectory` — PASS; smoke script `REAL_WORKSPACE_LAUNCH: PASS` (`xdg-open` exit 0; `konsole --workdir <dir>`).
* Regression fix: file manager launch uses `xdg-open` via `QProcess::startDetached` (QDesktopServices fails without GUI app).

## Status

* [x] Complete

---

# P3-T12 — Phase 3 Documentation Check

## Objective

Preserve only the durable knowledge future work needs.

## Instructions

Update documentation to describe:

* observed PicoClaw tool-event schema
* Phase 3 tool persistence
* relationship between tool activity and normal messages
* workspace action behavior
* editor configuration if introduced
* known limitations

Update SQLite/storage documentation if schema changes.

Do not document:

* gateway lifecycle controls
* plugin/tool configuration systems
* RAG
* workflows
* Phase 4 functionality as implemented

## Deliverable

Documentation matching actual Phase 3 behavior.

## Test

Compare documentation against:

* current schema
* real PicoClaw tool events
* UI behavior
* workspace launcher behavior

Correct unsupported assumptions.

## Completion Evidence

Record:

* docs created/updated
* schema docs updated
* protocol docs updated
* ADRs referenced

**Evidence (2026-08-15):**

* Updated `docs/pikaclaw-api.md` — Phase 3 tool persistence/UI + desktop workspace actions; removed outdated “tool UX out of scope” line.
* Updated `docs/local-storage.md` — tool_activities cascade note; Phase 3 desktop actions + copy behavior; terminal/editor QSettings keys.
* ADRs: `decisions/0004-tool-activity-persistence.md` (Phase 3); schema still governed by `0002` + `0003` transport ownership.

## Status

* [x] Complete

---

# P3-T13 — Phase Handoff Validation

## Objective

Determine whether Phase 3 satisfies its immutable contract.

This task adds no functionality.

## Instructions

Read **Phase 3 — Agent and Workspace UX** in `PHASES.md`.

Validate every acceptance criterion individually.

Create:

`docs/handoffs/phase-3.md`

The report must contain:

### Deliverables

Meaningful files/artifacts produced.

### Tests Performed

Actual tests executed, including real PicoClaw tool activity and real workspace actions.

### Results

Pass/fail for all 11 Phase 3 acceptance criteria.

### Known Limitations

Known incomplete or fragile behavior.

### Deferred Work

All valid discoveries outside Phase 3.

### Decisions

References to relevant ADRs.

## Test

Compare the implementation against every Phase 3 acceptance criterion.

If any criterion fails:

* Phase 3 remains active.
* Record the failure.
* Create/refine a Phase 3 task required to fix it.
* Do not weaken the contract.

Final result must be:

```text
PHASE 3 HANDOFF: PASS
```

or:

```text
PHASE 3 HANDOFF: FAIL
```

## Completion Evidence

Record final result and handoff document path.

**Evidence (2026-08-15):**

* Handoff: `docs/handoffs/phase-3.md`
* All 11 Phase 3 acceptance criteria PASS
* Real PicoClaw tool activity + real workspace directory launches recorded
* Final result:

```text
PHASE 3 HANDOFF: PASS
```

## Status

* [x] Complete

---

# Deferred Work

Record valid discoveries outside Phase 3 here.

Do not implement them.

Examples:

* changing PicoClaw execution workspace semantics → Future / gateway capability, not Phase 3 workspace actions
* gateway start/stop/restart → Phase 4
* detailed gateway monitoring → Phase 4
* tool configuration / approvals / generic plugin system → Future unless explicitly contracted
* search / tray / notifications → Phase 5
* conversation branching → Future
* RAG / knowledge-base management → Out of current scope
* arbitrary remote filesystem support → Out of current scope

---

# STOP CONDITION

When `P3-T13` passes:

**STOP.**

Do not create the Phase 4 task file.

Do not implement gateway start/stop/restart.

Do not implement generic tool configuration or plugins.

Do not attempt to make PicoClaw 0.3.1 execute inside the per-chat workspace unless a future phase explicitly authorizes that work.

Do not modify the Phase 4 contract.

Phase 4 requires explicit authorization after review.
