#include "appcontroller.h"
#include "pikaclawsettings.h"

#include <QDebug>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUuid>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    connect(&m_gateway, &PicoClawClient::connectionStateChanged, this, [this]() {
        Q_EMIT gatewayStateChanged();
        if (m_gateway.connectionState() != QStringLiteral("connected")) {
            m_sessionAppliedModel.clear();
        }
        if (m_isGenerating && m_gateway.connectionState() != QStringLiteral("connected")) {
            m_requestError = m_gateway.lastError().isEmpty() ? QStringLiteral("connection lost") : m_gateway.lastError();
            m_awaitingRetry = true;
            finishTurn();
        }
    });
    connect(&m_gateway, &PicoClawClient::lastErrorChanged, this, &AppController::gatewayStateChanged);
    connect(&m_gateway, &PicoClawClient::messageReceived, this, &AppController::onGatewayMessage);
}

AppController::~AppController()
{
    disconnect(&m_gateway, nullptr, this, nullptr);
    m_gateway.setAutoReconnect(false);
    m_gateway.disconnectFromGateway();
}

bool AppController::openStore(const QString &filePath, QString *error)
{
    if (!m_db.open(filePath, error)) {
        return false;
    }
    qint64 chatId = 0;
    qint64 projectId = 0;
    const bool hasChat = m_db.mostRecentlyActiveChat(&chatId, &projectId, error);
    reloadProjects();
    if (hasChat) {
        m_currentProjectId = projectId;
        m_currentChatId = chatId;
        Q_EMIT currentProjectChanged();
    }
    reloadChats();
    return true;
}

QVariantList AppController::projects() const
{
    return m_projects;
}

qint64 AppController::currentProjectId() const
{
    return m_currentProjectId;
}

QString AppController::currentProjectName() const
{
    for (const QVariant &item : m_projects) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("id")).toLongLong() == m_currentProjectId) {
            return map.value(QStringLiteral("name")).toString();
        }
    }
    return {};
}

QVariantList AppController::chats() const
{
    return m_chats;
}

qint64 AppController::currentChatId() const
{
    return m_currentChatId;
}

QString AppController::currentChatTitle() const
{
    for (const QVariant &item : m_chats) {
        const QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("id")).toLongLong() == m_currentChatId) {
            return map.value(QStringLiteral("title")).toString();
        }
    }
    return {};
}

QVariantList AppController::messages() const
{
    return m_messages;
}

QString AppController::currentWorkspace() const
{
    return m_currentWorkspace;
}

QString AppController::currentProjectWorkspace() const
{
    return m_currentProjectWorkspace;
}

bool AppController::currentChatHasWorkspaceOverride() const
{
    return m_currentChatHasWorkspaceOverride;
}

QString AppController::currentModel() const
{
    return m_currentModel;
}

QString AppController::currentProjectModel() const
{
    return m_currentProjectModel;
}

bool AppController::currentChatHasModelOverride() const
{
    return m_currentChatHasModelOverride;
}

QString AppController::currentDraft() const
{
    return m_currentDraft;
}

bool AppController::hasPendingDeletion() const
{
    return !m_pendingDeletionKind.isEmpty();
}

QString AppController::pendingDeletionKind() const
{
    return m_pendingDeletionKind;
}

QString AppController::pendingDeletionTitle() const
{
    return m_pendingDeletionTitle;
}

QString AppController::pendingDeletionMessage() const
{
    return m_pendingDeletionMessage;
}

QString AppController::gatewayState() const
{
    return m_gateway.connectionState();
}

QString AppController::gatewayError() const
{
    return m_gateway.lastError();
}

QUrl AppController::gatewayEndpoint() const
{
    return m_gateway.endpoint();
}

QVariantList AppController::availableModels() const
{
    return m_availableModels;
}

bool AppController::selectedModelUnavailable() const
{
    return m_selectedModelUnavailable;
}

bool AppController::isGenerating() const
{
    return m_isGenerating;
}

QString AppController::streamingAssistantText() const
{
    if (m_currentChatId != m_turnChatId) {
        return {};
    }
    return m_streamingAssistantText;
}

QString AppController::requestError() const
{
    return m_requestError;
}

