# ADR 0004: Tool activity persistence for PicoClaw

## Status

Accepted

## Context

Phase 3 must persist PicoClaw tool calls and results without replacing the existing `messages` table (ADR 0002). Pico Protocol on PicoClaw 0.3.1 publishes tool calls as `message.create` with `payload.kind = "tool_calls"`, but does not publish tool results on the WebSocket. Results appear only as `role: "tool"` rows in PicoClaw session JSONL, keyed by `tool_call_id`.

PikaTalk must not adopt PicoClaw session logs as conversation history (ADR 0003), but Phase 3 still needs durable tool results where PicoClaw exposes them.

## Decision

* Keep `messages` for user/assistant text only. Do not store tool activity as `role` values on `messages`.
* Add a dedicated `tool_activities` table in schema version `2`, linked to `chats.id` and optionally to the following assistant `messages.id` for the same turn.
* Persist tool-call fields from the WebSocket event, including raw `arguments` and the full raw call JSON.
* After a call is observed, attach the matching session JSONL `role:tool` result by `tool_call_id` when available. Store raw result text and a coarse status (`running`, `ok`, `error`, `unknown`).
* Reading PicoClaw session files is allowed only to enrich tool results for call ids already seen on the wire. Session user/assistant rows are not imported as PikaTalk history.

## Consequences

Schema version becomes `2` with a migration from `1`. Future providers that emit results on the wire can fill the same table without changing message ownership. Compact tool UI renders from `tool_activities`, not from assistant message content.
