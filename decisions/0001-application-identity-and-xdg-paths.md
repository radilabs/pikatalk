# ADR 0001: Application identity and XDG paths

## Status

Accepted

## Context

PikaTalk needs stable local filesystem locations for application data, configuration, and cache. Later phases will persist projects, chats, and related state in those locations. The Phase 0 contract requires normal Linux/XDG conventions rather than a custom directory under the user's home directory.

Qt 6 already maps XDG directories through `QStandardPaths` when organization and application names are set.

## Decision

Use this application identity:

* Organization name: `Radilabs`
* Organization domain: `radilabs.org`
* Application name: `PikaTalk`
* Desktop file / app ID: `org.radilabs.pikatalk`

Resolve local paths with Qt `QStandardPaths`:

* data: `AppDataLocation`
* config: `AppConfigLocation`
* cache: `CacheLocation`

Do not introduce a `~/.pikatalk` directory or any other home-directory layout outside XDG.

## Consequences

Default user paths are:

* `~/.local/share/Radilabs/PikaTalk`
* `~/.config/Radilabs/PikaTalk`
* `~/.cache/Radilabs/PikaTalk`

These locations change if `XDG_DATA_HOME`, `XDG_CONFIG_HOME`, or `XDG_CACHE_HOME` are set.

Later phases must store persistent state under these resolved locations. Changing the organization name, application name, or app ID would relocate user data and must be treated as a compatibility decision.