bool AppController::canRetryOrRegenerate() const
{
    return !m_isGenerating && m_currentChatId > 0 && !m_lastUserContent.isEmpty()
        && m_gateway.connectionState() == QStringLiteral("connected");
}

void AppController::reloadProjects()
{
    m_projects.clear();
    QString error;
    const QList<qint64> ids = m_db.listProjectIds(&error);
    bool currentExists = false;
    for (qint64 id : ids) {
        QString name;
        QString workspace;
        QString model;
        if (!m_db.readProject(id, &name, &workspace, &model, &error)) {
            qWarning() << error;
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), id);
        row.insert(QStringLiteral("name"), name);
        m_projects.append(row);
        if (id == m_currentProjectId) {
            currentExists = true;
        }
    }
    if (!currentExists) {
        m_currentProjectId = ids.isEmpty() ? 0 : ids.first();
    }
    Q_EMIT projectsChanged();
    Q_EMIT currentProjectChanged();
}

void AppController::reloadChats()
{
    m_chats.clear();
    QString error;
    QList<qint64> ids;
    if (m_currentProjectId > 0) {
        ids = m_db.listChatIds(m_currentProjectId, false, &error);
    }
    bool currentExists = false;
    for (qint64 id : ids) {
        qint64 projectId = 0;
        QString title;
        if (!m_db.readChat(id, &projectId, &title, &error)) {
            qWarning() << error;
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), id);
        row.insert(QStringLiteral("title"), title);
        m_chats.append(row);
        if (id == m_currentChatId) {
            currentExists = true;
        }
    }
    if (!currentExists) {
        m_currentChatId = ids.isEmpty() ? 0 : ids.first();
        if (m_currentChatId > 0) {
            m_db.touchChat(m_currentChatId, &error);
        }
    }
    Q_EMIT chatsChanged();
    Q_EMIT currentChatChanged();
    reloadMessages();
}

void AppController::reloadMessages()
{
    m_messages.clear();
    QString error;
    QList<qint64> ids;
    if (m_currentChatId > 0) {
        ids = m_db.listMessageIds(m_currentChatId, &error);
    }
    for (qint64 id : ids) {
        qint64 chatId = 0;
        QString role;
        QString content;
        qint64 position = 0;
        if (!m_db.readMessage(id, &chatId, &role, &content, &position, &error)) {
            qWarning() << error;
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), id);
        row.insert(QStringLiteral("role"), role);
        row.insert(QStringLiteral("content"), content);
        row.insert(QStringLiteral("position"), QVariant::fromValue(position));
        m_messages.append(row);
    }
    m_lastUserContent.clear();
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        const QVariantMap row = m_messages.at(i).toMap();
        if (row.value(QStringLiteral("role")).toString() == QStringLiteral("user")) {
            m_lastUserContent = row.value(QStringLiteral("content")).toString();
            break;
        }
    }
    Q_EMIT messagesChanged();
    reloadWorkspace();
}

void AppController::reloadWorkspace()
{
    m_currentWorkspace.clear();
    m_currentProjectWorkspace.clear();
    m_currentChatHasWorkspaceOverride = false;
    m_currentModel.clear();
    m_currentProjectModel.clear();
    m_currentChatHasModelOverride = false;
    QString error;
    if (m_currentProjectId > 0) {
        QString name;
        QString workspace;
        QString model;
        if (m_db.readProject(m_currentProjectId, &name, &workspace, &model, &error)) {
            m_currentProjectWorkspace = workspace;
            m_currentWorkspace = workspace;
            m_currentProjectModel = model;
            m_currentModel = model;
        }
    }
    if (m_currentChatId > 0) {
        bool hasOverride = false;
        QString override;
        if (m_db.readChatWorkspaceOverride(m_currentChatId, &hasOverride, &override, &error) && hasOverride) {
            m_currentChatHasWorkspaceOverride = true;
            m_currentWorkspace = override;
        }
        if (m_db.readChatModelOverride(m_currentChatId, &hasOverride, &override, &error) && hasOverride) {
            m_currentChatHasModelOverride = true;
            m_currentModel = override;
        }
    }
    Q_EMIT currentWorkspaceChanged();
    Q_EMIT currentModelChanged();
    refreshModelAvailability();
    reloadDraft();
}

