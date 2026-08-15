# Phase 2 Handoff — PikaClaw Chat

## Deliverables

Meaningful files produced or updated:

* `src/pikaclawclient.h` / `src/pikaclawclient.cpp` — Pico Protocol WebSocket client over `QTcpSocket` (handshake, masked client frames, server ping/pong, fragmented text frames)
* `src/pikaclawsettings.h` / `src/pikaclawsettings.cpp` — endpoint/token/`config.json` loading; Pico channel token from `~/.picoclaw/.security.yml` when unset; `model_list` and `agents.defaults.model_name`
* `src/appcontroller.h` / `src/appcontroller.cpp` — gateway connect/reconnect, model discovery, real send/stream/stop/retry, session URL `session_id` matching JSON `session_id`
* `src/database.cpp` — `updateMessageContent` / `deleteMessage`; empty drafts stored as `""` not SQL NULL
* `src/Main.qml` — Send through PikaClaw; Stop; Retry/Regenerate; streaming label; gateway Connected/Connecting/Error/Disconnected; model ComboBox
* `src/main.cpp` — loads gateway settings and connects on startup
* `src/CMakeLists.txt` — links `Qt6::Network`; client/settings sources
* `tests/fake_pico_server.h` — local WebSocket double
* `tests/pikaclawclient_test.cpp` / `tests/appcontroller_test.cpp` / `tests/CMakeLists.txt`
* `docs/pikaclaw-api.md` — observed PicoClaw 0.3.1 chat API
* `docs/development.md` / `docs/local-storage.md` / `README.md`
* `decisions/0003-pico-protocol-chat-transport.md`
* `tasks/phase-2-pikaclaw-chat.md` — execution evidence
* `docs/handoffs/phase-2.md` — this report

Built artifact on the development machine:

* `build/bin/pikatalk`

Gateway used for live evidence:

* PicoClaw 0.3.1 (`picoclaw gateway -E`)
* Chat bind `127.0.0.1:18790`
* WebSocket `ws://127.0.0.1:18790/pico/ws?session_id=pikatalk-chat-<id>`

PikaTalk continues to own projects, chats, messages, drafts, workspace state, and selected model state in `pikatalk.sqlite`. PicoClaw is execution/gateway only.

## Tests Performed

* `cmake --build build`
* `ctest --test-dir build --output-on-failure` — 5/5 passed:

  * appstreamtest
  * applicationpaths_test
  * database_test
  * appcontroller_test
  * pikaclawclient_test

* Real local gateway:

  ```bash
  PIKATALK_LIVE_GATEWAY=1 ./build/bin/appcontroller_test liveGatewaySendIfEnabled
  ```

  Result (2026-08-15): **PASS** in 4769ms. Connected to the running PicoClaw gateway, sent `Reply with exactly the word LIVEOK and nothing else.`, persisted one user row and one assistant row whose content contained `LIVEOK`.

* Independent stdlib WebSocket probe (same host/token/path rules) also received `LIVEOK` as a non-thought `message.create`.

* Cancellation `/stop` confirmed earlier against the live gateway during API discovery and by `stopGenerationEndsTurnAndPreservesHistory` against `FakePicoServer`.

* Phase 1 persistence/isolation/draft/inheritance tests still pass inside `appcontroller_test` and `database_test`.

## Results

