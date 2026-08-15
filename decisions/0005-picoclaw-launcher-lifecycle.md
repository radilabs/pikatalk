# ADR 0005: PicoClaw launcher HTTP is the local gateway lifecycle API

## Status

Accepted

## Context

Phase 4 must start, stop, and restart the local PikaClaw gateway used by PikaTalk. Chat already uses Pico Protocol WebSocket on the chat bind port (ADR 0003). PicoClaw 0.3.1 also ships `picoclaw-launcher`, which listens on a separate HTTP port (default `127.0.0.1:18800`) and owns gateway process lifecycle.

Shell/process hacks and generic systemd management are out of Phase 4 scope.

## Decision

* Use the PicoClaw launcher HTTP API as the only supported gateway lifecycle mechanism in PikaTalk.
* Default lifecycle base URL: `http://127.0.0.1:18800`.
* Authenticate with the launcher dashboard password via `POST /api/auth/login` and the `picoclaw_launcher_auth` session cookie. Store the password only in local PikaTalk config (never in git).
* Lifecycle operations:
  * `GET /api/gateway/status`
  * `POST /api/gateway/start`
  * `POST /api/gateway/stop`
  * `POST /api/gateway/restart`
* Version: prefer `gateway_version` from status when running; otherwise `GET /api/system/version`.
* Keep chat transport (`/pico/ws` on the chat port) conceptually separate from lifecycle HTTP.
* Use unauthenticated chat `GET /health` only as a complementary reachability signal, not as a process manager.
* Do not implement remote/fleet lifecycle, generic process discovery, or a PicoClaw configuration editor.
* Do not leave temporary/test `picoclaw-launcher` processes bound to the developer’s default port `18800` after a session. Prefer the user’s running launcher, or stop/restore the regular launcher around disposable probes (see `docs/development.md`).

## Consequences

PikaTalk Phase 4 requires the launcher to be running (or startable) for start/stop/restart. Chat may still be reachable when the gateway process is up even if the launcher is down; lifecycle controls will report launcher unavailability in that case. A leftover temp launcher on `18800` will reject the user’s normal dashboard password until the real launcher is restored.
