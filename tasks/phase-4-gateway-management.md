# Phase 4 Tasks — Gateway Management

Phase contract: `PHASES.md` → **Phase 4 — Gateway Management**

This file contains only implementation work for Phase 4.

## Execution Rules

1. Read `PROJECT.md`.
2. Read the Phase 4 contract in `PHASES.md`.
3. Read `docs/handoffs/phase-3.md`.
4. Read `docs/pikaclaw-api.md`.
5. Read relevant ADRs, especially Pico Protocol / gateway decisions.
6. Work on tasks in order.
7. Work on only one task at a time.
8. Do not implement anything listed under Phase 4 exclusions.
9. Do not begin Phase 5 work.
10. Discoveries outside Phase 4 go under **Deferred Work**.
11. Mark a task complete only after its test passes.
12. Record completion evidence under each task.
13. Completing all tasks does **not** automatically complete Phase 4.
14. Perform Phase 4 handoff validation after implementation tasks.
15. After handoff validation, STOP.

## Phase 4 Guardrails

Phase 4 manages exactly one thing:

**the configured local PikaClaw gateway used by PikaTalk.**

It may expose:

* running
* stopped
* unhealthy/error
* reconnecting
* endpoint
* version where available
* start
* stop
* restart

It must not become:

* a generic process manager
* a systemd UI
* remote gateway administration
* fleet management
* an infrastructure monitor
* a metrics dashboard
* an installer/upgrader
* a general PicoClaw configuration editor

Existing PikaTalk chat, persistence, tools, workspace actions, and local ownership must remain unchanged.

---

# P4-T1 — Discover Local Gateway Lifecycle Mechanism

## Objective

Determine the exact supported mechanism for managing the local PikaClaw gateway.

## Instructions

Inspect the locally installed PicoClaw/PikaClaw launcher and existing Phase 2 documentation.

Determine:

* how gateway running/stopped state can be detected
* lifecycle API endpoint(s), if available
* start mechanism
* stop mechanism
* restart mechanism
* authentication required
* response/error formats
* gateway version source
* relationship between launcher port and chat gateway port

Prefer supported PicoClaw lifecycle APIs over shell/process hacks where available.

Do not implement generic process discovery.

Do not investigate remote lifecycle management.

## Deliverable

Update:

`docs/pikaclaw-api.md`

or create a focused gateway lifecycle document if cleaner.

Document only behavior PikaTalk requires.

Create an ADR only if the lifecycle mechanism is a future compatibility boundary.

## Test

1. Confirm current gateway running state.
2. Identify launcher/lifecycle endpoint.
3. Query available status/version information.
4. Start a stopped gateway manually through the discovered mechanism.
5. Stop it.
6. Restart it.
7. Exercise one known failure case.

## Completion Evidence

Record:

* PicoClaw version
* lifecycle endpoint/mechanism
* auth requirements
* start/stop/restart behavior
* version/status fields
* failure behavior

**Evidence (2026-08-15):**

* PicoClaw **0.3.1** (`2cf030d2`); mechanism: `picoclaw-launcher` HTTP `http://127.0.0.1:18800` (not shell/systemd).
* Auth: `POST /api/auth/login` + cookie `picoclaw_launcher_auth`; setup `POST /api/auth/setup` with `password`+`confirm`.
* Exercised with real `config.json` credentials: start → chat `/health` ok; stop → chat down + `gateway_status=stopped`; start again; restart (new pid); unauthorized without cookie; unreachable port failure.
* Version: `GET /api/system/version` and `gateway_version` on running status.
* Documented in `docs/pikaclaw-api.md`; ADR `decisions/0005-picoclaw-launcher-lifecycle.md`.

## Status

* [x] Complete

---

# P4-T2 — Implement Gateway Lifecycle Client

## Objective

Add the smallest client required to control the configured local gateway.

## Instructions

Implement a PicoClaw-specific lifecycle client.

Support:

* status query
* start
* stop
* restart
* version retrieval where available

Keep this separate from the Pico Protocol chat transport where appropriate.

Do not create a generic service-control abstraction.

Do not expose arbitrary executable or service names.

