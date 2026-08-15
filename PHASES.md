# PikaTalk — Phase Contracts

These phases define immutable execution boundaries.

Tasks inside a phase may be refined as implementation progresses, but the phase goal, scope, exclusions, acceptance criteria, and handoff contract must not be changed during implementation.

Work discovered outside the active phase is recorded as deferred work and is not implemented.

A completed task does not mean a completed phase.

The agent must stop after satisfying the active phase handoff contract. It must never begin the next phase automatically.

Detailed task files are created only for the currently authorized phase.

---

# Phase Map

| Phase | Name                   | Outcome                                                                      |
| ----- | ---------------------- | ---------------------------------------------------------------------------- |
| 0     | Native Foundation      | Working Qt 6 / Kirigami Plasma application with SQLite and basic UI shell    |
| 1     | Local Chat State       | Projects, chats, history, workspace and model state work locally             |
| 2     | PikaClaw Chat          | Real conversations work through the PikaClaw gateway                         |
| 3     | Agent and Workspace UX | Tool activity and workspace-oriented actions work cleanly                    |
| 4     | Gateway Management     | Local gateway can be monitored and controlled from PikaTalk                  |
| 5     | Daily-Use Polish       | Search, shortcuts, packaging and reliability make PikaTalk usable day to day |

Future capabilities require additional phase contracts.

---

# Phase 0 — Native Foundation

## Goal

Create the smallest working native PikaTalk application and establish the local development, build, run, install, and debugging workflow on openSUSE + KDE Plasma 6.

## Entry Conditions

* Repository structure exists.
* `PROJECT.md` exists.
* Qt 6 / Plasma development environment is available.
* No implementation from later phases is required.

## Scope

* Create a Qt 6 application.
* Use Kirigami for the application structure.
* Use Qt Quick / QML where appropriate.
* Create the basic application window.
* Create the basic intended layout:

  * project/chat sidebar
  * conversation area
  * message input area
  * always-visible context area
* The context area must have placeholders for:

  * project
  * workspace
  * model
  * gateway state
* Initialize local SQLite storage.
* Establish the application's local data/config paths using normal Linux/XDG conventions.
* Add enough fake/static data to demonstrate the intended UI structure.
* Document local build, run, install, and debugging workflow.

## Explicit Exclusions

* Real PikaClaw gateway communication.
* Real chats.
* Chat persistence beyond proving SQLite works.
* Projects/folder management.
* Workspace management.
* Model discovery or model selection.
* Streaming.
* Tool-call handling.
* Gateway lifecycle management.
* Search.
* Notifications.
* Tray integration.
* Conversation branching.
* Plugin systems.
* RAG.
* Cloud synchronization.

## Acceptance Criteria

1. PikaTalk builds successfully on the target openSUSE development machine.
2. PikaTalk launches as a normal Plasma desktop application.
3. The main window renders correctly.
4. The sidebar, conversation area, input area, and context area are visible.
5. Fake/static content demonstrates the intended layout.
6. SQLite can be initialized and accessed successfully.
7. Application data uses appropriate local/XDG paths.
8. Local build/run/debug instructions are documented.
9. No PikaClaw network communication exists.
10. No later-phase functionality has been implemented.

## Handoff Contract

Before Phase 0 can be declared complete:

* All acceptance criteria must be demonstrated.
* Build/run/debug instructions must be documented.
* Files changed must be recorded.
* Tests performed and their results must be recorded.
* Known limitations must be recorded.
* Deferred discoveries must be recorded.
* Any architectural decision that later work must respect must be recorded under `decisions/`.

Then STOP.

Do not begin Phase 1.

---

# Phase 1 — Local Chat State

## Goal

Make PikaTalk a complete local conversation organizer before connecting it to PikaClaw.

Projects, chats, messages, workspace selection, and model selection must persist locally and survive application restart.

## Entry Conditions

* Phase 0 handoff contract is satisfied.
* Native application shell works.
* SQLite initialization works.

## Scope

### Projects / folders

Implement Hermes-style project/folder organization.

Support:

* create project
* rename project
* delete project where safe
* select project

Each project may define:

* default workspace
* default model

### Chats

Support:

* create chat
* switch chats
* view previous chats
* rename chat
* archive or delete chat
* persist chat ordering/history

