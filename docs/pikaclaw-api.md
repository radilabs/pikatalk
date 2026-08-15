# PikaClaw chat API (Phase 2–3)

This document records the **observed** local chat API PikaTalk uses. It is not a general PicoClaw manual.

On this development machine the locally available PikaClaw product is **PicoClaw 0.3.1** (`git: 2cf030d2`, `/usr/bin/picoclaw`). PikaTalk treats that process as the PikaClaw execution/gateway layer.

PikaTalk owns projects, chats, messages, drafts, workspace state, and selected model state. PicoClaw executes requests. PicoClaw session files are not the source of truth for PikaTalk history.

Do not put tokens, API keys, or `.security.yml` contents in this repository.

## Gateway tested

| Item | Observed value |
| --- | --- |
| Product | PicoClaw 0.3.1 (`2cf030d2`) |
| Process | `picoclaw gateway -E` |
| Chat bind | `127.0.0.1:18790` (also `[::1]:18790`) |
| Health | `GET http://127.0.0.1:18790/health` → `{"status":"ok",...}` **without auth** |
| Chat transport | WebSocket `ws://127.0.0.1:18790/pico/ws` |
| Config | `~/.picoclaw/config.json` |
| Default model | `step-3.7-flash` (`picoclaw model`) |
| Default workspace | `~/.picoclaw/workspace` |

A separate launcher listens on `127.0.0.1:18800`. That process is **not** the chat gateway. Phase 4 uses it for gateway start/stop/restart (see **Gateway lifecycle** below).

## Reachability

* Gateway up: `GET /health` returns HTTP 200 JSON with `"status":"ok"`.
* Gateway down / wrong port: TCP connection fails (tested `127.0.0.1:19999`).
* Chat path without auth: `GET /pico/ws` returns HTTP **401** `unauthorized`.
* Wrong chat path: `GET /pico` returns HTTP **404**.
* OpenAI-compat HTTP is **not** present: `GET /v1/models` and `GET /v1/chat/completions` return **404**.

## Authentication

Pico channel token lives under PicoClaw secrets (`~/.picoclaw/.security.yml` → `channel_list.pico.settings.token`). It is not in git.

WebSocket auth (from PicoClaw 0.3.1 source, confirmed by live 101 upgrade):

1. `Authorization: Bearer <token>` (used by the live probe)
2. WebSocket subprotocol `token.<token>`
3. Query `?token=` only if PicoClaw `allow_token_query` is enabled (do not use this)

PikaTalk stores the endpoint and token in local config (`QSettings` under the XDG config directory). If the token setting is empty, it may read the same-user PicoClaw secrets file. Never log the token.

## Chat endpoint and request format

Connect:

```text
ws://127.0.0.1:18790/pico/ws?session_id=<pikatalk-session>
```

`session_id` is PikaTalk's execution correlation id for this local chat (stable per PikaTalk chat id). **Observed PicoClaw 0.3.1 behavior:** the query parameter and the JSON `session_id` on `message.send` must be the same value. If the query is omitted, or if it differs from the JSON field, the gateway may accept the WebSocket (including protocol pings) but will not produce a chat reply. If the query is omitted, PicoClaw generates a UUID for its own session; that id is not the JSON `session_id`. PicoClaw maps the session to internal chat id `pico:<session_id>` and may keep its own session log. **PikaTalk still persists the conversation locally and does not adopt that log as history.**

Wire format (JSON text frames):

```json
{
  "type": "message.send",
  "id": "client-request-id",
  "session_id": "pikatalk-chat-1",
  "payload": {
    "content": "user text"
  }
}
```

Client → server types used in Phase 2:

| type | purpose |
| --- | --- |
| `ping` | keepalive; server replies `pong` with the same `id` |
| `message.send` | user text, `/stop`, or `/switch model to <name>` |

Sanitized successful ping:

```json
{"type": "ping", "id": "p1"}
```

```json
{"type": "pong", "id": "p1"}
```

Sanitized successful chat send (payload fields `model_name` and `workspace` are included by PikaTalk; see limitations):

```json
{
  "type": "message.send",
  "id": "m1",
  "session_id": "pikatalk-chat-1",
  "payload": {
    "content": "Reply with exactly the word STREAMOK and nothing else.",
    "model_name": "step-3.7-flash",
    "workspace": "/home/naorw/.picoclaw/workspace"
  }
}
```

Empty `content` is rejected with `error` / `empty_content`.

## Streaming

Observed live sequence for a short reply:

1. `typing.start`
2. `typing.stop`
3. `message.create` with `payload.kind = "thought"` (reasoning; **not** the user-visible reply)
4. `message.create` without `kind` (final assistant text)
5. optional WebSocket protocol `ping` frames