void AppController::reloadDraft()
{
    m_currentDraft.clear();
    if (m_currentChatId > 0) {
        QString error;
        QString draft;
        if (m_db.readDraft(m_currentChatId, &draft, &error)) {
            m_currentDraft = draft;
        }
    }
    Q_EMIT currentDraftChanged();
}

bool AppController::createProject(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    QString error;
    const qint64 id = m_db.createProject(trimmed, QStringLiteral(""), QStringLiteral(""), &error);
    if (id <= 0) {
        qWarning() << error;
        return false;
    }
    m_currentProjectId = id;
    m_currentChatId = 0;
    reloadProjects();
    reloadChats();
    return true;
}

bool AppController::renameCurrentProject(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (m_currentProjectId <= 0 || trimmed.isEmpty()) {
        return false;
    }
    QString error;
    if (!m_db.renameProject(m_currentProjectId, trimmed, &error)) {
        qWarning() << error;
        return false;
    }
    reloadProjects();
    return true;
}

bool AppController::deleteCurrentProject()
{
    if (m_currentProjectId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.deleteProject(m_currentProjectId, &error)) {
        qWarning() << error;
        return false;
    }
    m_currentProjectId = 0;
    m_currentChatId = 0;
    reloadProjects();
    reloadChats();
    return true;
}

void AppController::selectProject(qint64 id)
{
    if (id == m_currentProjectId) {
        return;
    }
    m_currentProjectId = id;
    m_currentChatId = 0;
    Q_EMIT currentProjectChanged();
    reloadChats();
}

bool AppController::createChat(const QString &title)
{
    const QString trimmed = title.trimmed();
    if (m_currentProjectId <= 0 || trimmed.isEmpty()) {
        return false;
    }
    QString error;
    const qint64 id = m_db.createChat(m_currentProjectId, trimmed, &error);
    if (id <= 0) {
        qWarning() << error;
        return false;
    }
    m_currentChatId = id;
    m_db.touchChat(id, &error);
    reloadChats();
    syncGatewaySession();
    return true;
}

bool AppController::renameCurrentChat(const QString &title)
{
    const QString trimmed = title.trimmed();
    if (m_currentChatId <= 0 || trimmed.isEmpty()) {
        return false;
    }
    QString error;
    if (!m_db.renameChat(m_currentChatId, trimmed, &error)) {
        qWarning() << error;
        return false;
    }
    reloadChats();
    return true;
}

bool AppController::archiveCurrentChat()
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.archiveChat(m_currentChatId, &error)) {
        qWarning() << error;
        return false;
    }
    m_currentChatId = 0;
    reloadChats();
    return true;
}

bool AppController::deleteCurrentChat()
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.deleteChat(m_currentChatId, &error)) {
        qWarning() << error;
        return false;
    }
    m_currentChatId = 0;
    reloadChats();
    return true;
}

void AppController::selectChat(qint64 id)
{
    if (id == m_currentChatId) {
        return;
    }
    m_currentChatId = id;
    QString error;
    m_db.touchChat(id, &error);
    reloadChats();
    syncGatewaySession();
}

bool AppController::addUserMessage(const QString &content)
{
    return addMessage(QStringLiteral("user"), content);
}

bool AppController::addAssistantMessage(const QString &content)
{
    return addMessage(QStringLiteral("assistant"), content);
}

bool AppController::addMessage(const QString &role, const QString &content)
{
    if (m_currentChatId <= 0 || content.isEmpty()) {
        return false;
    }
    QString error;
    const qint64 id = m_db.addMessage(m_currentChatId, role, content, &error);
    if (id <= 0) {
        qWarning() << error;
        return false;
    }
    m_db.touchChat(m_currentChatId, &error);
    QString draftError;
    m_db.saveDraft(m_currentChatId, QString(), &draftError);
    m_currentDraft.clear();
    reloadMessages();
    Q_EMIT currentDraftChanged();
    return true;
}

