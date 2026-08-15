# Phase 2 Tasks — PikaClaw Chat

Phase contract: `PHASES.md` → **Phase 2 — PikaClaw Chat**

This file contains only implementation work for Phase 2.

## Execution Rules

1. Read `PROJECT.md`.
2. Read the Phase 2 contract in `PHASES.md`.
3. Read `docs/handoffs/phase-1.md`.
4. Work on tasks in order.
5. Work on only one task at a time.
6. Do not implement anything listed under Phase 2 exclusions.
7. Do not begin Phase 3 work.
8. Discoveries outside Phase 2 go under **Deferred Work**.
9. Mark a task complete only after its test passes.
10. Record completion evidence under each task.
11. Completing all tasks does **not** automatically complete Phase 2.
12. Perform Phase 2 handoff validation after all implementation tasks.
13. After handoff validation, STOP.

## Phase 2 Guardrails

PikaTalk continues to own:

* projects
* chats
* message history
* drafts
* workspace state
* selected model state

PikaClaw is the execution/gateway layer.

Do not move conversation ownership into the gateway.

Do not bypass PikaClaw and connect directly to model providers.

Do not implement gateway start/stop/restart. That is Phase 4.

Tool events may be preserved only as required to avoid breaking normal chat operation. Full tool persistence and UX belongs to Phase 3.

---

# P2-T1 — Discover PikaClaw Chat API

## Objective

Determine the smallest real PikaClaw API surface PikaTalk needs for Phase 2.

## Instructions

Inspect the locally available PikaClaw gateway and its documentation/source where necessary.

Determine:

* gateway endpoint
* chat/request endpoint
* request format
* streaming mechanism
* cancellation mechanism
* model discovery endpoint, if available
* how workspace is supplied
* how conversation/session context is supplied
* error behavior
* connection/reconnection expectations

Prefer actual observed behavior over assumptions.

Do not implement client behavior yet beyond tiny probes needed for discovery.

Do not investigate gateway process management.

## Deliverable

Create:

`docs/pikaclaw-api.md`

Document only behavior required by PikaTalk.

Include sanitized request/response examples where useful.

If any protocol choice future phases must respect is architectural rather than merely factual, create an ADR.

## Test

1. Confirm gateway can be reached.
2. Perform a minimal successful request outside the UI.
3. Confirm streaming behavior.
4. Confirm model discovery if exposed.
5. Confirm what happens when gateway is unavailable.
6. Confirm cancellation mechanism if exposed.

## Completion Evidence

Recorded 2026-08-15 on the development machine.

* Gateway/version: PicoClaw 0.3.1 (`git: 2cf030d2`), process `picoclaw gateway -E` on `127.0.0.1:18790`
* Endpoints: `GET /health` (no auth, 200); `GET /pico/ws` (401 without token, 101 with Bearer); `GET /pico` 404; `GET /v1/models` 404; launcher `127.0.0.1:18800/api/models` 401
* Transport: Pico Protocol WebSocket `ws://127.0.0.1:18790/pico/ws`
* Successful request: CLI `picoclaw agent -m ...` returned `PONG`; WS `message.send` returned `STREAMOK`
* Streaming: live short/medium replies arrived as `typing.start` → `typing.stop` → thought `message.create` → final `message.create`. Source also defines `message.update`. No `[DONE]` / `turn.done`
* Cancellation: `message.send` content `/stop` on the same session returned `Task stopped. "..." was canceled.`
* Model discovery: not on the chat HTTP port; `model_list[].model_name` in `~/.picoclaw/config.json`. Inbound `payload.model_name` ignored; `/switch model to <name>` listed by `/help`
* Workspace: not consumed from `message.send`; agent default `~/.picoclaw/workspace`
* Session: `session_id` query/field; PikaTalk remains history owner
* Unreachable: TCP failure to `127.0.0.1:19999`
* Docs: `docs/pikaclaw-api.md`; ADR `decisions/0003-pico-protocol-chat-transport.md`

## Status

* [x] Complete

---

# P2-T2 — Implement Gateway Connection Layer

## Objective

Create the smallest client layer required for PikaTalk to communicate with PikaClaw.

## Instructions

Implement connection to the configured local PikaClaw gateway.

The connection layer must support:

* configured endpoint
* normal request errors
* connection unavailable
* reconnect after temporary failure

