#include "database.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

namespace
{
constexpr int kSchemaVersion = 2;

bool createToolActivitiesTable(QSqlDatabase &db, QString *error)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS tool_activities ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " chat_id INTEGER NOT NULL REFERENCES chats(id) ON DELETE CASCADE,"
            " message_id INTEGER REFERENCES messages(id) ON DELETE SET NULL,"
            " tool_call_id TEXT NOT NULL,"
            " tool_name TEXT NOT NULL,"
            " arguments_json TEXT NOT NULL DEFAULT '',"
            " raw_call_json TEXT NOT NULL DEFAULT '',"
            " result_text TEXT NOT NULL DEFAULT '',"
            " status TEXT NOT NULL DEFAULT 'unknown',"
            " error_text TEXT NOT NULL DEFAULT '',"
            " created_at INTEGER NOT NULL,"
            " position INTEGER NOT NULL"
            ")"))) {
        if (error != nullptr) {
            *error = query.lastError().text();
        }
        return false;
    }
    return true;
}
}

LocalDatabase::LocalDatabase()
    : m_connectionName(QStringLiteral("pikatalk-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

LocalDatabase::~LocalDatabase()
{
    close();
}

QString LocalDatabase::databaseFilePath(const QString &dataDirectory)
{
    return QDir(dataDirectory).filePath(QStringLiteral("pikatalk.sqlite"));
}

void LocalDatabase::setError(QString *error, const QString &message) const
{
    if (error != nullptr) {
        *error = message;
    }
}

qint64 LocalDatabase::nowMs() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

bool LocalDatabase::exec(const QString &sql, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(sql)) {
        setError(error, query.lastError().text());
        return false;
    }
    return true;
}

bool LocalDatabase::isOpen() const
{
    return m_open;
}

void LocalDatabase::close()
{
    if (!m_open) {
        return;
    }
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isValid()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_open = false;
}

bool LocalDatabase::open(const QString &filePath, QString *error)
{
    if (filePath.isEmpty()) {
        setError(error, QStringLiteral("Database path is empty"));
        return false;
    }

    close();

    const QFileInfo info(filePath);
    if (!QDir().mkpath(info.absolutePath())) {
        setError(error, QStringLiteral("Failed to create database directory: %1").arg(info.absolutePath()));
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(filePath);
    if (!db.open()) {
        setError(error, QStringLiteral("Failed to open database %1: %2").arg(filePath, db.lastError().text()));
        QSqlDatabase::removeDatabase(m_connectionName);
        return false;
    }

    m_open = true;
    if (!exec(QStringLiteral("PRAGMA foreign_keys = ON"), error)) {
        close();
        return false;
    }

    if (!exec(QStringLiteral(
                  "CREATE TABLE IF NOT EXISTS schema_version ("
                  " version INTEGER PRIMARY KEY NOT NULL"
                  ")"),
              error)) {
        close();
        return false;
    }

    QSqlQuery versionQuery(QSqlDatabase::database(m_connectionName));
    if (!versionQuery.exec(QStringLiteral("SELECT version FROM schema_version LIMIT 1"))) {
        setError(error, versionQuery.lastError().text());
        close();
        return false;
    }

    int currentVersion = 0;
    if (versionQuery.next()) {
        currentVersion = versionQuery.value(0).toInt();
    }

    if (currentVersion == 0) {
        const QString statements[] = {
            QStringLiteral(
                "CREATE TABLE projects ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " name TEXT NOT NULL,"
                " default_workspace TEXT NOT NULL DEFAULT '',"
                " default_model TEXT NOT NULL DEFAULT '',"
                " created_at INTEGER NOT NULL,"
                " sort_order INTEGER NOT NULL DEFAULT 0"
                ")"),
            QStringLiteral(
                "CREATE TABLE chats ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,"
                " title TEXT NOT NULL,"
                " workspace_override TEXT,"
                " model_override TEXT,"
                " archived INTEGER NOT NULL DEFAULT 0,"
                " created_at INTEGER NOT NULL,"
                " last_active_at INTEGER NOT NULL,"
                " sort_order INTEGER NOT NULL DEFAULT 0"
                ")"),
            QStringLiteral(
                "CREATE TABLE messages ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " chat_id INTEGER NOT NULL REFERENCES chats(id) ON DELETE CASCADE,"
                " role TEXT NOT NULL CHECK (role IN ('user', 'assistant')),"
                " content TEXT NOT NULL,"
                " created_at INTEGER NOT NULL,"
                " position INTEGER NOT NULL"
                ")"),
            QStringLiteral(
                "CREATE TABLE drafts ("
                " chat_id INTEGER PRIMARY KEY REFERENCES chats(id) ON DELETE CASCADE,"
                " content TEXT NOT NULL DEFAULT '',"
                " updated_at INTEGER NOT NULL"
                ")"),
            QStringLiteral(
                "CREATE TABLE tool_activities ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " chat_id INTEGER NOT NULL REFERENCES chats(id) ON DELETE CASCADE,"
                " message_id INTEGER REFERENCES messages(id) ON DELETE SET NULL,"
                " tool_call_id TEXT NOT NULL,"
                " tool_name TEXT NOT NULL,"
                " arguments_json TEXT NOT NULL DEFAULT '',"
                " raw_call_json TEXT NOT NULL DEFAULT '',"
                " result_text TEXT NOT NULL DEFAULT '',"
                " status TEXT NOT NULL DEFAULT 'unknown',"
                " error_text TEXT NOT NULL DEFAULT '',"
                " created_at INTEGER NOT NULL,"
                " position INTEGER NOT NULL"
                ")"),
            QStringLiteral("INSERT INTO schema_version(version) VALUES (2)"),
        };
        for (const QString &sql : statements) {
            if (!exec(sql, error)) {
                close();
                return false;
            }
        }
    } else if (currentVersion == 1) {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        if (!createToolActivitiesTable(database, error)) {
            close();
            return false;
        }
        if (!exec(QStringLiteral("UPDATE schema_version SET version = 2"), error)) {
            close();
            return false;
        }
    } else if (currentVersion != kSchemaVersion) {
        setError(error, QStringLiteral("Unsupported schema version: %1").arg(currentVersion));
        close();
        return false;
    }

    return true;
}

int LocalDatabase::schemaVersion(QString *error) const
{
    if (!m_open) {
        setError(error, QStringLiteral("Database is not open"));
        return 0;
    }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("SELECT version FROM schema_version LIMIT 1")) || !query.next()) {
        setError(error, query.lastError().text());
        return 0;
    }
    return query.value(0).toInt();
}