### Messages

Persist local conversation messages.

Messages must support enough structure for later phases to distinguish at minimum:

* user messages
* assistant messages

The schema must be extensible enough to later persist tool activity without replacing the existing message system.

Do not build tool UX yet.

### Workspace

Each project may define a default workspace directory.

Each chat may override that workspace.

New chats inherit the project default at creation time.

The active workspace must be visible in the context area.

### Model

Each project may define a default model.

Each chat may override that model.

New chats inherit the project default at creation time.

At this phase the model may be a locally stored identifier/string because real model discovery belongs to Phase 2.

The active model must be visible in the context area.

### Drafts

Persist unfinished message drafts locally so switching chats or restarting PikaTalk does not unnecessarily lose typed text.

## Explicit Exclusions

* PikaClaw communication.
* Sending messages to a model.
* Model discovery from the gateway.
* Streaming.
* Stop generation.
* Retry/regenerate.
* Tool calls.
* Tool results.
* Gateway status discovery.
* Gateway start/stop/restart.
* Open-workspace external actions.
* Full-text message search.
* Conversation branching/forking.
* Notifications.
* Tray integration.

## Acceptance Criteria

1. Projects can be created, selected, renamed, and persisted.
2. Chats can be created, selected, renamed, archived/deleted, and persisted.
3. Existing chats are available after restarting PikaTalk.
4. Local messages are persisted correctly.
5. Project default workspace is inherited by new chats.
6. A chat can override its workspace.
7. Project default model is inherited by new chats.
8. A chat can override its model.
9. Active project, workspace, and model are clearly visible.
10. Draft text survives chat switching and application restart.
11. Removing or changing one chat does not corrupt unrelated chats.
12. Phase 0 functionality does not regress.

## Handoff Contract

Before Phase 1 can be declared complete:

* Local project/chat state must work reliably across restart.
* Workspace inheritance and override behavior must be tested.
* Model inheritance and override behavior must be tested.
* SQLite schema and migration approach must be documented if future implementation must respect them.
* Tests and results must be recorded.
* Known limitations must be recorded.
* Deferred work must be recorded.
* Architectural decisions affecting later phases must be recorded.

Then STOP.

Do not begin Phase 2.

---

# Phase 2 — PikaClaw Chat

## Goal

Connect PikaTalk to the local PikaClaw gateway and make normal agent conversations work end-to-end.

## Entry Conditions

* Phase 1 handoff contract is satisfied.
* Local chats and messages persist correctly.
* Workspace and model context exist per chat.
* A usable PikaClaw gateway is available for development/testing.

## Scope

* Investigate and document the PikaClaw gateway API required for chat operation.
* Connect PikaTalk to the configured gateway endpoint.
* Discover available models where supported by PikaClaw.
* Populate model selection from the gateway.
* Preserve the existing per-project/per-chat model behavior.
* Send user messages to PikaClaw.
* Send the active workspace context required by PikaClaw.
* Send relevant conversation/session context required for execution.
* Stream assistant responses into the active chat.
* Persist final assistant responses locally.
* Support stopping an active generation.
* Support retry/regenerate of the most recent failed or selected assistant response where the gateway supports the required behavior.
* Represent connection/request errors clearly.
* Preserve drafts and local chat history during gateway failures.
* Reconnect after temporary communication failures without destroying local state.

PikaTalk remains the owner of conversation history and UI state.

## Explicit Exclusions

* Gateway process start/stop/restart.
* Detailed gateway service monitoring.
* Tool-call UI beyond what is strictly required to keep chat operation functional.
* Workspace open-in-terminal/editor/file-manager actions.
* Search.
* Conversation branching.
* Notifications.
* Tray integration.
* Remote gateway administration.
* Generic provider abstraction.
* Direct communication with arbitrary model providers.

## Acceptance Criteria

1. PikaTalk can connect to the configured PikaClaw gateway.
2. Available models can be retrieved where the gateway exposes them.
3. The selected model is used for a conversation request.
4. The active workspace is supplied correctly to PikaClaw where required.
5. A user message can produce a real assistant response.
6. Responses stream visibly into the chat.
7. Completed responses are persisted locally.
8. Generation can be stopped.
9. Retry/regenerate behaves correctly for the supported case.
10. Gateway/network failure produces a clear error without losing local history.
11. Draft text is not lost because the gateway fails.
12. Reconnection after a temporary gateway failure works.
13. Restarting PikaTalk still restores locally owned conversation history.
14. Phase 1 functionality does not regress.