Keep this layer specific to PikaClaw.

Do not create:

* generic provider abstraction
* generic API plugin architecture
* gateway lifecycle management
* infrastructure monitoring

Do not move chat/history ownership into this layer.

## Deliverable

PikaTalk can establish and report a usable PikaClaw connection.

## Test

1. Connect with gateway running.
2. Attempt connection with gateway stopped/unreachable.
3. Start/recover the gateway externally.
4. Confirm PikaTalk can reconnect.
5. Confirm no local projects/chats/drafts are altered by connectivity failure.

## Completion Evidence

Recorded 2026-08-15.

* Client: `src/pikaclawclient.cpp` Pico Protocol WebSocket over `QTcpSocket` (Qt WebSockets package not installed)
* Endpoint configuration: `~/.config/Radilabs/PikaTalk/pikatalk.conf` keys `picoClaw/endpoint` and `picoClaw/token`; default `ws://127.0.0.1:18790/pico/ws`; empty token reads same-user `~/.picoclaw/.security.yml`
* Error states: unreachable host → `error`; HTTP 401 → `error` with unauthorized; success → `connected`
* Reconnect: auto-reconnect timer; `PicoClawClientTest::reconnectsAfterTemporaryFailure` and `AppControllerTest::gatewayReconnectsWithoutAlteringLocalState`
* Local state: `gatewayFailureDoesNotAlterLocalState` keeps project/chat/messages/draft/model/workspace
* Tests: `pikaclawclient_test` 8/8; `appcontroller_test` 14/14 including Phase 1 regressions

## Status

* [x] Complete

---

# P2-T3 — Implement Model Discovery

## Objective

Populate real model choices from PikaClaw where the gateway exposes model discovery.

## Instructions

Retrieve available models from PikaClaw.

Preserve Phase 1 semantics:

* project default model
* per-chat override
* currently selected model

The gateway-provided list is discovery/validation data.

PikaTalk remains owner of the selected model state.

If a previously stored model is no longer advertised:

* do not silently rewrite local state
* represent the mismatch clearly enough for the user to choose another model

If PikaClaw does not expose model discovery, document that fact and implement only the smallest behavior necessary to satisfy the Phase 2 contract.

Do not contact providers directly.

## Deliverable

Model selection is backed by real PikaClaw model information where supported.

## Test

1. Retrieve model list.
2. Select a project default model.
3. Create/use a chat inheriting that model.
4. Override one chat with another model.
5. Restart PikaTalk.
6. Confirm local selection semantics remain intact.
7. Test a stored model not currently available if practical.

## Completion Evidence

Recorded 2026-08-15.

* Discovery: `loadPicoClawModelNames` reads `model_list[].model_name` from PicoClaw `config.json` (chat HTTP `/v1/models` is 404)
* Identifier format: PicoClaw `model_name` string (e.g. `step-3.7-flash`)
* Unavailable model: `selectedModelUnavailable` is true; stored override is not rewritten (`discoveredModelsDoNotRewriteLocalSelection`)
* Inheritance/override: existing Phase 1 tests still pass; new chat still inherits project default
* UI: model ComboBox populated from `availableModels`; context bar notes missing gateway list entries

## Status

* [x] Complete

---

# P2-T4 — Build Real Chat Request

## Objective

Send a real user message through PikaClaw using the selected chat context.

## Instructions

Replace the Phase 1 local-only send path with a real PikaClaw request.

The request must use:

* selected model
* active workspace where required by PikaClaw
* relevant conversation/session context
* current user message

PikaTalk must continue to persist the user's message locally.

Do not discard or replace existing local conversation history with gateway-owned state.

Do not implement tool UX.

## Deliverable

A user message from the PikaTalk UI reaches PikaClaw and produces a real response path.

## Test

1. Use a real project/chat.
2. Set a known model.
3. Set a known workspace.
4. Send a message.
5. Confirm PikaClaw receives the expected model.
6. Confirm PikaClaw receives the expected workspace/context where applicable.
7. Confirm the user's message persists locally.

## Completion Evidence

Recorded 2026-08-15.