QStringList LocalDatabase::tableNames(QString *error) const
{
    QStringList names;
    if (!m_open) {
        setError(error, QStringLiteral("Database is not open"));
        return names;
    }
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table'"))) {
        setError(error, query.lastError().text());
        return names;
    }
    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

qint64 LocalDatabase::createProject(const QString &name,
                                    const QString &defaultWorkspace,
                                    const QString &defaultModel,
                                    QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO projects(name, default_workspace, default_model, created_at, sort_order) "
        "VALUES(?, ?, ?, ?, COALESCE((SELECT MAX(sort_order) + 1 FROM projects), 1))"));
    query.addBindValue(name);
    query.addBindValue(defaultWorkspace);
    query.addBindValue(defaultModel);
    query.addBindValue(nowMs());
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

bool LocalDatabase::readProject(qint64 id,
                                QString *name,
                                QString *defaultWorkspace,
                                QString *defaultModel,
                                QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT name, default_workspace, default_model FROM projects WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Project not found"));
        return false;
    }
    if (name != nullptr) {
        *name = query.value(0).toString();
    }
    if (defaultWorkspace != nullptr) {
        *defaultWorkspace = query.value(1).toString();
    }
    if (defaultModel != nullptr) {
        *defaultModel = query.value(2).toString();
    }
    return true;
}

