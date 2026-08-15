# ADR 0002: Local SQLite schema and migrations

## Status

Accepted

## Context

Phase 1 requires persistent projects, chats, messages, and drafts. Phase 0 used a separate proof file `phase0.sqlite` with a marker table that is not the product schema. Later phases will add tool records, gateway settings, and other fields. Future work must extend this store without replacing the conversation model.

## Decision

* Store Phase 1 data in `$XDG_DATA_HOME/Radilabs/PikaTalk/pikatalk.sqlite`.
* Do not reuse or migrate `phase0.sqlite`. It remains a leftover proof file and may be ignored.
* Track schema with table `schema_version(version INTEGER PRIMARY KEY)`.
* Current version is `1`.
* On open, if no version row exists, create version 1 tables. If the stored version is not supported, fail openly rather than guessing.
* Future schema changes add a new integer version and a migration from `n` to `n+1`. Do not introduce an ORM.
* `chats.workspace_override` and `chats.model_override` are NULL when the chat inherits the project default, and non-NULL when overridden.
* Deleting a project cascades to its chats, messages, and drafts.
* Message `role` is `user` or `assistant`. Tool activity is not stored in version 1. Later phases may add tables or columns keyed to `messages.id` without replacing this table.

## Consequences

Phase 1 and later code must open `pikatalk.sqlite`, honor `schema_version`, and treat NULL chat overrides as inheritance. Cascading project delete is the supported cleanup behavior.