The client targets only the configured local PikaClaw lifecycle endpoint/mechanism.

## Deliverable

A local gateway lifecycle client usable by PikaTalk.

## Test

Using test doubles where appropriate:

1. Parse running state.
2. Parse stopped state.
3. Parse unhealthy/error state.
4. Parse version where available.
5. Execute start request.
6. Execute stop request.
7. Execute restart request.
8. Handle lifecycle command failure clearly.

## Completion Evidence

Record:

* client mechanism
* request/response behavior
* states represented
* tests added

**Evidence (2026-08-15):**

* Client: `PicoClawLifecycleClient` (`QNetworkAccessManager`) against launcher HTTP; cookie `picoclaw_launcher_auth`.
* Tests: `./build/bin/pikaclawlifecycle_test` — **6/6 PASS** (running/stopped status, version, start/stop/restart, unauthorized + precondition_failed).

## Status

* [x] Complete

---

# P4-T3 — Unify Gateway State for Normal UI

## Objective

Expose an understandable local gateway state without creating a monitoring subsystem.

## Instructions

Represent at minimum:

* Running / Connected
* Stopped / Unavailable
* Error / Unhealthy
* Reconnecting where relevant

Use existing Phase 2 chat connection state together with lifecycle status where necessary.

Do not invent dozens of operational states.

Gateway state should answer the user's practical question:

**Can PikaTalk use the local gateway right now, and what is happening?**

Expose:

* configured chat endpoint
* version where available

## Deliverable

Gateway state, endpoint, and version available to the UI/controller.

## Test

1. Gateway running and connected.
2. Gateway stopped.
3. Gateway starting/reconnecting.
4. Gateway unhealthy or lifecycle query failure.
5. Endpoint remains correct.
6. Version appears where supported.

## Completion Evidence

Record:

* state mapping
* endpoint source
* version source
* failure mapping

**Evidence (2026-08-15):** UI maps lifecyclePhase + lifecycleStatus + chat gatewayState (Starting/Stopping/Reconnecting/Stopped/Connected…). Endpoint from `gatewayEndpointDisplay` (configured chat WS). Version from launcher status/`/api/system/version`. Failures → `lifecycleError`. Unit: `gatewayLifecycleControlsPreserveLocalState` PASS.

## Status

* [x] Complete

---

# P4-T4 — Start Local Gateway

## Objective

Allow a stopped configured local gateway to be started from PikaTalk.

## Instructions

Expose a Start action when the lifecycle state indicates the gateway is stopped/unavailable in a way that start is meaningful.

After a successful start:

* wait for the chat gateway to become available
* reconnect the existing Pico Protocol client
* restore normal chat readiness

Do not:

* install PicoClaw
* modify arbitrary PicoClaw configuration
* start unrelated services
* invoke generic systemd controls

## Deliverable

A stopped local gateway can be started from PikaTalk.

## Test

Against the real local gateway:

1. Stop gateway externally.
2. Confirm PikaTalk shows stopped/unavailable.
3. Trigger Start.
4. Confirm lifecycle command succeeds.
5. Confirm chat gateway becomes reachable.
6. Confirm PikaTalk reconnects.
7. Send a real chat message afterward.

## Completion Evidence

Record:

* real start test
* state transition
* reconnect result
* post-start chat result

## Status

* [x] Complete

---

# P4-T5 — Stop Local Gateway

## Objective

Allow a running configured gateway to be stopped from PikaTalk.

## Instructions

Expose a Stop action when the gateway is running.

Stopping must:

* issue the supported local lifecycle command
* transition UI state appropriately
* allow the existing chat connection to close cleanly
* preserve all PikaTalk-owned local state

Do not delete or rewrite:

* projects
* chats
* messages
* drafts
* models
* workspace settings
* tool activity

## Deliverable

A running local gateway can be stopped from PikaTalk.

## Test

Against the real local gateway:

1. Ensure gateway is running.
2. Leave an unfinished draft.
3. Trigger Stop.
4. Confirm gateway stops.
5. Confirm PikaTalk shows stopped/unavailable.
6. Confirm draft remains.
7. Confirm project/chat/history remains.
8. Restart PikaTalk while gateway remains stopped.
9. Confirm local state still restores.