Phase 2 acceptance criteria from `PHASES.md`:

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | PikaTalk can connect to the configured PikaClaw gateway | PASS | `pikaclawclient_test::connectsWhenGatewayIsReachable`; live test `gatewayState == connected`; `GET /health` returned `{"status":"ok",...}` |
| 2 | Available models can be retrieved where the gateway exposes them | PASS | Chat HTTP `/v1/models` is 404; `loadPicoClawModelNames` reads `model_list[].model_name` from PicoClaw `config.json`; ComboBox uses `availableModels` |
| 3 | The selected model is used for a conversation request | PASS | Request includes `payload.model_name`. 0.3.1 ignores that field. PikaTalk sends `/switch model to <name>` when the selection is not `agents.defaults.model_name` and waits for confirmation before the user turn (`sendChatRequestUsesModelWorkspaceAndPersistsUser`, `sendSkipsSwitchWhenSelectedMatchesGatewayDefault`). Live send used `step-3.7-flash` (gateway default) |
| 4 | The active workspace is supplied correctly to PikaClaw where required | PASS | `payload.workspace` is sent on user `message.send`. 0.3.1 does not consume it; execution uses `agents.defaults.workspace`. Documented in `docs/pikaclaw-api.md` |
| 5 | A user message can produce a real assistant response | PASS | Live `liveGatewaySendIfEnabled` persisted assistant `LIVEOK` |
| 6 | Responses stream visibly into the chat | PASS | `streamsAndPersistsFinalAssistant` applies `message.update` into `streamingAssistantText` and one assistant row; QML shows streaming text. Live 0.3.1 short replies arrived as one `message.create` (documented) |
| 7 | Completed responses are persisted locally | PASS | Live test two messages after send; reopen path in `streamsAndPersistsFinalAssistant` |
| 8 | Generation can be stopped | PASS | `/stop` on the same session; `stopGenerationEndsTurnAndPreservesHistory`; `Task stopped.` is not stored as history |
| 9 | Retry/regenerate behaves correctly for the supported case | PASS | `retryRegeneratesLatestAssistant`: delete last assistant, resend last user content, persist replacement |
| 10 | Gateway/network failure produces a clear error without losing local history | PASS | `gatewayFailureDoesNotAlterLocalState`; `gatewayErrorPreservesDraftAndHistory`; UI `gatewayState` / `requestError` |
| 11 | Draft text is not lost because the gateway fails | PASS | Failed send does not consume draft (`gatewayErrorPreservesDraftAndHistory`) |
| 12 | Reconnection after a temporary gateway failure works | PASS | `gatewayReconnectsWithoutAlteringLocalState`; auto-reconnect timer; send works after recovery |
| 13 | Restarting PikaTalk still restores locally owned conversation history | PASS | `streamsAndPersistsFinalAssistant` reopen; existing Phase 1 reopen tests |
| 14 | Phase 1 functionality does not regress | PASS | Phase 1 `appcontroller_test` / `database_test` cases still pass; schema version 1 unchanged |

## Known Limitations

* PicoClaw 0.3.1 ignores inbound `payload.model_name`. Per-session model change is `/switch model to <name>`, which is a full agent turn. PikaTalk must not send it in the same serialized slot as the user message.
* PicoClaw 0.3.1 requires `/pico/ws?session_id=` to match JSON `session_id`. PikaTalk reconnects the WebSocket when the active chat changes.
* PicoClaw 0.3.1 does not apply `payload.workspace` on Pico `message.send`.
* Short live replies often arrive as one complete `message.create` rather than a series of `message.update` events. There is no `turn.done` event; completion is inferred from a non-thought create/update.
* Thoughts and `tool_calls` are ignored, not persisted.
* Partial assistant text is discarded on Stop unless a non-thought assistant row was already persisted.
* Qt WebSockets was not available; the client is a `QTcpSocket` implementation of Pico Protocol.
* Running from `build/bin/pikatalk` without install may still log `xdg-desktop-portal` `App info not found for 'org.radilabs.pikatalk'`.
* CMake/ECM may print `fatal: no upstream configured for branch ...` on a local-only git branch. Configure still succeeds.

## Deferred Work

From `tasks/phase-2-pikaclaw-chat.md` (not implemented):

* detailed tool-call/result persistence and UI → Phase 3
* open workspace in terminal/editor/file manager → Phase 3
* per-request workspace consumption if a later gateway honors `payload.workspace` → Phase 3/future
* gateway start/stop/restart → Phase 4
* detailed gateway monitoring → Phase 4
* search, tray, notifications, polish → Phase 5
* conversation branching → Future
* provider-direct integrations → Out of scope unless a future contract says otherwise

## Decisions

* `decisions/0002-local-sqlite-schema.md` — local history ownership; schema version 1
* `decisions/0003-pico-protocol-chat-transport.md` — Pico Protocol WebSocket is the only chat transport; matching URL/JSON `session_id`; no provider-direct calls; no launcher lifecycle APIs

```text
PHASE 2 HANDOFF: PASS
```