QList<qint64> LocalDatabase::listProjectIds(QString *error) const
{
    QList<qint64> ids;
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("SELECT id FROM projects ORDER BY sort_order, id"))) {
        setError(error, query.lastError().text());
        return ids;
    }
    while (query.next()) {
        ids.append(query.value(0).toLongLong());
    }
    return ids;
}

bool LocalDatabase::renameProject(qint64 id, const QString &name, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE projects SET name = ? WHERE id = ?"));
    query.addBindValue(name);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Project not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::deleteProject(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("DELETE FROM projects WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Project not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::setProjectDefaultWorkspace(qint64 id, const QString &workspace, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE projects SET default_workspace = ? WHERE id = ?"));
    query.addBindValue(workspace);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Project not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::setProjectDefaultModel(qint64 id, const QString &model, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE projects SET default_model = ? WHERE id = ?"));
    query.addBindValue(model);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Project not found"));
        return false;
    }
    return true;
}

qint64 LocalDatabase::createChat(qint64 projectId, const QString &title, QString *error)
{
    const qint64 now = nowMs();
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO chats(project_id, title, workspace_override, model_override, archived, created_at, last_active_at, sort_order) "
        "VALUES(?, ?, NULL, NULL, 0, ?, ?, "
        "COALESCE((SELECT MAX(sort_order) + 1 FROM chats WHERE project_id = ?), 1))"));
    query.addBindValue(projectId);
    query.addBindValue(title);
    query.addBindValue(now);
    query.addBindValue(now);
    query.addBindValue(projectId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

bool LocalDatabase::readChat(qint64 id, qint64 *projectId, QString *title, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT project_id, title FROM chats WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    if (projectId != nullptr) {
        *projectId = query.value(0).toLongLong();
    }
    if (title != nullptr) {
        *title = query.value(1).toString();
    }
    return true;
}

QList<qint64> LocalDatabase::listChatIds(qint64 projectId, bool includeArchived, QString *error) const
{
    QList<qint64> ids;
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (includeArchived) {
        query.prepare(QStringLiteral(
            "SELECT id FROM chats WHERE project_id = ? ORDER BY last_active_at DESC, id DESC"));
        query.addBindValue(projectId);
    } else {
        query.prepare(QStringLiteral(
            "SELECT id FROM chats WHERE project_id = ? AND archived = 0 ORDER BY last_active_at DESC, id DESC"));
        query.addBindValue(projectId);
    }
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return ids;
    }
    while (query.next()) {
        ids.append(query.value(0).toLongLong());
    }
    return ids;
}

bool LocalDatabase::renameChat(qint64 id, const QString &title, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET title = ? WHERE id = ?"));
    query.addBindValue(title);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::touchChat(qint64 id, QString *error)
{
    QSqlQuery maxQuery(QSqlDatabase::database(m_connectionName));
    if (!maxQuery.exec(QStringLiteral("SELECT COALESCE(MAX(last_active_at), 0) FROM chats")) || !maxQuery.next()) {
        setError(error, maxQuery.lastError().text());
        return false;
    }
    const qint64 nextActive = qMax(maxQuery.value(0).toLongLong() + 1, nowMs());

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET last_active_at = ? WHERE id = ?"));
    query.addBindValue(nextActive);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::archiveChat(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET archived = 1 WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::deleteChat(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("DELETE FROM chats WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::mostRecentlyActiveChat(qint64 *chatId, qint64 *projectId, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral(
            "SELECT id, project_id FROM chats WHERE archived = 0 "
            "ORDER BY last_active_at DESC, id DESC LIMIT 1"))) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        return false;
    }
    if (chatId != nullptr) {
        *chatId = query.value(0).toLongLong();
    }
    if (projectId != nullptr) {
        *projectId = query.value(1).toLongLong();
    }
    return true;
}