Example final create (sanitized):

```json
{
  "type": "message.create",
  "payload": {
    "content": "STREAMOK",
    "message_id": "<uuid>"
  }
}
```

Thought create (sanitized):

```json
{
  "type": "message.create",
  "payload": {
    "kind": "thought",
    "model_name": "step-3.7-flash",
    "content": "<reasoning text>",
    "message_id": "<uuid>"
  }
}
```

PicoClaw source also defines `message.update` for in-place content growth (`payload.message_id` + `payload.content`). A 341-character reply on this 0.3.1 gateway arrived as one complete `message.create`, not a series of `message.update` events. PikaTalk must:

* show typing while a turn is active
* apply `message.update` when it arrives
* treat a non-thought `message.create` as the visible assistant message
* persist only the completed user-visible assistant text, not each fragment and not thought/tool payloads

There is no `[DONE]` sentinel and no `turn.done` event on 0.3.1. Completion is inferred from `typing.stop` plus a non-thought `message.create` (and no further updates).

## Cancellation

There is no dedicated WebSocket cancel type on 0.3.1.

Send another `message.send` on the **same session** with content `/stop`.

Live result after starting a long count and sending `/stop`:

```text
Task stopped. "<original prompt>" was canceled.
```

PikaTalk should not persist that status line as a normal assistant chat message. Existing local history stays. Partial assistant text that already arrived may be kept in the UI until the turn ends; Phase 2 discards unpersisted partial text on stop unless a non-thought `message.create` already completed.

`/help` also lists `/stop - Stop the current task`.

## Model discovery

The chat gateway does **not** expose HTTP model listing:

* `GET http://127.0.0.1:18790/v1/models` → 404
* `GET http://127.0.0.1:18790/api/models` → 404
* `GET http://127.0.0.1:18800/api/models` → 401 (launcher dashboard auth; unused in Phase 2)

Structured discovery is PicoClaw's local `model_list` in `~/.picoclaw/config.json`. Each entry has `model_name` (the identifier PikaTalk stores). `picoclaw model` prints the current default. `/list models` exists as a slash command but returns chat text, not a JSON list.

PikaTalk reads `model_list[].model_name` from that config file. The list is discovery/validation data only. Selected model remains PikaTalk local state (project default / chat override).

If a stored model is missing from the list, PikaTalk must not silently rewrite it.

## Selected model for a request

Inbound `payload.model_name` on `message.send` is **ignored** by PicoClaw 0.3.1. A live send with `model_name: "not-a-real-model-xyz"` still ran as `step-3.7-flash` (the agent default). `handleMessageSend` only forwards `content` (and media) into the agent.

Per-session model change is a slash command discovered via `/help`:

```text
/switch [model to <name>|channel] - Switch model
```

Phase 2 therefore:

1. Keeps the selected model in PikaTalk.
2. Includes `payload.model_name` on every user `message.send` (ignored by 0.3.1, kept for the request record).
3. Sends `/switch model to <selected-model>` on that chat's Pico session **only when** the selected model differs from `agents.defaults.model_name` (or from the model last applied on that session). The switch is a full agent turn. PikaTalk waits for a non-thought `message.create`/`message.update` and does **not** persist that confirmation as chat history, then sends the user `content`.
4. Does not send `/switch` in the same serialized turn as the user message. Doing so on 0.3.1 prevents the user turn from completing.

Do not change the user's global `picoclaw model` default from PikaTalk.

## Workspace

PikaTalk continues to own per-project / per-chat workspace strings.

PicoClaw 0.3.1 Pico `message.send` does **not** consume a workspace field. Agent execution uses `agents.defaults.workspace` (here `~/.picoclaw/workspace`) with `restrict_to_workspace: true`.

PikaTalk still includes `payload.workspace` on `message.send` so the request documents the active PikaTalk workspace. That field is not required for this gateway version and is not observed to change PicoClaw's execution root.

## Session / conversation context

* PikaTalk sends the current user message as `payload.content`.
* PicoClaw keeps its own session keyed by `session_id`.
* PikaTalk uses one stable session id per local chat (`pikatalk-chat-<id>`) so follow-up turns share PicoClaw session context without making PicoClaw the history owner.
* PikaTalk persists user and final assistant messages in `pikatalk.sqlite` regardless of PicoClaw session files under `~/.picoclaw/workspace/sessions/`.

Retry/regenerate (Phase 2): resend the last user `content` on the same session and replace or append the local assistant message according to the supported case. PicoClaw's session log may contain extra turns; PikaTalk's SQLite history remains the UI source of truth.