## Handoff Contract

Before Phase 2 can be declared complete:

* A real PikaClaw conversation must work end-to-end.
* Streaming and cancellation must be tested.
* Gateway unavailable/recovery behavior must be tested.
* The discovered PikaClaw API behavior required by PikaTalk must be documented under `docs/`.
* Any protocol assumptions future work must respect must be recorded.
* Tests and results must be recorded.
* Known limitations must be recorded.
* Deferred work must be recorded.
* Architectural decisions must be recorded where necessary.

Then STOP.

Do not begin Phase 3.

---

# Phase 3 — Agent and Workspace UX

## Goal

Make PikaTalk useful for real workspace-based agent work by properly preserving and presenting tool activity and by connecting the active workspace to normal desktop actions.

## Entry Conditions

* Phase 2 handoff contract is satisfied.
* Real PikaClaw chat works end-to-end.
* Workspace context is sent correctly.
* Conversations persist locally.

## Scope

### Tool activity

Persist the raw information required to represent PikaClaw agent/tool activity.

Where PikaClaw exposes them, preserve:

* tool calls
* tool names
* tool inputs
* tool results
* relevant execution status/error information

Display tool activity in a compact and understandable way.

Tool details may be expandable/collapsible.

Do not discard useful raw tool information merely because the current UI chooses not to show every field.

### Message usability

Support:

* copy message text
* copy code blocks

### Workspace actions

Provide quick actions for the active workspace to open it using appropriate local desktop tools:

* terminal
* file manager
* editor

Implementation should respect the configured workspace rather than introducing a second workspace concept.

## Explicit Exclusions

* Gateway process lifecycle management.
* Generic tool/plugin framework.
* Tool configuration marketplace.
* Custom workflow builder.
* RAG.
* Knowledge-base management.
* Search.
* Conversation branching.
* Notifications.
* Tray integration.
* Arbitrary remote filesystem support.

## Acceptance Criteria

1. Tool calls returned by PikaClaw are persisted locally.
2. Tool results returned by PikaClaw are persisted locally.
3. Tool activity can be understood from the UI without exposing raw protocol noise by default.
4. Stored tool information remains available after restarting PikaTalk.
5. Copying normal message text works.
6. Copying code blocks works.
7. Active workspace can be opened in the file manager.
8. Active workspace can be opened in a terminal.
9. Active workspace can be opened in the configured/default editor.
10. Workspace actions operate on the workspace shown in the active context.
11. Existing Phase 2 chat behavior does not regress.

## Handoff Contract

Before Phase 3 can be declared complete:

* Tool activity must survive persistence and reload.
* Workspace actions must be tested against a real project directory.
* Tool success and tool failure representation must be tested where available.
* Any PikaClaw tool-event/schema behavior future work must understand must be documented.
* Tests and results must be recorded.
* Known limitations must be recorded.
* Deferred work must be recorded.
* Architectural decisions must be recorded where necessary.

Then STOP.

Do not begin Phase 4.

---

# Phase 4 — Gateway Management

## Goal

Make the local PikaClaw gateway operationally manageable from PikaTalk without turning PikaTalk into a general service manager.

## Entry Conditions

* Phase 3 handoff contract is satisfied.
* PikaTalk communicates reliably with PikaClaw.
* Local gateway execution method is known.

## Scope

Expose local gateway state.

At minimum represent:

* running
* stopped
* error/unhealthy
* reconnecting where relevant

Expose available gateway information where supported:

* endpoint
* version

Provide local gateway controls:

* start
* stop
* restart

After gateway restart:

* PikaTalk must reconnect cleanly.
* existing chats must remain intact.
* current local state must remain intact.
* drafts must remain intact.

Gateway management must target the configured local PikaClaw gateway and must not become a generic process/service control system.

## Explicit Exclusions