bool AppController::setCurrentProjectWorkspace(const QString &workspace)
{
    if (m_currentProjectId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.setProjectDefaultWorkspace(m_currentProjectId, workspace, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::setCurrentChatWorkspaceOverride(const QString &workspace)
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.setChatWorkspaceOverride(m_currentChatId, workspace, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::clearCurrentChatWorkspaceOverride()
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.clearChatWorkspaceOverride(m_currentChatId, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::setCurrentProjectModel(const QString &model)
{
    if (m_currentProjectId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.setProjectDefaultModel(m_currentProjectId, model, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::setCurrentChatModelOverride(const QString &model)
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.setChatModelOverride(m_currentChatId, model, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::clearCurrentChatModelOverride()
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.clearChatModelOverride(m_currentChatId, &error)) {
        qWarning() << error;
        return false;
    }
    reloadWorkspace();
    return true;
}

bool AppController::setCurrentDraft(const QString &content)
{
    if (m_currentChatId <= 0) {
        return false;
    }
    QString error;
    if (!m_db.saveDraft(m_currentChatId, content, &error)) {
        qWarning() << error;
        return false;
    }
    m_currentDraft = content;
    Q_EMIT currentDraftChanged();
    return true;
}

void AppController::clearPendingDeletion()
{
    if (m_pendingDeletionKind.isEmpty()) {
        return;
    }
    m_pendingDeletionKind.clear();
    m_pendingDeletionTitle.clear();
    m_pendingDeletionMessage.clear();
    Q_EMIT pendingDeletionChanged();
}

bool AppController::requestDeleteCurrentChat()
{
    if (m_currentChatId <= 0) {
        return false;
    }
    m_pendingDeletionKind = QStringLiteral("chat");
    m_pendingDeletionTitle = QStringLiteral("Delete chat");
    m_pendingDeletionMessage = QStringLiteral("Permanently delete chat “%1”?").arg(currentChatTitle());
    Q_EMIT pendingDeletionChanged();
    return true;
}

bool AppController::requestDeleteCurrentProject()
{
    if (m_currentProjectId <= 0) {
        return false;
    }
    m_pendingDeletionKind = QStringLiteral("project");
    m_pendingDeletionTitle = QStringLiteral("Delete project");
    m_pendingDeletionMessage = QStringLiteral(
                                     "Permanently delete project “%1”? Its chats and history will also be deleted.")
                                     .arg(currentProjectName());
    Q_EMIT pendingDeletionChanged();
    return true;
}

void AppController::cancelPendingDeletion()
{
    clearPendingDeletion();
}

bool AppController::confirmPendingDeletion()
{
    const QString kind = m_pendingDeletionKind;
    clearPendingDeletion();
    if (kind == QStringLiteral("project")) {
        return deleteCurrentProject();
    }
    if (kind == QStringLiteral("chat")) {
        return deleteCurrentChat();
    }
    return false;
}

void AppController::configureGateway(const QUrl &endpoint, const QString &token)
{
    m_baseGatewayEndpoint = endpoint;
    QUrlQuery query(m_baseGatewayEndpoint);
    query.removeAllQueryItems(QStringLiteral("session_id"));
    m_baseGatewayEndpoint.setQuery(query);
    m_gateway.setToken(token);
    m_gateway.setEndpoint(sessionAwareEndpoint());
}

void AppController::loadGatewaySettings(const QString &configDirectory)
{
    const PicoClawConnectionSettings settings = loadPicoClawConnectionSettings(configDirectory);
    configureGateway(settings.endpoint, settings.token);
    m_picoConfigPath = settings.picoConfigPath;
    m_availableModels.clear();
    const QStringList names = loadPicoClawModelNames(m_picoConfigPath);
    for (const QString &name : names) {
        m_availableModels.append(name);
    }
    Q_EMIT availableModelsChanged();
    refreshModelAvailability();
}

void AppController::refreshModelAvailability()
{
    bool unavailable = false;
    if (!m_currentModel.isEmpty() && !m_availableModels.isEmpty()) {
        unavailable = true;
        for (const QVariant &item : m_availableModels) {
            if (item.toString() == m_currentModel) {
                unavailable = false;
                break;
            }
        }
    }
    if (m_selectedModelUnavailable != unavailable) {
        m_selectedModelUnavailable = unavailable;
        Q_EMIT availableModelsChanged();
    }
}

void AppController::setGatewayAutoReconnect(bool enabled)
{
    m_gateway.setAutoReconnect(enabled);
}

void AppController::setGatewayReconnectIntervalMs(int milliseconds)
{
    m_gateway.setReconnectIntervalMs(milliseconds);
}

void AppController::connectToGateway()
{
    syncGatewaySession();
    m_gateway.connectToGateway();
}

void AppController::disconnectFromGateway()
{
    m_gateway.disconnectFromGateway();
}

bool AppController::sendChatMessage(const QString &content)
{
    const QString trimmed = content.trimmed();
    if (m_currentChatId <= 0 || trimmed.isEmpty() || m_isGenerating) {
        return false;
    }
    if (m_gateway.connectionState() != QStringLiteral("connected")) {
        m_requestError = m_gateway.lastError().isEmpty() ? QStringLiteral("gateway unavailable") : m_gateway.lastError();
        m_awaitingRetry = true;
        Q_EMIT chatTurnChanged();
        return false;
    }
    if (!addUserMessage(trimmed)) {
        return false;
    }
    beginGatewayTurn(trimmed);
    return true;
}

QString AppController::picoSessionId(qint64 chatId) const
{
    return QStringLiteral("pikatalk-chat-%1").arg(chatId);
}

QUrl AppController::sessionAwareEndpoint() const
{
    QUrl url = m_baseGatewayEndpoint.isEmpty() ? m_gateway.endpoint() : m_baseGatewayEndpoint;
    if (m_currentChatId <= 0) {
        return url;
    }
    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("session_id"));
    query.addQueryItem(QStringLiteral("session_id"), picoSessionId(m_currentChatId));
    url.setQuery(query);
    return url;
}

void AppController::syncGatewaySession()
{
    const QUrl next = sessionAwareEndpoint();
    if (next == m_gateway.endpoint()) {
        return;
    }
    m_gateway.setEndpoint(next);
    if (m_gateway.connectionState() == QStringLiteral("connected")
        || m_gateway.connectionState() == QStringLiteral("connecting")) {
        m_gateway.connectToGateway();
    }
}

bool AppController::sessionNeedsModelSwitch() const
{
    if (m_currentModel.isEmpty()) {
        return false;
    }
    const QString sessionId = picoSessionId(m_turnChatId > 0 ? m_turnChatId : m_currentChatId);
    const QString applied = m_sessionAppliedModel.value(sessionId);
    if (applied == m_currentModel) {
        return false;
    }
    if (applied.isEmpty()) {
        const QString gatewayDefault = loadPicoClawDefaultModelName(m_picoConfigPath);
        if (!gatewayDefault.isEmpty() && gatewayDefault == m_currentModel) {
            return false;
        }
    }
    return true;
}

void AppController::beginGatewayTurn(const QString &userContent)
{
    m_lastUserContent = userContent;
    m_turnChatId = m_currentChatId;
    m_persistedAssistantId = 0;
    m_streamingAssistantText.clear();
    m_streamingPicoMessageId.clear();
    m_requestError.clear();
    m_awaitingRetry = false;
    m_isGenerating = true;
    Q_EMIT chatTurnChanged();
    if (sessionNeedsModelSwitch()) {
        m_modelSwitchPending = true;
        m_pendingUserContent = userContent;
        sendSelectedModelSwitch();
        return;
    }
    m_modelSwitchPending = false;
    m_pendingUserContent.clear();
    sendPicoUserContent(userContent);
}

void AppController::sendPicoUserContent(const QString &content)
{
    const QString sessionId = picoSessionId(m_turnChatId > 0 ? m_turnChatId : m_currentChatId);
    QJsonObject payload;
    payload.insert(QStringLiteral("content"), content);
    payload.insert(QStringLiteral("model_name"), m_currentModel);
    payload.insert(QStringLiteral("workspace"), m_currentWorkspace);
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.send"));
    message.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    message.insert(QStringLiteral("session_id"), sessionId);
    message.insert(QStringLiteral("payload"), payload);
    m_gateway.sendJson(message);
}

void AppController::sendSelectedModelSwitch()
{
    if (m_currentModel.isEmpty()
        || m_gateway.connectionState() != QStringLiteral("connected")) {
        return;
    }
    const qint64 chatId = m_currentChatId > 0 ? m_currentChatId : m_turnChatId;
    if (chatId <= 0) {
        return;
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("content"), QStringLiteral("/switch model to %1").arg(m_currentModel));
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.send"));
    message.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    message.insert(QStringLiteral("session_id"), picoSessionId(chatId));
    message.insert(QStringLiteral("payload"), payload);
    m_gateway.sendJson(message);
}

bool AppController::stopGeneration()
{
    if (!m_isGenerating) {
        return false;
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("content"), QStringLiteral("/stop"));
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.send"));
    message.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    message.insert(QStringLiteral("session_id"), picoSessionId(m_turnChatId));
    message.insert(QStringLiteral("payload"), payload);
    m_gateway.sendJson(message);
    m_streamingAssistantText.clear();
    m_streamingPicoMessageId.clear();
    finishTurn();
    return true;
}

bool AppController::retryOrRegenerate()
{
    if (!canRetryOrRegenerate()) {
        return false;
    }
    if (!m_messages.isEmpty()) {
        const QVariantMap last = m_messages.last().toMap();
        if (last.value(QStringLiteral("role")).toString() == QStringLiteral("assistant")) {
            const qint64 id = last.value(QStringLiteral("id")).toLongLong();
            QString error;
            m_db.deleteMessage(id, &error);
            reloadMessages();
        }
    }
    beginGatewayTurn(m_lastUserContent);
    return true;
}

void AppController::finishTurn()
{
    m_isGenerating = false;
    m_modelSwitchPending = false;
    m_pendingUserContent.clear();
    Q_EMIT chatTurnChanged();
}

void AppController::persistStreamingAssistant()
{
    if (m_turnChatId <= 0 || m_streamingAssistantText.isEmpty()) {
        return;
    }
    QString error;
    if (m_persistedAssistantId > 0) {
        m_db.updateMessageContent(m_persistedAssistantId, m_streamingAssistantText, &error);
    } else {
        m_persistedAssistantId = m_db.addMessage(m_turnChatId, QStringLiteral("assistant"), m_streamingAssistantText, &error);
        m_db.touchChat(m_turnChatId, &error);
    }
    if (m_currentChatId == m_turnChatId) {
        reloadMessages();
    }
}

void AppController::onGatewayMessage(const QJsonObject &object)
{
    const QString type = object.value(QStringLiteral("type")).toString();
    const QJsonObject payload = object.value(QStringLiteral("payload")).toObject();
    if (type == QStringLiteral("error")) {
        m_requestError = payload.value(QStringLiteral("message")).toString();
        if (m_requestError.isEmpty()) {
            m_requestError = payload.value(QStringLiteral("code")).toString();
        }
        if (m_requestError.isEmpty()) {
            m_requestError = QStringLiteral("gateway error");
        }
        m_awaitingRetry = true;
        m_streamingAssistantText.clear();
        finishTurn();
        return;
    }
    const QString kind = payload.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("thought") || kind == QStringLiteral("tool_calls")) {
        return;
    }
    if (type == QStringLiteral("message.create") || type == QStringLiteral("message.update")) {
        const QString content = payload.value(QStringLiteral("content")).toString();
        if (m_modelSwitchPending) {
            if (content.startsWith(QStringLiteral("Task stopped."))) {
                m_streamingAssistantText.clear();
                finishTurn();
                return;
            }
            if (content.isEmpty()) {
                return;
            }
            m_sessionAppliedModel.insert(picoSessionId(m_turnChatId), m_currentModel);
            const QString pending = m_pendingUserContent;
            m_modelSwitchPending = false;
            m_pendingUserContent.clear();
            sendPicoUserContent(pending);
            return;
        }
        if (!m_isGenerating && m_persistedAssistantId <= 0) {
            return;
        }
        if (content.startsWith(QStringLiteral("Task stopped."))) {
            m_streamingAssistantText.clear();
            finishTurn();
            return;
        }
        if (content.isEmpty()) {
            return;
        }
        m_streamingPicoMessageId = payload.value(QStringLiteral("message_id")).toString();
        m_streamingAssistantText = content;
        persistStreamingAssistant();
        finishTurn();
        return;
    }
    if (!m_isGenerating) {
        return;
    }
    if (type == QStringLiteral("typing.start")) {
        Q_EMIT chatTurnChanged();
    }
}
