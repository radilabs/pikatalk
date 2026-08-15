# Phase 4 Handoff — Gateway Management

## Deliverables

* `src/picoclawlifecycle.h` / `src/picoclawlifecycle.cpp` — PicoClaw launcher HTTP lifecycle client (login cookie, status, version, start/stop/restart)
* `src/appcontroller.*` — unified lifecycle + chat reconnect; Start/Stop/Restart actions; endpoint/version/error properties
* `src/pikaclawsettings.*` — `picoClaw/launcherUrl`, `picoClaw/launcherPassword` (or `PIKATALK_LAUNCHER_PASSWORD`)
* `src/Main.qml` — gateway state/endpoint/version labels; Start/Stop/Restart buttons
* `tests/fake_launcher_server.h`, `tests/picoclawlifecycle_test.cpp`, lifecycle cases in `tests/appcontroller_test.cpp`
* `docs/pikaclaw-api.md` — Gateway lifecycle section
* `decisions/0005-picoclaw-launcher-lifecycle.md`
* `tasks/phase-4-gateway-management.md` — execution evidence
* `docs/handoffs/phase-4.md` — this report

Built: `build/bin/pikatalk`

Mechanism: PicoClaw 0.3.1 `picoclaw-launcher` on `http://127.0.0.1:18800` (cookie `picoclaw_launcher_auth`). Chat remains Pico Protocol on `ws://127.0.0.1:18790/pico/ws`.

## Tests Performed

* `ctest --test-dir build` — **6/6 PASS** (includes `pikaclawlifecycle_test`)
* `./build/bin/pikaclawlifecycle_test` — 6/6 PASS
* `./build/bin/appcontroller_test gatewayLifecycleControlsPreserveLocalState` — PASS (stop + unreachable launcher error; drafts/messages preserved)
* Live (gated):

```bash
PIKATALK_LIVE_GATEWAY=1 PIKATALK_LAUNCHER_PASSWORD=… \
  ./build/bin/appcontroller_test liveGatewayLifecycleIfEnabled
```

  **PASS** — stop → start → reconnect → `LIFEOK` chat → restart → reconnect; draft/history preserved
* Phase 2 regression: `PIKATALK_LIVE_GATEWAY=1 liveGatewaySendIfEnabled` — PASS

## Results

| # | Criterion | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Detects whether configured local gateway is available | PASS | Launcher `/api/gateway/status` + chat `/health` |
| 2 | Gateway state visible in normal chat UI | PASS | Context labels for connected/stopped/reconnecting/error |
| 3 | Gateway endpoint can be seen | PASS | `gatewayEndpointDisplay` |
| 4 | Gateway version visible where exposed | PASS | `gatewayVersion` from status/system version |
| 5 | Stopped gateway can be started | PASS | Live `startLocalGateway` |
| 6 | Running gateway can be stopped | PASS | Live `stopLocalGateway` |
| 7 | Running gateway can be restarted | PASS | Live `restartLocalGateway` |
| 8 | Reconnects after successful restart | PASS | Live `gatewayState==connected` after restart |
| 9 | Restart does not lose projects/chats/messages/drafts | PASS | Live draft + message counts unchanged across stop/start/restart |
| 10 | Failed lifecycle commands produce understandable errors | PASS | Unit bad launcher URL → `lifecycleError`; unauthorized/precondition in client tests |
| 11 | Phase 3 behavior does not regress | PASS | Full `ctest`; live chat send |

## Known Limitations

* Lifecycle requires `picoclaw-launcher` and a configured launcher password (`picoClaw/launcherPassword` or env). Chat can still work if the gateway process is up without the launcher, but Start/Stop/Restart will not.
* Starting the launcher may auto-start the gateway; PikaTalk still uses explicit API commands for control.
* Live lifecycle tests are gated (`PIKATALK_LIVE_GATEWAY` + launcher password) so default `ctest` does not stop a developer gateway.
* Temporary/test launchers on port `18800` must be torn down after use; leaving one running replaces the user’s dashboard auth context. Documented under `docs/development.md` and `docs/pikaclaw-api.md`.
* No remote/fleet/systemd/config-editor/metrics features (by contract).

## Deferred Work

* Gateway install/upgrade → Out of scope
* Launcher config editor / logs viewer → Out of scope / Future
* Search, tray, notifications, packaging polish → Phase 5
* Honoring `payload.workspace` in PicoClaw → gateway capability / Future

## Decisions

* `decisions/0003-pico-protocol-chat-transport.md` — chat transport unchanged
* `decisions/0004-tool-activity-persistence.md` — tool activity ownership unchanged
* `decisions/0005-picoclaw-launcher-lifecycle.md` — launcher HTTP is the only lifecycle API

```text
PHASE 4 HANDOFF: PASS
```