* Request fields: `message.send` with `session_id=pikatalk-chat-<id>` matching `/pico/ws?session_id=`, `payload.content`, `payload.model_name`, `payload.workspace`
* Model: `payload.model_name` is always sent. `/switch model to <name>` is sent only when the selected model is not PicoClaw `agents.defaults.model_name` and has not already been applied on that session; PikaTalk waits for the switch confirmation before the user `message.send` (`sendChatRequestUsesModelWorkspaceAndPersistsUser`, `sendSkipsSwitchWhenSelectedMatchesGatewayDefault`)
* Workspace: included in `payload.workspace` (PicoClaw 0.3.1 does not consume it)
* Local persistence: user row written before the gateway send; draft cleared

## Status

* [x] Complete

---

# P2-T5 — Stream Assistant Responses

## Objective

Display real PikaClaw output while it is being generated.

## Instructions

Implement streaming according to the discovered PikaClaw protocol.

The active assistant response should visibly update while tokens/content arrive.

Persist the final completed assistant response locally after successful completion.

Do not persist every tiny stream fragment as a separate conversation message.

If the request fails mid-stream:

* show a clear error state
* do not destroy existing history
* preserve useful partial response behavior only if the protocol makes that safe and understandable

Do not build Phase 3 tool-call rendering.

## Deliverable

Real streamed assistant responses appear in the active chat and completed output survives restart.

## Test

1. Send a request producing a multi-part/long response.
2. Confirm content appears progressively.
3. Confirm final content persists.
4. Switch chats after completion and return.
5. Restart PikaTalk.
6. Confirm final assistant response is restored.

## Completion Evidence

Recorded 2026-08-15.

* Streaming: `message.update` / non-thought `message.create` update `streamingAssistantText`; thoughts/`tool_calls` ignored
* Persistence: one assistant row, updated in place (`updateMessageContent`); not one row per fragment (`streamsAndPersistsFinalAssistant`)
* Partial/error: gateway `error` sets `requestError`, does not delete history
* Restart: reopen restores the final assistant row

## Status

* [x] Complete

---

# P2-T6 — Implement Stop Generation

## Objective

Allow the user to cancel an active PikaClaw generation.

## Instructions

Expose a Stop action only while generation is active.

Use the cancellation mechanism discovered in P2-T1.

Cancellation must:

* stop further generation
* return the chat UI to an idle usable state
* preserve existing local history
* not corrupt drafts or unrelated chats

Document whether partial assistant output is retained or discarded after cancellation.

Use the simplest behavior consistent with PikaClaw.

## Deliverable

Active generation can be stopped from PikaTalk.

## Test

1. Start a sufficiently long generation.
2. Stop it before completion.
3. Confirm generation ends.
4. Confirm another message can be sent afterward.
5. Confirm unrelated chats are unaffected.
6. Restart and confirm persisted history is coherent.

## Completion Evidence

Recorded 2026-08-15.

* Cancellation: `message.send` content `/stop` (`stopGenerationEndsTurnAndPreservesHistory`)
* Partial response: unpersisted streaming text is discarded; `Task stopped.` lines are not stored as assistant history
* Recovery: another `sendChatMessage` works afterward; prior local messages remain

## Status

* [x] Complete

---

# P2-T7 — Implement Retry / Regenerate

## Objective

Allow retry/regenerate for the supported Phase 2 case without introducing conversation branching.

## Instructions

Implement the smallest retry/regenerate behavior supported by the gateway and existing local model.

This must not create branch/fork UX.

Prefer a simple operation such as retrying the latest failed request or regenerating the latest assistant response from the same prior context.

The exact supported case must be documented.

Do not invent generalized branching semantics.

## Deliverable

One clearly defined retry/regenerate operation works end-to-end.

## Test

1. Exercise the supported retry/regenerate case.
2. Confirm the correct model/workspace/context is reused.
3. Confirm history remains understandable.
4. Confirm no duplicate/corrupt unrelated messages appear.
5. Restart and confirm persisted history remains valid.

## Completion Evidence

Recorded 2026-08-15.

* Semantics: **Regenerate latest assistant** from the same last user message. Deletes that assistant row locally, resends the user content on the same Pico session, persists the new assistant. Also retries a failed send of the last user text without duplicating the user row
* Gateway: same `message.send` path as a normal turn, including `/switch model to` when the session is not already on the selected model
* Persistence: one user row; assistant replaced
* Limitations: PicoClaw session logs may contain extra turns; PikaTalk SQLite is the UI source of truth. No conversation branching

## Status

* [x] Complete

---

# P2-T8 — Handle Gateway Failure and Recovery

