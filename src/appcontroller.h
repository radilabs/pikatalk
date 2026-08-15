#pragma once

#include "database.h"

#include <QObject>
#include <QVariant>
#include <QVariantList>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList projects READ projects NOTIFY projectsChanged)
    Q_PROPERTY(qint64 currentProjectId READ currentProjectId NOTIFY currentProjectChanged)
    Q_PROPERTY(QString currentProjectName READ currentProjectName NOTIFY currentProjectChanged)
    Q_PROPERTY(QVariantList chats READ chats NOTIFY chatsChanged)
    Q_PROPERTY(qint64 currentChatId READ currentChatId NOTIFY currentChatChanged)
    Q_PROPERTY(QString currentChatTitle READ currentChatTitle NOTIFY currentChatChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QString currentWorkspace READ currentWorkspace NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(QString currentProjectWorkspace READ currentProjectWorkspace NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(bool currentChatHasWorkspaceOverride READ currentChatHasWorkspaceOverride NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(QString currentModel READ currentModel NOTIFY currentModelChanged)
    Q_PROPERTY(QString currentProjectModel READ currentProjectModel NOTIFY currentModelChanged)
    Q_PROPERTY(bool currentChatHasModelOverride READ currentChatHasModelOverride NOTIFY currentModelChanged)
    Q_PROPERTY(QString currentDraft READ currentDraft WRITE setCurrentDraft NOTIFY currentDraftChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    bool openStore(const QString &filePath, QString *error = nullptr);

    QVariantList projects() const;
    qint64 currentProjectId() const;
    QString currentProjectName() const;
    QVariantList chats() const;
    qint64 currentChatId() const;
    QString currentChatTitle() const;
    QVariantList messages() const;
    QString currentWorkspace() const;
    QString currentProjectWorkspace() const;
    bool currentChatHasWorkspaceOverride() const;
    QString currentModel() const;
    QString currentProjectModel() const;
    bool currentChatHasModelOverride() const;
    QString currentDraft() const;

    Q_INVOKABLE bool createProject(const QString &name);
    Q_INVOKABLE bool renameCurrentProject(const QString &name);
    Q_INVOKABLE bool deleteCurrentProject();
    Q_INVOKABLE void selectProject(qint64 id);

    Q_INVOKABLE bool createChat(const QString &title);
    Q_INVOKABLE bool renameCurrentChat(const QString &title);
    Q_INVOKABLE bool archiveCurrentChat();
    Q_INVOKABLE bool deleteCurrentChat();
    Q_INVOKABLE void selectChat(qint64 id);
    Q_INVOKABLE bool addUserMessage(const QString &content);
    Q_INVOKABLE bool addAssistantMessage(const QString &content);
    Q_INVOKABLE bool setCurrentProjectWorkspace(const QString &workspace);
    Q_INVOKABLE bool setCurrentChatWorkspaceOverride(const QString &workspace);
    Q_INVOKABLE bool clearCurrentChatWorkspaceOverride();
    Q_INVOKABLE bool setCurrentProjectModel(const QString &model);
    Q_INVOKABLE bool setCurrentChatModelOverride(const QString &model);
    Q_INVOKABLE bool clearCurrentChatModelOverride();
    Q_INVOKABLE bool setCurrentDraft(const QString &content);

Q_SIGNALS:
    void projectsChanged();
    void currentProjectChanged();
    void chatsChanged();
    void currentChatChanged();
    void messagesChanged();
    void currentWorkspaceChanged();
    void currentModelChanged();
    void currentDraftChanged();

private:
    void reloadProjects();
    void reloadChats();
    void reloadMessages();
    void reloadWorkspace();
    void reloadDraft();
    bool addMessage(const QString &role, const QString &content);

    LocalDatabase m_db;
    QVariantList m_projects;
    QVariantList m_chats;
    QVariantList m_messages;
    qint64 m_currentProjectId = 0;
    qint64 m_currentChatId = 0;
    QString m_currentWorkspace;
    QString m_currentProjectWorkspace;
    bool m_currentChatHasWorkspaceOverride = false;
    QString m_currentModel;
    QString m_currentProjectModel;
    bool m_currentChatHasModelOverride = false;
    QString m_currentDraft;
};
