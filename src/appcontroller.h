#pragma once

#include "database.h"
#include "pikaclawclient.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
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
    Q_PROPERTY(bool hasPendingDeletion READ hasPendingDeletion NOTIFY pendingDeletionChanged)
    Q_PROPERTY(QString pendingDeletionKind READ pendingDeletionKind NOTIFY pendingDeletionChanged)
    Q_PROPERTY(QString pendingDeletionTitle READ pendingDeletionTitle NOTIFY pendingDeletionChanged)
    Q_PROPERTY(QString pendingDeletionMessage READ pendingDeletionMessage NOTIFY pendingDeletionChanged)
    Q_PROPERTY(QString gatewayState READ gatewayState NOTIFY gatewayStateChanged)
    Q_PROPERTY(QString gatewayError READ gatewayError NOTIFY gatewayStateChanged)
    Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(bool selectedModelUnavailable READ selectedModelUnavailable NOTIFY availableModelsChanged)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY chatTurnChanged)
    Q_PROPERTY(QString streamingAssistantText READ streamingAssistantText NOTIFY chatTurnChanged)
    Q_PROPERTY(QString requestError READ requestError NOTIFY chatTurnChanged)
    Q_PROPERTY(bool canRetryOrRegenerate READ canRetryOrRegenerate NOTIFY chatTurnChanged)
    Q_PROPERTY(QVariantList toolActivities READ toolActivities NOTIFY toolActivitiesChanged)
    Q_PROPERTY(QString lastCopiedText READ lastCopiedText NOTIFY lastCopiedTextChanged)
    Q_PROPERTY(QString workspaceActionError READ workspaceActionError NOTIFY workspaceActionErrorChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

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
    bool hasPendingDeletion() const;
    QString pendingDeletionKind() const;
    QString pendingDeletionTitle() const;
    QString pendingDeletionMessage() const;
    QString gatewayState() const;
    QString gatewayError() const;
    QUrl gatewayEndpoint() const;
    QVariantList availableModels() const;
    bool selectedModelUnavailable() const;
    bool isGenerating() const;
    QString streamingAssistantText() const;
    QString requestError() const;
    bool canRetryOrRegenerate() const;
    QVariantList toolActivities() const;
    QString lastCopiedText() const;
    QString workspaceActionError() const;

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
    Q_INVOKABLE bool requestDeleteCurrentProject();
    Q_INVOKABLE bool requestDeleteCurrentChat();
    Q_INVOKABLE void cancelPendingDeletion();
    Q_INVOKABLE bool confirmPendingDeletion();
    Q_INVOKABLE void configureGateway(const QUrl &endpoint, const QString &token);
    Q_INVOKABLE void loadGatewaySettings(const QString &configDirectory);
    Q_INVOKABLE void setGatewayAutoReconnect(bool enabled);
    Q_INVOKABLE void setGatewayReconnectIntervalMs(int milliseconds);
    Q_INVOKABLE void connectToGateway();
    Q_INVOKABLE void disconnectFromGateway();
    Q_INVOKABLE bool sendChatMessage(const QString &content);
    Q_INVOKABLE bool stopGeneration();
    Q_INVOKABLE bool retryOrRegenerate();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QVariantList messageSegments(const QString &content) const;
    Q_INVOKABLE bool openWorkspaceInFileManager();
    Q_INVOKABLE bool openWorkspaceInTerminal();
    Q_INVOKABLE bool openWorkspaceInEditor();

Q_SIGNALS:
    void projectsChanged();
    void currentProjectChanged();
    void chatsChanged();
    void currentChatChanged();
    void messagesChanged();
    void currentWorkspaceChanged();
    void currentModelChanged();
    void currentDraftChanged();
    void pendingDeletionChanged();
    void gatewayStateChanged();
    void availableModelsChanged();
    void chatTurnChanged();
    void toolActivitiesChanged();
    void lastCopiedTextChanged();
    void workspaceActionErrorChanged();

private:
    void reloadProjects();
    void reloadChats();
    void reloadMessages();
    void reloadWorkspace();
    void reloadDraft();
    void reloadToolActivities();
    void refreshModelAvailability();
    bool addMessage(const QString &role, const QString &content);
    void clearPendingDeletion();
    void onGatewayMessage(const QJsonObject &object);
    void finishTurn();
    void persistStreamingAssistant();
    void persistToolCalls(const QJsonObject &payload);
    void refreshPendingToolResults();
    QString classifyToolResultStatus(const QString &resultText) const;
    QString picoSessionId(qint64 chatId) const;
    QUrl sessionAwareEndpoint() const;
    void syncGatewaySession();
    void beginGatewayTurn(const QString &userContent);
    void sendPicoUserContent(const QString &content);
    void sendSelectedModelSwitch();
    bool sessionNeedsModelSwitch() const;

    LocalDatabase m_db;
    QVariantList m_projects;
    QVariantList m_chats;
    QVariantList m_messages;
    QVariantList m_toolActivities;
    qint64 m_currentProjectId = 0;
    qint64 m_currentChatId = 0;
    QString m_currentWorkspace;
    QString m_currentProjectWorkspace;
    bool m_currentChatHasWorkspaceOverride = false;
    QString m_currentModel;
    QString m_currentProjectModel;
    bool m_currentChatHasModelOverride = false;
    QString m_currentDraft;
    QString m_pendingDeletionKind;
    QString m_pendingDeletionTitle;
    QString m_pendingDeletionMessage;
    PicoClawClient m_gateway;
    QUrl m_baseGatewayEndpoint;
    QVariantList m_availableModels;
    QString m_picoConfigPath;
    bool m_selectedModelUnavailable = false;
    bool m_isGenerating = false;
    QString m_streamingAssistantText;
    QString m_streamingPicoMessageId;
    QString m_requestError;
    QString m_lastUserContent;
    qint64 m_turnChatId = 0;
    qint64 m_persistedAssistantId = 0;
    bool m_awaitingRetry = false;
    bool m_modelSwitchPending = false;
    QString m_pendingUserContent;
    QHash<QString, QString> m_sessionAppliedModel;
    QList<qint64> m_pendingToolActivityIds;
    qint64 m_nextToolPosition = 1;
    QString m_lastCopiedText;
    QString m_workspaceActionError;
};
