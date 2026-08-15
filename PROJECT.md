# PikaTalk

## General Information

**PikaTalk** is a small native Linux desktop client for **PikaClaw**, aimed primarily at **openSUSE + KDE Plasma**.

The goal is not to build another general-purpose AI frontend.

The goal is to build a fast, lightweight working interface for local agent use where conversations are tied to actual projects and working directories.

PikaTalk owns its local UI state. PikaClaw remains the execution/gateway layer.

## Core Product Model

PikaTalk manages:

* projects / folders
* chats and chat history
* per-project defaults
* per-chat workspace
* per-chat model selection
* local drafts
* gateway connection configuration
* gateway status and lifecycle controls
* persisted message and tool-call history

PikaClaw receives the context required to execute a request, including relevant chat history, selected model, workspace, and session information.

PikaTalk must not depend on the gateway owning conversation history or UI state.

## Core v1 Capabilities

### Projects and chats

* Hermes-style project/folder organization
* create new chats
* switch between chats
* view previous chats
* rename chats
* archive or delete chats
* persist all local state across restarts

### Workspace

Each project may define a default working directory.

Each chat may override it.

The active workspace must always be clearly visible.

Where practical, PikaTalk should provide quick actions to open the workspace in:

* terminal
* file manager
* editor

### Model selection

Projects may define a default model.

Chats may override it.

The active model must always be clearly visible.

### Chat interaction

Minimum chat UX:

* send messages
* streamed responses
* stop generation
* retry/regenerate
* copy message text
* copy code
* preserve unfinished drafts locally

### Gateway

PikaTalk must clearly expose local PikaClaw gateway state.

Minimum controls:

* running / stopped / error state
* configured endpoint
* version where available
* start
* stop
* restart
* reconnect after temporary failure

Gateway failure must not destroy local chat state or drafts.

### Tool activity

PikaTalk must persist the raw information needed to represent agent/tool activity.

The UI may simplify how this information is displayed, but the stored conversation must not discard tool calls or tool results merely because the first UI does not expose every detail.

## Always-Visible Context

During a chat, the user should be able to determine without opening settings:

* current project
* current workspace
* current model
* gateway state

Wrong-context agent execution is considered a serious UX failure.

## Technical Direction

Initial target platform:

* Linux
* openSUSE
* KDE Plasma 6

Initial application stack:

* Qt 6
* Kirigami for native application structure
* Qt Quick / QML where appropriate
* SQLite for local persistent state

Use native Plasma/Linux integration where it materially improves normal use.

Do not introduce cross-platform abstractions unless an actual requirement exists.

## Product Principles

### Native and small

PikaTalk should feel like a normal Plasma application.

Avoid Electron.

Avoid introducing a browser application architecture unless there is a demonstrated reason for it.

### Local state belongs to PikaTalk

The gateway should be replaceable or restartable without losing the user's projects and conversations.

### Workspace is first-class

This is an agent client, not merely a chat window.

The working directory is part of the operational context of the conversation.

### Prefer visible context over hidden magic

The application should make important execution context obvious instead of silently inferring it.

### KISS

Do not build abstractions for hypothetical future features.

Do not introduce plugin systems, generic provider architectures, RAG systems, prompt libraries, cloud synchronization, telemetry, or account-management machinery without an explicit future phase requiring them.

## Initial Non-Goals

The initial product does not include:

* cloud chat synchronization
* multi-user support
* remote gateway administration
* generic AI provider management
* RAG / knowledge-base management
* prompt marketplace or prompt library
* agent marketplace
* workflow builder
* arbitrary plugin framework
* branching/forking conversations unless explicitly introduced later
* full-text message search unless explicitly introduced later
* support for non-Linux platforms

These may be revisited by future phase contracts.

---

# Development Factory Rules

## Phase Boundaries

A phase is an immutable execution boundary.

Tasks may be refined inside the active phase, but its:

* goal
* scope
* exclusions
* acceptance criteria
* handoff contract

must not be expanded during implementation.

Work discovered outside the active phase is recorded as deferred work.

It is not implemented.

A completed task does not mean a completed phase.

The implementation agent must stop when the active phase handoff contract has been satisfied.

It must never begin the next phase automatically.

## Task Availability

`PHASES.md` contains the product roadmap.

Detailed task files are created only for the currently authorized phase.

Future phase task files should not be created in advance.

This is intentional.

The roadmap tells agents where the project is going without giving the implementation agent a pile of future work to accidentally execute.

## Roles

### Implementation

Cursor is expected to perform the primary implementation work.

Cursor must operate only inside the currently active phase contract.

### Project steering

The project owner and ChatGPT oversee:

* scope
* architecture
* phase acceptance
* decisions
* phase transitions

### External review

Grok may inspect the implementation as an external reviewer.

External review may:

* identify bugs
* question decisions
* identify risks
* suggest future work

External review does not automatically change implementation scope.

Any review finding outside the current phase must be recorded as deferred work unless explicitly promoted into the active phase.

---

# Information Management

Use four categories.

## Decision Records

Create a decision record only when future work must respect a:

* choice
* constraint
* rejection
* tradeoff
* compatibility commitment

Decision records live under `decisions/`.

Use the format:

* Status
* Context
* Decision
* Consequences

Do not create an ADR merely because implementation work happened.

Do not rewrite accepted history.

If a decision changes later, supersede it with a new decision record.

## Documentation

Use `docs/` for information such as:

* discovered PikaClaw API behavior
* schemas
* protocols
* setup procedures
* build/install instructions
* Plasma integration behavior
* operational procedures
* external technical facts future work needs

Documentation records knowledge.

It does not record every development event.

## Task Notes

Use the current phase task file for:

* implementation progress
* tests performed
* test results
* files changed
* temporary findings
* current known limitations

Task notes are execution evidence, not permanent architecture records.

## Deferred Work

Valid work discovered outside the current phase must be recorded as deferred.

Do not implement it opportunistically.

A deferred item becomes implementation work only when it is deliberately added to a future or current phase.

## The Rule

If forgetting information could cause a future agent to make a wrong implementation choice, record it.

Otherwise, don't.
