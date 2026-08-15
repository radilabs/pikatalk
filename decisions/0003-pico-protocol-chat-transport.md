# ADR 0003: Pico Protocol is the PikaClaw chat transport

## Status

Accepted

## Context

Phase 2 must talk to the local PikaClaw gateway for real conversations. On the development machine the available product is PicoClaw 0.3.1. That gateway exposes `GET /health` and a Pico channel WebSocket. It does not expose OpenAI-compatible ` /v1/chat/completions` or `/v1/models` (HTTP 404). A separate launcher HTTP port exists for dashboard/lifecycle APIs; those are Phase 4.

PikaTalk already owns conversation history in SQLite (ADR 0002). PicoClaw also stores session logs. Future phases must not accidentally treat the gateway as the history owner or bypass it by calling model providers.

## Decision

* Use the Pico Protocol WebSocket (`/pico/ws`) as the only chat transport to the local gateway.
* Authenticate with the Pico channel token (`Authorization: Bearer` or `token.<token>` subprotocol). Do not commit tokens.
* Identify a turn's gateway session with a PikaTalk-owned `session_id` derived from the local chat id. On PicoClaw 0.3.1 that id must appear both as the `/pico/ws?session_id=` query parameter and as JSON `session_id`, and those two values must match. The id is execution correlation, not history ownership.
* Persist user messages and completed user-visible assistant messages in `pikatalk.sqlite`. Do not replace local history with PicoClaw session files.
* Do not call model providers from PikaTalk.
* Do not use launcher gateway start/stop/restart APIs in this phase.
* Do not introduce a generic multi-provider client abstraction.

## Consequences

Phase 2 client code is PicoClaw-specific. Streaming, stop (`/stop`), and per-session model switch (`/switch model to <name>`) follow Pico Protocol and slash commands documented in `docs/pikaclaw-api.md`. Later gateway products would need a new decision if they are not Pico Protocol compatible.