## Errors and reconnect

| Situation | Observed / expected |
| --- | --- |
| Unreachable host/port | TCP failure; PikaTalk connection state `error` / disconnected |
| Missing/wrong token | HTTP 401 on `/pico/ws` |
| Unknown JSON `type` | server `error` with `unknown_type` |
| Empty content | server `error` with `empty_content` |
| Temporary disconnect | WebSocket close; PikaTalk reconnects without mutating projects/chats/drafts |

Server error shape:

```json
{
  "type": "error",
  "payload": {
    "code": "empty_content",
    "message": "message content is empty",
    "request_id": "m1"
  }
}
```

## Out of scope (do not use as chat transport)

* Direct calls to model provider HTTP APIs
* Treating PicoClaw session JSONL as PikaTalk conversation history
* Launcher routes unrelated to gateway lifecycle (config editor, logs viewer, models catalog UI, etc.)

## Gateway lifecycle (Phase 4)

Observed on PicoClaw 0.3.1 with `picoclaw-launcher` on `127.0.0.1:18800` and chat gateway on `127.0.0.1:18790`.

| Layer | Bind | Role |
| --- | --- | --- |
| Chat gateway | `127.0.0.1:18790` | Pico Protocol WebSocket + `GET /health` |
| Launcher | `127.0.0.1:18800` | Dashboard + **gateway start/stop/restart** |

Starting `picoclaw-launcher` may also start the chat gateway process automatically when a default model is configured.

### Authentication

Unauthenticated:

* `GET /api/auth/status` → `{"authenticated":bool,"initialized":bool}`

First-time setup (when `initialized` is false):

```http
POST /api/auth/setup
{"password":"...","confirm":"..."}
```

Login (required for lifecycle routes):

```http
POST /api/auth/login
{"password":"..."}
```

Success sets HttpOnly cookie `picoclaw_launcher_auth` (`SameSite=Lax`). Subsequent lifecycle calls must send that cookie. Wrong password → `401 {"error":"invalid password"}`. Missing auth → `401 {"error":"unauthorized"}`.

The Pico channel chat token is **not** accepted as launcher auth.

PikaTalk stores the launcher password only in local config (`picoClaw/launcherPassword`). Never commit it.

### Status and version

```http
GET /api/gateway/status
```

Sanitized running example:

```json
{
  "gateway_status": "running",
  "gateway_version": "0.3.1",
  "pid": 67209,
  "gateway_start_allowed": true,
  "gateway_restart_required": false,
  "config_default_model": "step-3.7-flash",
  "boot_default_model": "step-3.7-flash"
}
```

Stopped example omits `pid` / `gateway_version` and sets `"gateway_status":"stopped"`.

When start is blocked (e.g. missing model credentials):

* `gateway_start_allowed`: false
* `gateway_start_reason`: human-readable string
* `POST /api/gateway/start` → `400 {"status":"precondition_failed","message":"..."}`

```http
GET /api/system/version
```

```json
{
  "version": "0.3.1",
  "git_commit": "2cf030d2",
  "build_time": "2026-07-03T07:10:50Z",
  "go_version": "1.25.11"
}
```

`GET /api/version` is **404** on this launcher build.

Complementary chat reachability (no launcher auth):

```http
GET http://127.0.0.1:18790/health
→ {"status":"ok","uptime":"...","pid":...}
```

### Start / stop / restart

All require launcher session cookie.

| Method | Path | Observed success body |
| --- | --- | --- |
| POST | `/api/gateway/start` | `{"status":"ok","pid":...}` (also ok if already running) |
| POST | `/api/gateway/stop` | `{"status":"ok","pid":...}` or `{"status":"not_running"}` |
| POST | `/api/gateway/restart` | `{"status":"ok","pid":...}` (new pid) |

After stop, chat `GET /health` fails to connect. After start/restart, chat health returns ok again.

Failure examples:

* Launcher down / wrong port → TCP connection failure
* No cookie → `401 {"error":"unauthorized"}`
* Start blocked by credentials → `400 precondition_failed`

See `decisions/0005-picoclaw-launcher-lifecycle.md`.

### Developer warning: temporary launchers

Live Phase 4 discovery/tests must not leave a disposable `picoclaw-launcher` on `127.0.0.1:18800`. That process owns the dashboard auth DB for its `HOME`. A leftover temp launcher makes the user’s normal password fail and blocks the desktop menu from starting another instance. Prefer the user’s running launcher, or stop the regular one only for the duration of a temp probe and restore it afterward. See `docs/development.md` (Live PicoClaw launcher hygiene).