* Remote gateway administration.
* Fleet management.
* Generic systemd management UI.
* Gateway installation/upgrades.
* Gateway configuration editor beyond connection information already required by PikaTalk.
* Infrastructure monitoring.
* Metrics dashboards.
* Search.
* Conversation branching.
* Notifications except where later introduced explicitly.
* Plugin systems.

## Acceptance Criteria

1. PikaTalk detects whether the configured local gateway is available.
2. Gateway state is clearly visible in the normal chat UI.
3. Gateway endpoint can be seen.
4. Gateway version is visible where the gateway exposes it.
5. A stopped gateway can be started from PikaTalk.
6. A running gateway can be stopped from PikaTalk.
7. A running gateway can be restarted from PikaTalk.
8. PikaTalk reconnects after a successful gateway restart.
9. Gateway restart does not lose projects, chats, messages, or drafts.
10. Failed lifecycle commands produce understandable errors.
11. Existing Phase 3 behavior does not regress.

## Handoff Contract

Before Phase 4 can be declared complete:

* Start, stop, and restart must be tested against the real local gateway.
* Recovery after restart must be demonstrated.
* Failure behavior must be tested.
* The local gateway lifecycle mechanism must be documented.
* Tests and results must be recorded.
* Known limitations must be recorded.
* Deferred work must be recorded.
* Decisions future work must respect must be recorded.

Then STOP.

Do not begin Phase 5.

---

# Phase 5 — Daily-Use Polish

## Goal

Make PikaTalk comfortable and reliable enough for normal daily use without expanding the core product into a general AI platform.

## Entry Conditions

* Phase 4 handoff contract is satisfied.
* Projects, chats, workspaces, models, agent activity, and gateway management work end-to-end.

## Scope

May include the following agreed v1 polish items:

### Search

* Fast title/filter search for chats and projects.
* Full-text message search may be implemented only if it remains small and justified by actual use during this phase.

### Keyboard UX

Add useful shortcuts for common operations such as:

* new chat
* switch/focus chat search
* focus message input
* stop generation

Exact shortcuts should follow normal Plasma conventions where practical.

### Plasma integration

May include, where useful:

* native notifications
* tray integration

These should only be added if they improve normal daily operation.

### Reliability and UX cleanup

* Improve empty states.
* Improve loading states.
* Improve gateway/error states.
* Ensure long chats remain reasonably usable.
* Ensure UI state behaves predictably across restart.
* Remove obvious rough edges discovered through normal use.

### Packaging

* Provide a clean local installation/package path appropriate for openSUSE/Plasma.
* Bring user-facing README/install documentation to first-release quality.
* Establish simple application versioning.

Each substantial polish item must still be introduced as an explicit task before implementation.

## Explicit Exclusions

* Conversation branching/forking.
* Cloud synchronization.
* Multi-user support.
* Generic provider management.
* Direct provider integrations bypassing PikaClaw.
* RAG.
* Knowledge-base management.
* Prompt marketplace/library.
* Agent marketplace.
* Workflow builder.
* Generic plugin framework.
* Telemetry/analytics.
* Support for arbitrary operating systems.
* Large redesigns of working core architecture merely for cleanliness.

## Acceptance Criteria

1. Normal project/chat navigation is comfortable for daily use.
2. Chat/project title search works.
3. Important keyboard operations work reliably.
4. Long conversations remain reasonably usable.
5. Gateway and request errors remain understandable.
6. Application restart restores expected local state.
7. Any notifications/tray features introduced are useful and non-annoying.
8. Installation documentation works from a clean environment.
9. Versioning is defined.
10. Core functionality from Phases 0–4 remains intact.
11. No excluded platform features have slipped into scope.

## Handoff Contract

Phase 5 is complete when:

* Implemented polish items have explicit tasks and acceptance tests.
* All selected Phase 5 tasks pass.
* Installation and operating documentation is current.
* Known limitations are documented.
* Remaining ideas are clearly recorded as future/deferred work rather than silently included in the product.
* Repository state represents a usable first PikaTalk release.

Then STOP.

Any significant new capability requires a new phase contract.

---

# Future Phase Candidates

These are not active phases and are not implementation authorization.

Possible future work includes:

* conversation fork/branch support
* richer per-project instructions/context
* deeper full-text search
* additional Plasma integration

They must not be implemented until deliberately converted into phase contracts.