## Objective

Make chat failure survivable without turning Phase 2 into gateway management.

## Instructions

Handle at minimum:

* gateway unavailable before request
* connection loss during request
* gateway error response
* reconnect after gateway returns

PikaTalk must preserve:

* projects
* chats
* messages
* drafts
* workspace
* model selection

Represent failure clearly in the UI.

Do not add gateway start/stop/restart controls.

Those belong to Phase 4.

## Deliverable

Temporary gateway failure does not damage local state and normal chat can resume after recovery.

## Test

1. Save a draft.
2. Make gateway unavailable.
3. Attempt a request.
4. Confirm clear error.
5. Confirm draft/history remain intact.
6. Restore gateway externally.
7. Confirm PikaTalk reconnects.
8. Send a successful message.
9. Restart and verify local history.

## Completion Evidence

Recorded 2026-08-15.

* Failure cases: unreachable endpoint before send; 401; `error` frames; connection loss while generating
* UI: `gatewayState` / `gatewayError` / `requestError`
* Reconnect: auto-reconnect timer; send works after recovery (`gatewayErrorPreservesDraftAndHistory`)
* Draft/history: failed send does not consume draft or add a user row; existing messages remain

## Status

* [x] Complete

---

# P2-T9 — Integrate Phase 2 Chat UX

## Objective

Turn the Phase 1 local-only chat UI into a coherent real PikaClaw chat interface.

## Instructions

Remove or retire Phase 1 development-only behavior such as **Local reply**.

The normal UI should now provide:

* real Send
* visible streaming response
* Stop while generating
* supported Retry/Regenerate action
* real model selection based on PikaClaw where available
* clear request/connection error state

Keep:

* project/chat navigation
* local history ownership
* workspace/model context display
* drafts

The gateway status shown here is only the communication state required for normal chat.

Do not implement Phase 4 service lifecycle controls or monitoring.

Avoid general UI redesign.

## Deliverable

A coherent PikaTalk chat workflow backed by PikaClaw.

## Test

Run a normal workflow:

1. Select/create project.
2. Set workspace.
3. select model.
4. Create chat.
5. Send real request.
6. Observe streaming.
7. Stop one request.
8. Retry/regenerate supported case.
9. Switch chats.
10. Restart PikaTalk.
11. Confirm local history/state restores.
12. Temporarily lose gateway and recover.

## Completion Evidence

Recorded 2026-08-15.

* UI: removed **Local reply**; Send calls `sendChatMessage`; Stop while generating; Retry/Regenerate; streaming label; gateway Connected/Connecting/Error/Disconnected; model ComboBox; missing-model warning
* Workflow: covered by controller tests plus live gateway test when `PIKATALK_LIVE_GATEWAY=1`
* Phase 1 retained: projects/chats/drafts/inheritance/delete confirmation tests still pass
* Defects fixed: empty draft bind used NULL (`QString()`) and did not clear SQLite drafts; save now stores `""`

## Status

* [x] Complete

---

# P2-T10 — Regression and Integration Tests

## Objective

Prove Phase 2 did not damage Phase 1 local ownership or isolation.

## Instructions

Add useful automated tests around:

* request construction
* selected model
* workspace context
* stream state
* cancellation
* error/recovery state
* message persistence

Use test doubles/mocks where appropriate.

Do not build an elaborate generic networking test framework.

Manual end-to-end tests must also run against a real local PikaClaw gateway.

## Test

At minimum:

1. Run complete existing test suite.
2. Test two projects with separate chats.
3. Use different model/workspace settings.
4. Send real requests.
5. Confirm history isolation.
6. Test cancellation.
7. Test gateway failure/recovery.
8. Restart PikaTalk.
9. Confirm local state remains correct.

## Completion Evidence

Recorded 2026-08-15.

* Tests added: `pikaclawclient_test` (connect, 401, reconnect, settings, model list, default model, short and long `message.send`); `appcontroller_test` request/stream/stop/retry/failure/draft/live send; `FakePicoServer`
* `ctest --test-dir build --output-on-failure` — 5/5 passed (appstreamtest, applicationpaths_test, database_test, appcontroller_test, pikaclawclient_test)
* Real gateway: `PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewaySendIfEnabled` — PASS (2026-08-15, 4769ms) against PicoClaw 0.3.1 on `127.0.0.1:18790`; assistant content contained `LIVEOK`
* Regressions found/fixed: empty draft bound as SQL NULL; `/switch` interleaved with user send blocked the turn; WebSocket URL `session_id` must match JSON `session_id`; AppController destructor must disconnect gateway signals before member teardown