## Completion Evidence

Record:

* real stop test
* state transition
* draft preservation
* history preservation

## Status

* [x] Complete

---

# P4-T6 — Restart Local Gateway

## Objective

Restart the running gateway and recover automatically.

## Instructions

Expose Restart for the configured local gateway.

Restart must:

1. issue the supported restart mechanism
2. tolerate the expected temporary disconnect
3. show restarting/reconnecting state
4. reconnect the Pico Protocol chat transport
5. return to usable chat state

Do not recreate local chat state.

Do not clear drafts.

Do not create new projects/chats or alter model/workspace selections as part of recovery.

## Deliverable

A running local gateway can be restarted from PikaTalk and normal chat resumes.

## Test

Against the real local gateway:

1. Open an existing chat.
2. Leave a draft.
3. Confirm existing messages/tool activity exist.
4. Trigger Restart.
5. Observe temporary disconnect/reconnecting.
6. Confirm gateway returns.
7. Confirm PikaTalk reconnects.
8. Confirm draft remains.
9. Confirm existing history remains.
10. Send a new real message successfully.

## Completion Evidence

Record:

* real restart test
* transition sequence
* reconnect evidence
* local state preservation
* post-restart chat result

## Status

* [x] Complete

---

# P4-T7 — Handle Lifecycle Command Failures

## Objective

Make failed start/stop/restart operations understandable and recoverable.

## Instructions

Handle practical lifecycle failures such as:

* launcher unavailable
* authentication rejected
* start failure
* stop failure
* restart failure
* lifecycle request timeout
* gateway fails to become healthy after command

Present a concise user-visible error.

The error must not destroy local state or strand the UI in a false state.

Allow a later retry.

Do not add logs dashboard or infrastructure troubleshooting UI.

## Deliverable

Lifecycle command failures produce clear errors and leave PikaTalk usable.

## Test

At minimum:

1. Target an unavailable lifecycle endpoint.
2. Exercise a rejected/failed command with a test double if needed.
3. Confirm understandable error.
4. Confirm projects/chats/messages/drafts remain intact.
5. Restore correct endpoint/mechanism.
6. Retry successfully.

## Completion Evidence

Record:

* failure cases tested
* UI/error behavior
* recovery behavior

## Status

* [x] Complete

---

# P4-T8 — Integrate Gateway Controls into UI

## Objective

Expose useful gateway management without turning the main UI into an operations console.

## Instructions

The normal UI should show:

* gateway state
* endpoint
* version where available

Provide appropriate controls:

* Start
* Stop
* Restart

Controls should enable/disable according to current state.

Keep the UI compact.

Do not add:

* CPU/GPU metrics
* request graphs
* logs viewer
* process lists
* systemd unit controls
* PicoClaw configuration editor
* generic endpoint fleet UI

## Deliverable

Compact gateway state and lifecycle controls available from PikaTalk.

## Test

1. Running state shows appropriate controls.
2. Stopped state shows appropriate controls.
3. Restarting/reconnecting state is understandable.
4. Endpoint is visible.
5. Version is visible where supported.
6. Error state is understandable.
7. UI remains usable at normal window sizes.

## Completion Evidence

Record:

* UI placement
* controls/state behavior
* layout tests

## Status

* [x] Complete

---

# P4-T9 — State Preservation and Regression Tests

## Objective

Prove gateway management cannot damage the application state built in Phases 1–3.

## Instructions

Add focused automated coverage around lifecycle transitions and recovery.

Use lifecycle test doubles where appropriate.

Test preservation of:

* projects
* chats
* messages
* drafts
* workspace/model state
* tool activity

Existing chat streaming, stop, retry, tool activity, copy, and workspace actions must continue working.

## Test

At minimum:

1. Run full existing `ctest`.
2. Create/use multiple projects/chats.
3. Leave drafts.
4. Preserve tool activity.
5. Stop gateway.
6. Restart application.
7. Start gateway.
8. Confirm reconnect.
9. Restart gateway.
10. Confirm all local state unchanged.
11. Send a normal chat afterward.
12. Exercise Phase 3 workspace actions.

## Completion Evidence

