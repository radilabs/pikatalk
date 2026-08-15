# Local storage paths

PikaTalk stores local files using the XDG Base Directory Specification through Qt `QStandardPaths`.

It does not create a custom directory directly under the home directory.

Application identity used for these paths:

* Organization: `Radilabs`
* Application: `PikaTalk`
* Desktop file / app ID: `org.radilabs.pikatalk`

## Default locations

On Linux these resolve to:

| Kind | Qt API | Default path |
| --- | --- | --- |
| Application data | `QStandardPaths::AppDataLocation` | `$XDG_DATA_HOME/Radilabs/PikaTalk` |
| Configuration | `QStandardPaths::AppConfigLocation` | `$XDG_CONFIG_HOME/Radilabs/PikaTalk` |
| Cache | `QStandardPaths::CacheLocation` | `$XDG_CACHE_HOME/Radilabs/PikaTalk` |

If the XDG environment variables are unset, the usual defaults apply:

* `XDG_DATA_HOME` → `~/.local/share`
* `XDG_CONFIG_HOME` → `~/.config`
* `XDG_CACHE_HOME` → `~/.cache`

So a normal user session uses:

* `~/.local/share/Radilabs/PikaTalk`
* `~/.config/Radilabs/PikaTalk`
* `~/.cache/Radilabs/PikaTalk`

PikaTalk creates these directories on startup if they do not already exist.

## Overrides

Setting `XDG_DATA_HOME`, `XDG_CONFIG_HOME`, or `XDG_CACHE_HOME` before launch changes the resolved locations. This is useful for tests and isolated runs.

Example:

```bash
XDG_DATA_HOME=/tmp/pikatalk-xdg/data \
XDG_CONFIG_HOME=/tmp/pikatalk-xdg/config \
XDG_CACHE_HOME=/tmp/pikatalk-xdg/cache \
./build/bin/pikatalk
```

Resolved paths are logged at startup:

```text
PikaTalk data directory: ...
PikaTalk config directory: ...
PikaTalk cache directory: ...
```

## Phase 0 contents

Phase 0 proves that a SQLite database can be created under the application data directory.

The Phase 0 database file is:

* `$XDG_DATA_HOME/Radilabs/PikaTalk/phase0.sqlite`

It contains only a temporary `phase0_init` marker table. This is not the Phase 1 schema and must not be treated as the final data model.

Real project, chat, message, draft, model, and gateway state begins in Phase 1.
