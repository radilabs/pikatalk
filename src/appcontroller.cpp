#include "appcontroller.h"

#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
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
    reloadMessages();
    QString draftError;
    m_db.saveDraft(m_currentChatId, QString(), &draftError);
    m_currentDraft.clear();
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