bool LocalDatabase::readChatWorkspaceOverride(qint64 id, bool *hasOverride, QString *override, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT workspace_override FROM chats WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    const QVariant value = query.value(0);
    const bool isNull = value.isNull();
    if (hasOverride != nullptr) {
        *hasOverride = !isNull;
    }
    if (override != nullptr) {
        *override = isNull ? QString() : value.toString();
    }
    return true;
}

bool LocalDatabase::setChatWorkspaceOverride(qint64 id, const QString &workspace, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET workspace_override = ? WHERE id = ?"));
    query.addBindValue(workspace);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::clearChatWorkspaceOverride(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET workspace_override = NULL WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        qint64 projectId = 0;
        QString title;
        if (!readChat(id, &projectId, &title, error)) {
            return false;
        }
    }
    return true;
}

bool LocalDatabase::readChatModelOverride(qint64 id, bool *hasOverride, QString *override, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT model_override FROM chats WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    const QVariant value = query.value(0);
    const bool isNull = value.isNull();
    if (hasOverride != nullptr) {
        *hasOverride = !isNull;
    }
    if (override != nullptr) {
        *override = isNull ? QString() : value.toString();
    }
    return true;
}

bool LocalDatabase::setChatModelOverride(qint64 id, const QString &model, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET model_override = ? WHERE id = ?"));
    query.addBindValue(model);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        setError(error, QStringLiteral("Chat not found"));
        return false;
    }
    return true;
}

bool LocalDatabase::clearChatModelOverride(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE chats SET model_override = NULL WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        qint64 projectId = 0;
        QString title;
        if (!readChat(id, &projectId, &title, error)) {
            return false;
        }
    }
    return true;
}

qint64 LocalDatabase::addMessage(qint64 chatId, const QString &role, const QString &content, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO messages(chat_id, role, content, created_at, position) "
        "VALUES(?, ?, ?, ?, COALESCE((SELECT MAX(position) + 1 FROM messages WHERE chat_id = ?), 1))"));
    query.addBindValue(chatId);
    query.addBindValue(role);
    query.addBindValue(content);
    query.addBindValue(nowMs());
    query.addBindValue(chatId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

bool LocalDatabase::updateMessageContent(qint64 id, const QString &content, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE messages SET content = ? WHERE id = ?"));
    query.addBindValue(content);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LocalDatabase::deleteMessage(qint64 id, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("DELETE FROM messages WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LocalDatabase::readMessage(qint64 id, qint64 *chatId, QString *role, QString *content, qint64 *position, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT chat_id, role, content, position FROM messages WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Message not found"));
        return false;
    }
    if (chatId != nullptr) {
        *chatId = query.value(0).toLongLong();
    }
    if (role != nullptr) {
        *role = query.value(1).toString();
    }
    if (content != nullptr) {
        *content = query.value(2).toString();
    }
    if (position != nullptr) {
        *position = query.value(3).toLongLong();
    }
    return true;
}

QList<qint64> LocalDatabase::listMessageIds(qint64 chatId, QString *error) const
{
    QList<qint64> ids;
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT id FROM messages WHERE chat_id = ? ORDER BY position, id"));
    query.addBindValue(chatId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return ids;
    }
    while (query.next()) {
        ids.append(query.value(0).toLongLong());
    }
    return ids;
}

bool LocalDatabase::saveDraft(qint64 chatId, const QString &content, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO drafts(chat_id, content, updated_at) VALUES(?, ?, ?) "
        "ON CONFLICT(chat_id) DO UPDATE SET content = excluded.content, updated_at = excluded.updated_at"));
    query.addBindValue(chatId);
    query.addBindValue(content.isEmpty() ? QStringLiteral("") : content);
    query.addBindValue(nowMs());
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    return true;
}

bool LocalDatabase::readDraft(qint64 chatId, QString *content, QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT content FROM drafts WHERE chat_id = ?"));
    query.addBindValue(chatId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        if (content != nullptr) {
            content->clear();
        }
        return true;
    }
    if (content != nullptr) {
        *content = query.value(0).toString();
    }
    return true;
}

