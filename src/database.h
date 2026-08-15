#pragma once

#include <QList>
#include <QString>
#include <QStringList>

class LocalDatabase
{
public:
    LocalDatabase();
    ~LocalDatabase();

    LocalDatabase(const LocalDatabase &) = delete;
    LocalDatabase &operator=(const LocalDatabase &) = delete;

    static QString databaseFilePath(const QString &dataDirectory);

    bool open(const QString &filePath, QString *error = nullptr);
    void close();
    bool isOpen() const;

    int schemaVersion(QString *error = nullptr) const;
    QStringList tableNames(QString *error = nullptr) const;

    qint64 createProject(const QString &name,
                         const QString &defaultWorkspace,
                         const QString &defaultModel,
                         QString *error = nullptr);
    bool readProject(qint64 id,
                     QString *name,
                     QString *defaultWorkspace,
                     QString *defaultModel,
                     QString *error = nullptr) const;
    QList<qint64> listProjectIds(QString *error = nullptr) const;
    bool renameProject(qint64 id, const QString &name, QString *error = nullptr);
    bool deleteProject(qint64 id, QString *error = nullptr);
    bool setProjectDefaultWorkspace(qint64 id, const QString &workspace, QString *error = nullptr);
    bool setProjectDefaultModel(qint64 id, const QString &model, QString *error = nullptr);

    qint64 createChat(qint64 projectId, const QString &title, QString *error = nullptr);
    bool readChat(qint64 id, qint64 *projectId, QString *title, QString *error = nullptr) const;
    QList<qint64> listChatIds(qint64 projectId, bool includeArchived, QString *error = nullptr) const;
    bool renameChat(qint64 id, const QString &title, QString *error = nullptr);
    bool touchChat(qint64 id, QString *error = nullptr);
    bool archiveChat(qint64 id, QString *error = nullptr);
    bool deleteChat(qint64 id, QString *error = nullptr);
    bool mostRecentlyActiveChat(qint64 *chatId, qint64 *projectId, QString *error = nullptr) const;
    bool readChatWorkspaceOverride(qint64 id, bool *hasOverride, QString *override, QString *error = nullptr) const;
    bool setChatWorkspaceOverride(qint64 id, const QString &workspace, QString *error = nullptr);
    bool clearChatWorkspaceOverride(qint64 id, QString *error = nullptr);
    bool readChatModelOverride(qint64 id, bool *hasOverride, QString *override, QString *error = nullptr) const;
    bool setChatModelOverride(qint64 id, const QString &model, QString *error = nullptr);
    bool clearChatModelOverride(qint64 id, QString *error = nullptr);

    qint64 addMessage(qint64 chatId, const QString &role, const QString &content, QString *error = nullptr);
    bool updateMessageContent(qint64 id, const QString &content, QString *error = nullptr);
    bool deleteMessage(qint64 id, QString *error = nullptr);
    bool readMessage(qint64 id, qint64 *chatId, QString *role, QString *content, qint64 *position, QString *error = nullptr) const;
    QList<qint64> listMessageIds(qint64 chatId, QString *error = nullptr) const;

    bool saveDraft(qint64 chatId, const QString &content, QString *error = nullptr);
    bool readDraft(qint64 chatId, QString *content, QString *error = nullptr) const;

    qint64 addToolActivity(qint64 chatId,
                           qint64 messageId,
                           const QString &toolCallId,
                           const QString &toolName,
                           const QString &argumentsJson,
                           const QString &rawCallJson,
                           const QString &resultText,
                           const QString &status,
                           const QString &errorText,
                           qint64 position,
                           QString *error = nullptr);
    bool updateToolActivityResult(qint64 id,
                                  const QString &resultText,
                                  const QString &status,
                                  const QString &errorText,
                                  QString *error = nullptr);
    bool updateToolActivityMessageId(qint64 id, qint64 messageId, QString *error = nullptr);
    bool readToolActivity(qint64 id,
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
                          QString *error = nullptr) const;
    QList<qint64> listToolActivityIds(qint64 chatId, QString *error = nullptr) const;

private:
    bool exec(const QString &sql, QString *error) const;
    qint64 nowMs() const;
    void setError(QString *error, const QString &message) const;

    QString m_connectionName;
    bool m_open = false;
};