Record:

* tests added
* full `ctest` result
* local state comparison
* regressions found/fixed

## Status

* [x] Complete

---

# P4-T10 — Real Gateway Lifecycle Integration Test

## Objective

Demonstrate the full Phase 4 lifecycle against the actual local PicoClaw installation.

## Instructions

Run one deliberate real lifecycle sequence.

Use the same configured local gateway PikaTalk normally uses.

Sequence:

1. confirm running
2. stop from PikaTalk
3. verify stopped
4. start from PikaTalk
5. verify connected
6. send a real message
7. restart from PikaTalk
8. verify reconnect
9. send another real message
10. verify local state remained intact

Gate destructive real tests behind an explicit test environment variable if needed so normal `ctest` does not unexpectedly stop the user's gateway.

## Deliverable

Recorded real start/stop/restart/recovery evidence.

## Test

Perform the complete lifecycle sequence above.

## Completion Evidence

Record:

* command/test used
* PicoClaw version
* lifecycle states observed
* chat success after start
* chat success after restart
* state preservation result

## Status

* [x] Complete

---

# P4-T11 — Phase 4 Documentation Check

## Objective

Document only the lifecycle behavior future work needs to understand.

## Instructions

Update documentation with:

* local lifecycle endpoint/mechanism
* authentication/config required
* state mapping
* version lookup
* start behavior
* stop behavior
* restart behavior
* reconnect behavior
* known failure cases
* explicit boundary that this controls only the configured local PikaClaw gateway

Do not document generic service management.

Do not describe Phase 5 polish as implemented.

## Deliverable

Documentation matching actual Phase 4 behavior.

## Test

Compare documentation against:

* real local lifecycle tests
* current code
* current configuration

Correct unsupported assumptions.

## Completion Evidence

Record:

* docs updated
* assumptions corrected
* ADRs referenced

## Status

* [x] Complete

---

# P4-T12 — Phase Handoff Validation

## Objective

Determine whether Phase 4 satisfies its immutable contract.

This task adds no functionality.

## Instructions

Read **Phase 4 — Gateway Management** in `PHASES.md`.

Validate every acceptance criterion individually.

Create:

`docs/handoffs/phase-4.md`

The report must contain:

### Deliverables

Meaningful files/artifacts produced.

### Tests Performed

Actual automated and real local gateway lifecycle tests.

### Results

Pass/fail for all 11 Phase 4 acceptance criteria.

### Known Limitations

Known incomplete or fragile behavior.

### Deferred Work

All valid discoveries outside Phase 4.

### Decisions

References to relevant ADRs.

## Test

Compare implementation against every Phase 4 acceptance criterion.

If any criterion fails:

* Phase 4 remains active.
* Record the failure.
* Create/refine a Phase 4 task needed to fix it.
* Do not weaken the contract.

Final result must be:

```text
PHASE 4 HANDOFF: PASS
```

or:

```text
PHASE 4 HANDOFF: FAIL
```

## Completion Evidence

Record final result and handoff document path.

**Evidence (2026-08-15):**

* Handoff: `docs/handoffs/phase-4.md`
* All 11 Phase 4 acceptance criteria PASS
* Real start/stop/restart/reconnect + `LIFEOK` chat + local-state preservation recorded
* Final result:

```text
PHASE 4 HANDOFF: PASS
```

## Status

* [x] Complete

---

# Deferred Work

Record valid discoveries outside Phase 4 here.

Do not implement them.

Examples:

* remote gateway administration → Out of scope
* managing multiple gateways → Future only if explicitly contracted
* generic service/systemd controls → Out of scope
* gateway install/update → Out of scope
* configuration editor beyond existing connection information → Out of scope
* infrastructure monitoring / metrics → Out of scope
* search / shortcuts / tray / notifications / packaging polish → Phase 5
* conversation branching → Future

---

# STOP CONDITION

When `P4-T12` passes:

**STOP.**

Do not create the Phase 5 task file.

Do not start search, shortcuts, notifications, tray, packaging, or visual-polish work.

Do not turn lifecycle control into generic service management.

Do not modify the Phase 5 contract.

Phase 5 requires explicit authorization after review.
