# Phase 3 Handoff — Agent and Workspace UX

## Deliverables

Meaningful files produced or updated:

* `src/database.h` / `src/database.cpp` — schema version `2` with `tool_activities`; migrate v1→v2
* `src/pikaclawsettings.h` / `src/pikaclawsettings.cpp` — default workspace/sessions paths; `loadPicoClawToolResults` (JSONL by `tool_call_id`)
* `src/appcontroller.h` / `src/appcontroller.cpp` — persist/reload tool activity; link to assistant `message_id`; copy text; message segments; workspace open actions
* `src/messageformatting.h` / `src/messageformatting.cpp` — fenced code-block segmentation
* `src/workspaceactions.h` / `src/workspaceactions.cpp` — prepare/launch file manager, terminal, editor for the active workspace
* `src/Main.qml` — compact expandable tool rows; Copy / Copy code; Open folder / terminal / editor
* `src/CMakeLists.txt` / `tests/CMakeLists.txt` — new sources; tests link `Qt6::Gui`
* `tests/database_test.cpp` — v1→v2 migration + tool activity round-trip
* `tests/appcontroller_test.cpp` — tool capture, compact fields, copy, code segments, workspace path selection, live tool/desktop gates
* `docs/pikaclaw-api.md` — observed tool-call WS + session JSONL results; PikaTalk persistence/UI; desktop actions
* `docs/local-storage.md` — schema v2 / `tool_activities`; Phase 3 desktop/copy notes
* `decisions/0004-tool-activity-persistence.md`
* `decisions/0002-local-sqlite-schema.md` — note version `2` / ADR 0004
* `tasks/phase-3-agent-and-workspace-ux.md` — execution evidence
* `docs/handoffs/phase-3.md` — this report

Built artifact:

* `build/bin/pikatalk`

Gateway used for live tool evidence:

* PicoClaw 0.3.1 (`picoclaw gateway -E`)
* WebSocket `ws://127.0.0.1:18790/pico/ws?session_id=pikatalk-chat-<id>`

PikaTalk continues to own projects, chats, messages, drafts, workspace state, selected model state, and tool activity records in `pikatalk.sqlite`. PicoClaw remains execution/gateway only. Desktop workspace actions use the **same** PikaTalk `currentWorkspace` shown in the context bar (not PicoClaw’s execution root).

## Tests Performed

* `cmake --build build`
* `ctest --test-dir build --output-on-failure` — **5/5 PASS** (~1.9s):

  * appstreamtest
  * applicationpaths_test
  * database_test
  * appcontroller_test
  * pikaclawclient_test

* Fake-gateway tool persistence:

  ```bash
  ./build/bin/appcontroller_test persistsToolCallsAndResultsFromGateway
  ```

  **PASS** — WS `kind=tool_calls` → SQLite; session JSONL enriches result; reload keeps record; assistant message separate.

* Compact success/failure fields + reload:

  ```bash
  ./build/bin/appcontroller_test toolActivityExposesCompactSuccessAndFailureFields
  ```

  **PASS** — `ok` and `error` (`path escapes workspace`) with retained `rawCallJson`.

* Copy + code blocks:

  ```bash
  ./build/bin/appcontroller_test copyTextPutsExactMessageOnClipboardBufferWithoutMutatingHistory messageSegmentsSplitFencedCodeBlocks
  ```

  **PASS**

* Workspace path selection (project default + chat override + invalid path):

  ```bash
  ./build/bin/appcontroller_test workspaceLaunchersUseActiveWorkspacePath
  ```

  **PASS**

* Real desktop launches against a real local project directory:

  ```bash
  PIKATALK_LIVE_DESKTOP=1 ./build/bin/appcontroller_test openWorkspaceActionsLaunchAgainstRealDirectory
  ```

  **PASS** — file manager (`xdg-open`), terminal (`konsole --workdir`), editor (`kate`) for project workspace then chat override.

  Additional smoke: `xdg-open` exit 0; `/usr/bin/konsole --workdir /tmp/pikatalk-ws-*` cmdline confirmed.

* Real PicoClaw chats:

  ```bash
  PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewaySendIfEnabled liveGatewayToolActivityIfEnabled
  ```

  **PASS** — non-tool `LIVEOK`; tool-using turn persisted tool name/status/result then `TOOLOK` assistant text.

## Results

Phase 3 acceptance criteria from `PHASES.md`:

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Tool calls returned by PikaClaw are persisted locally | PASS | Live `liveGatewayToolActivityIfEnabled`; unit `persistsToolCallsAndResultsFromGateway`; schema `tool_activities` |
| 2 | Tool results returned by PikaClaw are persisted locally | PASS | Session JSONL enrichment by `tool_call_id`; live non-empty `resultText` |
| 3 | Tool activity understandable in UI without raw protocol noise by default | PASS | Compact `Tool: name — ok\|failed\|running`; expand for input/result; `rawCallJson` not rendered |
| 4 | Stored tool information remains available after restart | PASS | Reopen controller / DB reload tests |
| 5 | Copying normal message text works | PASS | `copyText` + QML Copy; unit test exact text, history unchanged |
| 6 | Copying code blocks works | PASS | `splitMessageSegments` + Copy code; fences/prose excluded |
| 7 | Active workspace opened in file manager | PASS | `xdg-open` via `openWorkspaceInFileManager`; live desktop test |
| 8 | Active workspace opened in a terminal | PASS | `konsole --workdir`; live desktop test |
| 9 | Active workspace opened in configured/default editor | PASS | `kate` (optional `desktop/editorCommand`); live desktop test |
| 10 | Workspace actions operate on workspace shown in active context | PASS | All launchers use `m_currentWorkspace` only; override changes path for all three |
| 11 | Existing Phase 2 chat behavior does not regress | PASS | Full `ctest`; live `LIVEOK` send; Phase 2 client/controller tests still green |

## Known Limitations

* PicoClaw 0.3.1 still ignores `payload.workspace` for agent execution. Phase 3 does not “fix” that; desktop actions use PikaTalk’s active workspace independently.
* Tool **results** are not on the Pico WebSocket; enrichment depends on finding the matching session JSONL via `direct:pico:<session_id>`.
* Result `status` is inferred from result text heuristics (`failed` / `error` / `escapes workspace`, etc.), not a wire boolean.
* Thought/reasoning events remain ignored.
* `QClipboard` requires a GUI application; guiless tests verify `lastCopiedText` only.
* Default terminal/editor assume KDE (`konsole`, `kate`); overrides are minimal QSettings keys only.
* Live desktop launches can open real windows; gated behind `PIKATALK_LIVE_DESKTOP=1` so default `ctest` stays fast.

## Deferred Work

* Gateway start/stop/restart and detailed monitoring → Phase 4
* Making PicoClaw honor per-request `payload.workspace` → gateway capability / future
* Tool configuration, approvals, plugins, workflows, replay → Future
* Search, tray, notifications, packaging polish → Phase 5
* Conversation branching → Future
* RAG / knowledge-base / remote filesystem → Out of scope

## Decisions

* `decisions/0002-local-sqlite-schema.md` — local ownership; schema versions; Phase 3 adds v2
* `decisions/0003-pico-protocol-chat-transport.md` — Pico Protocol remains the only chat transport; session files are not history
* `decisions/0004-tool-activity-persistence.md` — dedicated `tool_activities`; enrich results from session JSONL by `tool_call_id` only

```text
PHASE 3 HANDOFF: PASS
```