## Tool activity (Phase 3)

Observed on PicoClaw 0.3.1 with real tool-using Pico Protocol turns.

### Tool-call event on the WebSocket

Tool calls arrive as:

```text
type = message.create
payload.kind = "tool_calls"
```

Sanitized successful `list_dir` call (live probe `pikatalk-phase3-toolprobe`):

```json
{
  "type": "message.create",
  "payload": {
    "kind": "tool_calls",
    "model_name": "step-3.7-flash",
    "content": "",
    "message_id": "<uuid>",
    "tool_calls": [
      {
        "id": "chatcmpl-tool-847e14db40650740",
        "type": "function",
        "function": {
          "name": "list_dir",
          "arguments": "{\n  \"path\": \".\"\n}"
        },
        "extra_content": {
          "tool_feedback_explanation": "Continuing the current task.: ..."
        }
      }
    ]
  }
}
```

Stable fields to persist from the wire:

| Field | Meaning |
| --- | --- |
| `payload.message_id` | Pico message id for this tool-call event |
| `payload.tool_calls[].id` | Tool call id (also used as `tool_call_id` in results) |
| `payload.tool_calls[].type` | Usually `function` |
| `payload.tool_calls[].function.name` | Tool name |
| `payload.tool_calls[].function.arguments` | JSON string of inputs |
| `payload.tool_calls[].extra_content` | Optional metadata (e.g. feedback explanation) |

Ordering observed for a short tool turn:

1. `typing.start` / `typing.stop`
2. `message.create` `kind=thought` (ignore for tool UX)
3. `message.create` `kind=tool_calls`
4. optional further thought
5. final non-thought `message.create` (user-visible assistant text)

### Tool results are not on the Pico WebSocket

Live probes never received a `message.create`/`message.update` carrying tool results. Results are written only into PicoClaw's own session JSONL under `~/.picoclaw/workspace/sessions/`.

Successful result (session JSONL, sanitized):

```json
{
  "role": "tool",
  "tool_call_id": "chatcmpl-tool-847e14db40650740",
  "content": "FILE: AGENT.md\nFILE: HEARTBEAT.md\n...",
  "created_at": "..."
}
```

Failed result (live probe `list_dir` on `/home/naorw`):

```json
{
  "role": "tool",
  "tool_call_id": "chatcmpl-tool-924a89f6ec4df5ef",
  "content": "failed to read directory: path escapes workspace: /home/naorw",
  "created_at": "..."
}
```

There is no separate structured `status` / `error` field on these session rows. Failure is represented as ordinary `content` text that describes the error.

Session files are located by scanning `*.meta.json` for:

```text
scope.values.chat = "direct:pico:<pikatalk-session-id>"
```

PikaTalk may read those result rows **only** to attach tool results to call ids already observed on the WebSocket. PicoClaw session logs remain not the source of truth for conversation history.

### Deliberately ignored

* Thought / reasoning events (`kind=thought`)
* `extra_content.tool_feedback_explanation` may be stored in raw JSON but is not required for the compact UI
* Treating session JSONL assistant/user rows as PikaTalk messages

### Protocol limitations

* Pico Protocol WebSocket exposes tool **calls**, not tool **results**, on 0.3.1
* Tool result success vs failure is inferred from result text when needed; there is no boolean status on the wire
* Multiple tools in one `tool_calls` array are possible in the schema; live probes exercised a single call at a time
* Enabling PicoClaw `agents.defaults.tool_feedback` was not required for receiving `kind=tool_calls`

### PikaTalk persistence and UI (Phase 3)

* WebSocket `kind=tool_calls` rows are stored in SQLite `tool_activities` (schema v2), not as assistant messages.
* Matching session JSONL `role=tool` content is attached by `tool_call_id` only (see `decisions/0004-tool-activity-persistence.md`).
* Compact UI shows `Tool: <name> — ok|failed|running` with expandable input/result; `raw_call_json` is retained but not shown by default.
* Thought events remain ignored.

## Desktop workspace actions (Phase 3)

These actions open the **PikaTalk active workspace** shown in the context bar (`currentWorkspace`). They do **not** change PicoClaw's execution root and do not introduce a second workspace concept.

| Action | Default command |
| --- | --- |
| Open folder | `xdg-open <workspace>` |
| Open terminal | `konsole --workdir <workspace>` (override: QSettings `desktop/terminalCommand`) |
| Open editor | `kate <workspace>` (override: QSettings `desktop/editorCommand`) |

Missing or non-directory workspaces fail with a visible non-destructive error (`workspaceActionError`).