qint64 LocalDatabase::addToolActivity(qint64 chatId,
                                      qint64 messageId,
                                      const QString &toolCallId,
                                      const QString &toolName,
                                      const QString &argumentsJson,
                                      const QString &rawCallJson,
                                      const QString &resultText,
                                      const QString &status,
                                      const QString &errorText,
                                      qint64 position,
                                      QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "INSERT INTO tool_activities("
        " chat_id, message_id, tool_call_id, tool_name, arguments_json, raw_call_json,"
        " result_text, status, error_text, created_at, position)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(chatId);
    if (messageId > 0) {
        query.addBindValue(messageId);
    } else {
        query.addBindValue(QVariant());
    }
    query.addBindValue(toolCallId);
    query.addBindValue(toolName);
    query.addBindValue(argumentsJson);
    query.addBindValue(rawCallJson);
    query.addBindValue(resultText.isNull() ? QStringLiteral("") : resultText);
    query.addBindValue(status.isEmpty() ? QStringLiteral("unknown") : status);
    query.addBindValue(errorText.isNull() ? QStringLiteral("") : errorText);
    query.addBindValue(nowMs());
    query.addBindValue(position);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

bool LocalDatabase::updateToolActivityResult(qint64 id,
                                             const QString &resultText,
                                             const QString &status,
                                             const QString &errorText,
                                             QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "UPDATE tool_activities SET result_text = ?, status = ?, error_text = ? WHERE id = ?"));
    query.addBindValue(resultText);
    query.addBindValue(status);
    query.addBindValue(errorText);
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LocalDatabase::updateToolActivityMessageId(qint64 id, qint64 messageId, QString *error)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("UPDATE tool_activities SET message_id = ? WHERE id = ?"));
    if (messageId > 0) {
        query.addBindValue(messageId);
    } else {
        query.addBindValue(QVariant());
    }
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool LocalDatabase::readToolActivity(qint64 id,
                                     qint64 *chatId,
                                     qint64 *messageId,
                                     QString *toolCallId,
                                     QString *toolName,
                                     QString *argumentsJson,
                                     QString *rawCallJson,
                                     QString *resultText,
                                     QString *status,
                                     QString *errorText,
                                     qint64 *position,
                                     QString *error) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT chat_id, message_id, tool_call_id, tool_name, arguments_json, raw_call_json,"
        " result_text, status, error_text, position FROM tool_activities WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return false;
    }
    if (!query.next()) {
        setError(error, QStringLiteral("Tool activity not found"));
        return false;
    }
    if (chatId != nullptr) {
        *chatId = query.value(0).toLongLong();
    }
    if (messageId != nullptr) {
        *messageId = query.value(1).isNull() ? 0 : query.value(1).toLongLong();
    }
    if (toolCallId != nullptr) {
        *toolCallId = query.value(2).toString();
    }
    if (toolName != nullptr) {
        *toolName = query.value(3).toString();
    }
    if (argumentsJson != nullptr) {
        *argumentsJson = query.value(4).toString();
    }
    if (rawCallJson != nullptr) {
        *rawCallJson = query.value(5).toString();
    }
    if (resultText != nullptr) {
        *resultText = query.value(6).toString();
    }
    if (status != nullptr) {
        *status = query.value(7).toString();
    }
    if (errorText != nullptr) {
        *errorText = query.value(8).toString();
    }
    if (position != nullptr) {
        *position = query.value(9).toLongLong();
    }
    return true;
}

QList<qint64> LocalDatabase::listToolActivityIds(qint64 chatId, QString *error) const
{
    QList<qint64> ids;
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT id FROM tool_activities WHERE chat_id = ? ORDER BY position, id"));
    query.addBindValue(chatId);
    if (!query.exec()) {
        setError(error, query.lastError().text());
        return ids;
    }
    while (query.next()) {
        ids.append(query.value(0).toLongLong());
    }
    return ids;
}
