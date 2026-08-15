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
PikaTalk sqlite database: ...
```

## Phase 1 database

The production database is:

* `$XDG_DATA_HOME/Radilabs/PikaTalk/pikatalk.sqlite`

Do not reuse `phase0.sqlite`. That file was a Phase 0 proof with a `phase0_init` marker table. It is not migrated. If it still exists on disk, ignore it.

Schema decisions future work must respect are in `decisions/0002-local-sqlite-schema.md`.

### Versioning

Table `schema_version(version INTEGER PRIMARY KEY)` stores a single integer.

Current version: **2**.

On open:

* If no version row exists, create version 2 tables and insert version `2`.
* If the stored version is `1`, migrate to `2` by adding `tool_activities`.
* If the stored version is not supported, open fails. Do not guess.
* Later schema changes add version `n+1` and a migration from `n` to `n+1`. There is no ORM.

`PRAGMA foreign_keys = ON` is required.

### Tables (version 2)

**projects**

* `id`, `name`, `default_workspace`, `default_model`, `created_at`, `sort_order`
* `default_workspace` and `default_model` are stored strings. Empty means unset. They are not discovered from a gateway.

**chats**

* `id`, `project_id` (FK to `projects.id` ON DELETE CASCADE)
* `title`, `workspace_override`, `model_override`, `archived`, `created_at`, `last_active_at`, `sort_order`
* A chat belongs to one project.
* `workspace_override` / `model_override` are NULL when the chat inherits the project default, and non-NULL when overridden (including an explicit empty string).
* `archived = 1` hides the chat from the default list without deleting messages or drafts.
* Chat list order is `last_active_at DESC`.

**messages**

* `id`, `chat_id` (FK to `chats.id` ON DELETE CASCADE)
* `role` CHECK (`user` or `assistant`)
* `content`, `created_at`, `position`
* `position` starts at 1 per chat and is the stable display order.
* Tool activity is **not** stored as message roles. See `tool_activities`.

**drafts**

* `chat_id` PRIMARY KEY (FK to `chats.id` ON DELETE CASCADE)
* `content`, `updated_at`
* One unfinished input per chat. Saving a draft does not create a message. Submit (Send) creates a user message and clears the draft.

**tool_activities** (added in version 2)

* `id`, `chat_id` (FK to `chats.id` ON DELETE CASCADE)
* `message_id` (nullable FK to `messages.id` ON DELETE SET NULL) — optional link to the following assistant message for the turn
* `tool_call_id`, `tool_name`, `arguments_json`, `raw_call_json`
* `result_text`, `status` (`running` / `ok` / `error` / `unknown`), `error_text`
* `created_at`, `position` — stable order within the chat
* Deleting a chat cascades tool rows. See `decisions/0004-tool-activity-persistence.md`.

### Inheritance

Effective workspace:

1. If `chats.workspace_override` is not NULL, use it.
2. Otherwise use `projects.default_workspace`.

Effective model:

1. If `chats.model_override` is not NULL, use it.
2. Otherwise use `projects.default_model`.

New chats are created with NULL overrides, so they inherit. Changing a project default updates inheriting chats. Clearing an override sets the column back to NULL.

Deleting a project cascades to its chats, messages, drafts, and tool activities.

## Phase 2 gateway configuration

Conversation history stays in `pikatalk.sqlite`. PikaClaw connection settings are **not** stored in SQLite.

File: `$XDG_CONFIG_HOME/Radilabs/PikaTalk/pikatalk.conf`

Keys under `[picoClaw]`:

* `endpoint` — default `ws://127.0.0.1:18790/pico/ws`
* `token` — Pico channel bearer token. If empty, PikaTalk reads `~/.picoclaw/.security.yml`
* `configPath` — PicoClaw `config.json` used for `model_list` discovery

See `docs/pikaclaw-api.md` and `decisions/0003-pico-protocol-chat-transport.md`.

## Phase 3 desktop actions

Open-folder / open-terminal / open-editor use the effective active workspace from inheritance above. They do not write workspace paths into PicoClaw config. Optional launcher overrides live in the same QSettings org as other app prefs:

* `desktop/terminalCommand`
* `desktop/editorCommand`

Copy-message and copy-code use the system clipboard via `AppController::copyText`. Code blocks are detected as Markdown fenced ``` segments in message content; they are not stored separately.