## Status

* [x] Complete

---

# P2-T11 — Phase 2 Documentation Check

## Objective

Ensure future phases can understand the PikaClaw integration without reverse-engineering it.

## Instructions

Review/update durable documentation.

At minimum document:

* gateway endpoint/configuration
* chat API behavior
* streaming
* cancellation
* model discovery
* workspace/context handling
* retry/regenerate semantics
* failure/reconnect behavior
* what state remains owned by PikaTalk

Do not document Phase 3 or Phase 4 functionality as implemented.

## Deliverable

Documentation matching actual Phase 2 behavior.

## Test

Compare documentation against:

* running implementation
* observed PikaClaw behavior
* current tests

Correct unsupported assumptions.

## Completion Evidence

Recorded 2026-08-15.

* Docs created/updated: `docs/pikaclaw-api.md`, `docs/development.md`, `docs/local-storage.md`, `README.md`
* Assumptions corrected: inbound `payload.model_name` is ignored by 0.3.1; `/switch` is a full agent turn and must not share the serialized slot with the user message; URL `session_id` must match JSON `session_id`; PicoClaw does not consume `payload.workspace`; chat HTTP `/v1/models` is 404
* ADRs referenced: `decisions/0002-local-sqlite-schema.md`, `decisions/0003-pico-protocol-chat-transport.md`

## Status

* [x] Complete

---

# P2-T12 — Phase Handoff Validation

## Objective

Determine whether Phase 2 satisfies its immutable contract.

This task adds no functionality.

## Instructions

Read **Phase 2 — PikaClaw Chat** in `PHASES.md`.

Validate every acceptance criterion individually.

Create:

`docs/handoffs/phase-2.md`

The report must contain:

### Deliverables

Meaningful files/artifacts produced.

### Tests Performed

Actual tests executed, including real gateway tests.

### Results

Pass/fail for all 14 Phase 2 acceptance criteria.

### Known Limitations

Known incomplete or fragile behavior.

### Deferred Work

All valid discoveries outside Phase 2.

### Decisions

References to relevant ADRs.

## Test

Compare implementation against every Phase 2 acceptance criterion.

If any criterion fails:

* Phase 2 remains active.
* Record the failure.
* Create/refine a Phase 2 task required to fix it.
* Do not weaken the contract.

Final result must be:

```text
PHASE 2 HANDOFF: PASS
```

or:

```text
PHASE 2 HANDOFF: FAIL
```

## Completion Evidence

Recorded 2026-08-15.

* Handoff: `docs/handoffs/phase-2.md`
* All 14 Phase 2 acceptance criteria PASS
* Live evidence: `PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewaySendIfEnabled` PASS (4769ms, assistant contained `LIVEOK`)
* `ctest --test-dir build --output-on-failure` — 5/5 passed

```text
PHASE 2 HANDOFF: PASS
```

## Status

* [x] Complete

---

# Deferred Work

Record valid discoveries outside Phase 2 here.

Do not implement them.

* detailed tool-call/result persistence and UI (`payload.kind = "tool_calls"`, thoughts) → Phase 3
* open workspace in terminal/editor/file manager → Phase 3
* PicoClaw 0.3.1 does not apply per-request `payload.workspace`; agent uses `agents.defaults.workspace` → Phase 3/future if a later gateway consumes it
* gateway start/stop/restart → Phase 4
* detailed gateway monitoring → Phase 4
* search / tray / notifications → Phase 5
* branching/forks → Future
* provider-direct integrations → Out of scope unless a future contract says otherwise
* Qt WebSockets module was not used (not installed); custom `QTcpSocket` Pico Protocol client is the Phase 2 transport
* PicoClaw launcher port `18800` is unused in Phase 2

---

# STOP CONDITION

When `P2-T12` passes:

**STOP.**

Do not create the Phase 3 task file.

Do not implement tool UX beyond the minimum required to keep chat functional.

Do not implement workspace launcher actions.

Do not implement gateway start/stop/restart.

Do not modify the Phase 3 contract.

Phase 3 requires explicit authorization after review.
