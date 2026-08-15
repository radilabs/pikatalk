#include "appcontroller.h"
#include "fake_pico_server.h"
#include "pikaclawsettings.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

namespace {

QJsonObject picoAssistant(const QString &content, const QString &id)
{
    return QJsonObject{
        {QStringLiteral("type"), QStringLiteral("message.create")},
        {QStringLiteral("payload"),
         QJsonObject{{QStringLiteral("content"), content}, {QStringLiteral("message_id"), id}}}};
}

void confirmPendingModelSwitch(FakePicoServer &server)
{
    QTRY_VERIFY(server.clientMessages().size() >= 1);
    const QString content = server.clientMessages()
                                .last()
                                .value(QStringLiteral("payload"))
                                .toObject()
                                .value(QStringLiteral("content"))
                                .toString();
    QVERIFY(content.startsWith(QStringLiteral("/switch model to ")));
    const int before = server.clientMessages().size();
    server.sendJson(picoAssistant(QStringLiteral("Switched."), QStringLiteral("switch-1")));
    QTRY_COMPARE(server.clientMessages().size(), before + 1);
    QVERIFY(!server.clientMessages()
                 .last()
                 .value(QStringLiteral("payload"))
                 .toObject()
                 .value(QStringLiteral("content"))
                 .toString()
                 .startsWith(QStringLiteral("/switch model to ")));
}

}

class AppControllerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createRenameSwitchDeleteAndReopen();
    void chatsAreScopedToProjectAndSurviveRestart();
    void messagesAreIsolatedPerChatAndSurviveRestart();
    void workspaceDefaultsAndOverridesPersist();
    void modelDefaultsAndOverridesPersist();
    void draftsSurviveSwitchAndRestartWithoutCreatingMessages();
    void localWorkflowRestoresState();
    void isolationAcrossProjectsSurvivesRestart();
    void deleteConfirmationCanCancelOrConfirm();
    void gatewayFailureDoesNotAlterLocalState();
    void gatewayReconnectsWithoutAlteringLocalState();
    void loadGatewaySettingsConnectsUsingConfigFile();
    void discoveredModelsDoNotRewriteLocalSelection();
    void sendChatRequestUsesModelWorkspaceAndPersistsUser();
    void sendSkipsSwitchWhenSelectedMatchesGatewayDefault();
    void streamsAndPersistsFinalAssistant();
    void stopGenerationEndsTurnAndPreservesHistory();
    void retryRegeneratesLatestAssistant();
    void gatewayErrorPreservesDraftAndHistory();
    void liveGatewaySendIfEnabled();
};

void AppControllerTest::createRenameSwitchDeleteAndReopen()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 betaId = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Alpha")));
        QVERIFY(controller.createProject(QStringLiteral("Beta")));
        QCOMPARE(controller.projects().size(), 2);
        QCOMPARE(controller.currentProjectName(), QStringLiteral("Beta"));
        betaId = controller.currentProjectId();
        const qint64 alphaId = controller.projects().at(0).toMap().value(QStringLiteral("id")).toLongLong();
        controller.selectProject(alphaId);
        QCOMPARE(controller.currentProjectName(), QStringLiteral("Alpha"));
        QVERIFY(controller.renameCurrentProject(QStringLiteral("Alpha Renamed")));
        QCOMPARE(controller.currentProjectName(), QStringLiteral("Alpha Renamed"));
        controller.selectProject(betaId);
        QCOMPARE(controller.currentProjectName(), QStringLiteral("Beta"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(controller.projects().size(), 2);
    const QString first = controller.projects().at(0).toMap().value(QStringLiteral("name")).toString();
    const QString second = controller.projects().at(1).toMap().value(QStringLiteral("name")).toString();
    QVERIFY(first == QStringLiteral("Alpha Renamed") || second == QStringLiteral("Alpha Renamed"));
    QVERIFY(first == QStringLiteral("Beta") || second == QStringLiteral("Beta"));

    controller.selectProject(betaId);
    QVERIFY(controller.deleteCurrentProject());
    QCOMPARE(controller.projects().size(), 1);
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Alpha Renamed"));
}

void AppControllerTest::chatsAreScopedToProjectAndSurviveRestart()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 alphaId = 0;
    qint64 betaId = 0;
    qint64 alphaChatId = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Alpha")));
        alphaId = controller.currentProjectId();
        QVERIFY(controller.createChat(QStringLiteral("Alpha One")));
        alphaChatId = controller.currentChatId();
        QVERIFY(controller.createChat(QStringLiteral("Alpha Two")));
        QCOMPARE(controller.chats().size(), 2);
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha Two"));
        QVERIFY(controller.createProject(QStringLiteral("Beta")));
        betaId = controller.currentProjectId();
        QVERIFY(controller.createChat(QStringLiteral("Beta One")));
        QCOMPARE(controller.chats().size(), 1);
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Beta One"));

        controller.selectProject(alphaId);
        QCOMPARE(controller.chats().size(), 2);
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha Two"));
        QVERIFY(controller.renameCurrentChat(QStringLiteral("Alpha Two Renamed")));
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha Two Renamed"));
        QVERIFY(controller.archiveCurrentChat());
        QCOMPARE(controller.chats().size(), 1);
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha One"));
        QVERIFY(controller.createChat(QStringLiteral("Alpha Three")));
        QCOMPARE(controller.chats().size(), 2);
        QVERIFY(controller.deleteCurrentChat());
        QCOMPARE(controller.chats().size(), 1);
        QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha One"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(controller.currentProjectId(), alphaId);
    QCOMPARE(controller.currentChatId(), alphaChatId);
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Alpha One"));
    QCOMPARE(controller.chats().size(), 1);
    controller.selectProject(betaId);
    QCOMPARE(controller.chats().size(), 1);
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Beta One"));
}

void AppControllerTest::messagesAreIsolatedPerChatAndSurviveRestart()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 chatB = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("P")));
        QVERIFY(controller.createChat(QStringLiteral("Chat A")));
        QVERIFY(controller.addUserMessage(QStringLiteral("A user")));
        QVERIFY(controller.addAssistantMessage(QStringLiteral("A assistant")));
        QCOMPARE(controller.messages().size(), 2);
        QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("user"));
        QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("A user"));
        QCOMPARE(controller.messages().at(1).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));

        QVERIFY(controller.createChat(QStringLiteral("Chat B")));
        chatB = controller.currentChatId();
        QVERIFY(controller.addUserMessage(QStringLiteral("B user")));
        QCOMPARE(controller.messages().size(), 1);
        QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("B user"));

        const qint64 chatA = controller.chats().at(1).toMap().value(QStringLiteral("id")).toLongLong();
        controller.selectChat(chatA);
        QCOMPARE(controller.messages().size(), 2);
        QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("A user"));
        QVERIFY(controller.deleteCurrentChat());
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(controller.currentChatId(), chatB);
    QCOMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("B user"));
}

void AppControllerTest::workspaceDefaultsAndOverridesPersist()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 chatA = 0;
    qint64 chatB = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Project A")));
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/project-a")));
        QVERIFY(controller.createChat(QStringLiteral("Chat A")));
        chatA = controller.currentChatId();
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/project-a"));
        QVERIFY(!controller.currentChatHasWorkspaceOverride());
        QVERIFY(controller.setCurrentChatWorkspaceOverride(QStringLiteral("/tmp/chat-a")));
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/chat-a"));
        QVERIFY(controller.currentChatHasWorkspaceOverride());
        QVERIFY(controller.createChat(QStringLiteral("Chat B")));
        chatB = controller.currentChatId();
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/project-a"));
        QVERIFY(!controller.currentChatHasWorkspaceOverride());
        controller.selectChat(chatA);
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/chat-a"));
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/project-a-updated")));
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/chat-a"));
        controller.selectChat(chatB);
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/project-a-updated"));
        QVERIFY(controller.clearCurrentChatWorkspaceOverride());
        controller.selectChat(chatA);
        QVERIFY(controller.clearCurrentChatWorkspaceOverride());
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/project-a-updated"));
        QVERIFY(controller.setCurrentChatWorkspaceOverride(QStringLiteral("/tmp/chat-a")));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    controller.selectChat(chatA);
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/chat-a"));
    controller.selectChat(chatB);
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/project-a-updated"));
}

void AppControllerTest::modelDefaultsAndOverridesPersist()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 chatA = 0;
    qint64 chatB = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Project A")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("example-model")));
        QVERIFY(controller.createChat(QStringLiteral("Chat A")));
        chatA = controller.currentChatId();
        QCOMPARE(controller.currentModel(), QStringLiteral("example-model"));
        QVERIFY(!controller.currentChatHasModelOverride());
        QVERIFY(controller.setCurrentChatModelOverride(QStringLiteral("chat-model")));
        QCOMPARE(controller.currentModel(), QStringLiteral("chat-model"));
        QVERIFY(controller.currentChatHasModelOverride());
        QVERIFY(controller.createChat(QStringLiteral("Chat B")));
        chatB = controller.currentChatId();
        QCOMPARE(controller.currentModel(), QStringLiteral("example-model"));
        QVERIFY(!controller.currentChatHasModelOverride());
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("example-model-updated")));
        QCOMPARE(controller.currentModel(), QStringLiteral("example-model-updated"));
        controller.selectChat(chatA);
        QCOMPARE(controller.currentModel(), QStringLiteral("chat-model"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    controller.selectChat(chatA);
    QCOMPARE(controller.currentModel(), QStringLiteral("chat-model"));
    controller.selectChat(chatB);
    QCOMPARE(controller.currentModel(), QStringLiteral("example-model-updated"));
}

void AppControllerTest::draftsSurviveSwitchAndRestartWithoutCreatingMessages()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 chatA = 0;
    qint64 chatB = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("P")));
        QVERIFY(controller.createChat(QStringLiteral("Chat A")));
        chatA = controller.currentChatId();
        QVERIFY(controller.setCurrentDraft(QStringLiteral("draft A")));
        QCOMPARE(controller.messages().size(), 0);
        QVERIFY(controller.createChat(QStringLiteral("Chat B")));
        chatB = controller.currentChatId();
        QVERIFY(controller.setCurrentDraft(QStringLiteral("draft B")));
        QCOMPARE(controller.messages().size(), 0);
        controller.selectChat(chatA);
        QCOMPARE(controller.currentDraft(), QStringLiteral("draft A"));
        QCOMPARE(controller.messages().size(), 0);
        controller.selectChat(chatB);
        QCOMPARE(controller.currentDraft(), QStringLiteral("draft B"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    controller.selectChat(chatA);
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft A"));
    QCOMPARE(controller.messages().size(), 0);
    controller.selectChat(chatB);
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft B"));
    QVERIFY(controller.addUserMessage(QStringLiteral("sent B")));
    QCOMPARE(controller.currentDraft(), QString());
    QCOMPARE(controller.messages().size(), 1);
    controller.selectChat(chatA);
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft A"));
    QCOMPARE(controller.messages().size(), 0);
}

void AppControllerTest::localWorkflowRestoresState()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 projectA = 0;
    qint64 chatA = 0;
    qint64 chatB = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Project A")));
        projectA = controller.currentProjectId();
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/ws-a")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("model-a")));
        QVERIFY(controller.createChat(QStringLiteral("Chat A")));
        chatA = controller.currentChatId();
        QVERIFY(controller.addUserMessage(QStringLiteral("hello A")));
        QVERIFY(controller.addAssistantMessage(QStringLiteral("reply A")));
        QVERIFY(controller.createChat(QStringLiteral("Chat B")));
        chatB = controller.currentChatId();
        QVERIFY(controller.addUserMessage(QStringLiteral("hello B")));
        QVERIFY(controller.setCurrentChatWorkspaceOverride(QStringLiteral("/tmp/ws-b")));
        QVERIFY(controller.setCurrentChatModelOverride(QStringLiteral("model-b")));
        QVERIFY(controller.setCurrentDraft(QStringLiteral("draft B")));
        controller.selectChat(chatA);
        QVERIFY(controller.setCurrentDraft(QStringLiteral("draft A")));
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/ws-a"));
        QCOMPARE(controller.currentModel(), QStringLiteral("model-a"));
        controller.selectChat(chatB);
        QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/ws-b"));
        QCOMPARE(controller.currentModel(), QStringLiteral("model-b"));
        QCOMPARE(controller.currentDraft(), QStringLiteral("draft B"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(controller.currentProjectId(), projectA);
    controller.selectChat(chatA);
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Chat A"));
    QCOMPARE(controller.messages().size(), 2);
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft A"));
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/ws-a"));
    QCOMPARE(controller.currentModel(), QStringLiteral("model-a"));
    controller.selectChat(chatB);
    QCOMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft B"));
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/ws-b"));
    QCOMPARE(controller.currentModel(), QStringLiteral("model-b"));
}

void AppControllerTest::isolationAcrossProjectsSurvivesRestart()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 projectKeep = 0;
    qint64 chatKeep = 0;
    qint64 chatKeepOther = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Keep")));
        projectKeep = controller.currentProjectId();
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/keep")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("keep-model")));
        QVERIFY(controller.createChat(QStringLiteral("Keep One")));
        chatKeep = controller.currentChatId();
        QVERIFY(controller.addUserMessage(QStringLiteral("keep-1")));
        QVERIFY(controller.setCurrentDraft(QStringLiteral("keep-draft-1")));
        QVERIFY(controller.createChat(QStringLiteral("Keep Two")));
        chatKeepOther = controller.currentChatId();
        QVERIFY(controller.addUserMessage(QStringLiteral("keep-2")));
        QVERIFY(controller.setCurrentDraft(QStringLiteral("keep-draft-2")));

        QVERIFY(controller.createProject(QStringLiteral("Mutate")));
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/mutate")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("mutate-model")));
        QVERIFY(controller.createChat(QStringLiteral("Mutate One")));
        QVERIFY(controller.addUserMessage(QStringLiteral("mutate-1")));
        QVERIFY(controller.createChat(QStringLiteral("Mutate Two")));
        QVERIFY(controller.renameCurrentChat(QStringLiteral("Mutate Two Renamed")));
        QVERIFY(controller.addUserMessage(QStringLiteral("mutate-2")));
        QVERIFY(controller.archiveCurrentChat());
        QVERIFY(controller.deleteCurrentChat());
        QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/mutate-updated")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("mutate-model-updated")));
        QVERIFY(controller.renameCurrentProject(QStringLiteral("Mutate Renamed")));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    controller.selectProject(projectKeep);
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Keep"));
    QCOMPARE(controller.chats().size(), 2);
    controller.selectChat(chatKeep);
    QCOMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("keep-1"));
    QCOMPARE(controller.currentDraft(), QStringLiteral("keep-draft-1"));
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/keep"));
    QCOMPARE(controller.currentModel(), QStringLiteral("keep-model"));
    controller.selectChat(chatKeepOther);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("keep-2"));
    QCOMPARE(controller.currentDraft(), QStringLiteral("keep-draft-2"));

    const qint64 mutateId = controller.projects().at(0).toMap().value(QStringLiteral("id")).toLongLong() == projectKeep
            ? controller.projects().at(1).toMap().value(QStringLiteral("id")).toLongLong()
            : controller.projects().at(0).toMap().value(QStringLiteral("id")).toLongLong();
    controller.selectProject(mutateId);
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Mutate Renamed"));
    QCOMPARE(controller.chats().size(), 0);
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/mutate-updated"));
    QCOMPARE(controller.currentModel(), QStringLiteral("mutate-model-updated"));
}

void AppControllerTest::deleteConfirmationCanCancelOrConfirm()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));

    QVERIFY(controller.createProject(QStringLiteral("Keep")));
    QVERIFY(controller.createChat(QStringLiteral("Keep chat")));
    QVERIFY(controller.addUserMessage(QStringLiteral("keep history")));
    QVERIFY(controller.createProject(QStringLiteral("Remove")));
    QVERIFY(controller.createChat(QStringLiteral("Remove chat")));
    QVERIFY(controller.addUserMessage(QStringLiteral("remove history")));
    QCOMPARE(controller.projects().size(), 2);
    QCOMPARE(controller.chats().size(), 1);

    QVERIFY(controller.requestDeleteCurrentChat());
    QVERIFY(controller.hasPendingDeletion());
    QCOMPARE(controller.pendingDeletionKind(), QStringLiteral("chat"));
    QVERIFY(controller.pendingDeletionMessage().contains(QStringLiteral("Remove chat")));
    controller.cancelPendingDeletion();
    QVERIFY(!controller.hasPendingDeletion());
    QCOMPARE(controller.chats().size(), 1);
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Remove chat"));
    QCOMPARE(controller.messages().size(), 1);

    QVERIFY(controller.requestDeleteCurrentChat());
    QVERIFY(controller.confirmPendingDeletion());
    QVERIFY(!controller.hasPendingDeletion());
    QCOMPARE(controller.chats().size(), 0);

    QVERIFY(controller.createChat(QStringLiteral("Another")));
    QVERIFY(controller.archiveCurrentChat());
    QVERIFY(!controller.hasPendingDeletion());
    QCOMPARE(controller.chats().size(), 0);

    QVERIFY(controller.requestDeleteCurrentProject());
    QVERIFY(controller.hasPendingDeletion());
    QCOMPARE(controller.pendingDeletionKind(), QStringLiteral("project"));
    QVERIFY(controller.pendingDeletionMessage().contains(QStringLiteral("Remove")));
    QVERIFY(controller.pendingDeletionMessage().contains(QStringLiteral("chats")));
    QVERIFY(controller.pendingDeletionMessage().contains(QStringLiteral("history")));
    controller.cancelPendingDeletion();
    QVERIFY(!controller.hasPendingDeletion());
    QCOMPARE(controller.projects().size(), 2);
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Remove"));

    QVERIFY(controller.requestDeleteCurrentProject());
    QVERIFY(controller.confirmPendingDeletion());
    QVERIFY(!controller.hasPendingDeletion());
    QCOMPARE(controller.projects().size(), 1);
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Keep"));
    QCOMPARE(controller.chats().size(), 1);
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Keep chat"));
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("keep history"));
}

void AppControllerTest::gatewayFailureDoesNotAlterLocalState()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Keep")));
    QVERIFY(controller.createChat(QStringLiteral("Keep chat")));
    QVERIFY(controller.addUserMessage(QStringLiteral("hello")));
    QVERIFY(controller.setCurrentDraft(QStringLiteral("unsent draft")));
    QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
    QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/workspace")));

    controller.configureGateway(QUrl(QStringLiteral("ws://127.0.0.1:1/pico/ws")), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("error"));
    QVERIFY(!controller.gatewayError().isEmpty());

    QCOMPARE(controller.currentProjectName(), QStringLiteral("Keep"));
    QCOMPARE(controller.currentChatTitle(), QStringLiteral("Keep chat"));
    QCOMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.currentDraft(), QStringLiteral("unsent draft"));
    QCOMPARE(controller.currentModel(), QStringLiteral("step-3.7-flash"));
    QCOMPARE(controller.currentWorkspace(), QStringLiteral("/tmp/workspace"));
}

void AppControllerTest::gatewayReconnectsWithoutAlteringLocalState()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    const quint16 port = server.port();

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Keep")));
    QVERIFY(controller.createChat(QStringLiteral("Keep chat")));
    QVERIFY(controller.setCurrentDraft(QStringLiteral("still here")));

    controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(true);
    controller.setGatewayReconnectIntervalMs(100);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));

    server.stopListening();
    QTRY_VERIFY(controller.gatewayState() != QStringLiteral("connected"));
    QCOMPARE(controller.currentDraft(), QStringLiteral("still here"));

    server.setPreferredPort(port);
    QVERIFY(server.listen());
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Keep"));
    QCOMPARE(controller.currentDraft(), QStringLiteral("still here"));
}

void AppControllerTest::loadGatewaySettingsConnectsUsingConfigFile()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("from-config"));
    QVERIFY(server.listen());

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile file(QDir(tmp.path()).filePath(QStringLiteral("pikatalk.conf")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QStringLiteral("[picoClaw]\nendpoint=%1\ntoken=from-config\n")
                   .arg(server.wsUrl().toString())
                   .toUtf8());
    file.close();

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Keep")));
    controller.setGatewayAutoReconnect(false);
    controller.loadGatewaySettings(tmp.path());
    QCOMPARE(controller.gatewayEndpoint(), server.wsUrl());
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QCOMPARE(controller.currentProjectName(), QStringLiteral("Keep"));
}

void AppControllerTest::discoveredModelsDoNotRewriteLocalSelection()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString picoConfig = tmp.filePath(QStringLiteral("config.json"));
    QFile pico(picoConfig);
    QVERIFY(pico.open(QIODevice::WriteOnly | QIODevice::Text));
    pico.write(R"({"model_list":[{"model_name":"step-3.7-flash"},{"model_name":"glm-4.7"}]})");
    pico.close();

    QFile conf(QDir(tmp.path()).filePath(QStringLiteral("pikatalk.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    conf.write(QStringLiteral("[picoClaw]\nconfigPath=%1\ntoken=x\n").arg(picoConfig).toUtf8());
    conf.close();

    qint64 chatId = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Alpha")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
        QVERIFY(controller.createChat(QStringLiteral("Inherited")));
        QCOMPARE(controller.currentModel(), QStringLiteral("step-3.7-flash"));
        QVERIFY(!controller.currentChatHasModelOverride());
        QVERIFY(controller.createChat(QStringLiteral("Override")));
        QVERIFY(controller.setCurrentChatModelOverride(QStringLiteral("missing-model")));
        QCOMPARE(controller.currentModel(), QStringLiteral("missing-model"));
        chatId = controller.currentChatId();
        controller.loadGatewaySettings(tmp.path());
        QCOMPARE(controller.availableModels().size(), 2);
        QVERIFY(controller.selectedModelUnavailable());
        QCOMPARE(controller.currentModel(), QStringLiteral("missing-model"));
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    controller.loadGatewaySettings(tmp.path());
    controller.selectChat(chatId);
    QCOMPARE(controller.currentModel(), QStringLiteral("missing-model"));
    QVERIFY(controller.selectedModelUnavailable());
    QVERIFY(controller.currentChatHasModelOverride());
}

void AppControllerTest::sendChatRequestUsesModelWorkspaceAndPersistsUser()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Alpha")));
    QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
    QVERIFY(controller.setCurrentProjectWorkspace(QStringLiteral("/tmp/pikatalk-ws")));
    QVERIFY(controller.createChat(QStringLiteral("Chat")));
    controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));

    QVERIFY(controller.sendChatMessage(QStringLiteral("hello gateway")));
    confirmPendingModelSwitch(server);
    QTRY_COMPARE(server.clientMessages().size(), 2);
    const QJsonObject switchMsg = server.clientMessages().at(0);
    QCOMPARE(switchMsg.value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("/switch model to step-3.7-flash"));
    const QJsonObject userMsg = server.clientMessages().at(1);
    QCOMPARE(userMsg.value(QStringLiteral("type")).toString(), QStringLiteral("message.send"));
    QCOMPARE(userMsg.value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("hello gateway"));
    QCOMPARE(userMsg.value(QStringLiteral("payload")).toObject().value(QStringLiteral("model_name")).toString(),
             QStringLiteral("step-3.7-flash"));
    QCOMPARE(userMsg.value(QStringLiteral("payload")).toObject().value(QStringLiteral("workspace")).toString(),
             QStringLiteral("/tmp/pikatalk-ws"));
    QVERIFY(userMsg.value(QStringLiteral("session_id")).toString().contains(QString::number(controller.currentChatId())));
    QCOMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("hello gateway"));
    QCOMPARE(controller.currentDraft(), QString());
}

void AppControllerTest::sendSkipsSwitchWhenSelectedMatchesGatewayDefault()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString picoConfig = tmp.filePath(QStringLiteral("config.json"));
    QFile pico(picoConfig);
    QVERIFY(pico.open(QIODevice::WriteOnly | QIODevice::Text));
    pico.write(R"({"agents":{"defaults":{"model_name":"step-3.7-flash"}},"model_list":[{"model_name":"step-3.7-flash"}]})");
    pico.close();
    QFile conf(QDir(tmp.path()).filePath(QStringLiteral("pikatalk.conf")));
    QVERIFY(conf.open(QIODevice::WriteOnly | QIODevice::Text));
    conf.write(QStringLiteral("[picoClaw]\nendpoint=%1\ntoken=test-token\nconfigPath=%2\n")
                   .arg(server.wsUrl().toString(), picoConfig)
                   .toUtf8());
    conf.close();

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Alpha")));
    QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
    QVERIFY(controller.createChat(QStringLiteral("Chat")));
    controller.loadGatewaySettings(tmp.path());
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QVERIFY(controller.sendChatMessage(QStringLiteral("hello default")));
    QTRY_COMPARE(server.clientMessages().size(), 1);
    QCOMPARE(server.clientMessages().at(0).value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("hello default"));
}

void AppControllerTest::streamsAndPersistsFinalAssistant()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    qint64 chatId = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Alpha")));
        QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
        QVERIFY(controller.createChat(QStringLiteral("Chat")));
        chatId = controller.currentChatId();
        controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
        controller.setGatewayAutoReconnect(false);
        controller.connectToGateway();
        QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
        QVERIFY(controller.sendChatMessage(QStringLiteral("hello")));
        QVERIFY(controller.isGenerating());
        confirmPendingModelSwitch(server);
        server.sendJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("typing.start")}});
        server.sendJson(QJsonObject{
            {QStringLiteral("type"), QStringLiteral("message.create")},
            {QStringLiteral("payload"),
             QJsonObject{{QStringLiteral("kind"), QStringLiteral("thought")},
                         {QStringLiteral("content"), QStringLiteral("thinking")},
                         {QStringLiteral("message_id"), QStringLiteral("t1")}}}});
        server.sendJson(QJsonObject{
            {QStringLiteral("type"), QStringLiteral("message.update")},
            {QStringLiteral("payload"),
             QJsonObject{{QStringLiteral("content"), QStringLiteral("Hel")},
                         {QStringLiteral("message_id"), QStringLiteral("a1")}}}});
        QTRY_COMPARE(controller.streamingAssistantText(), QStringLiteral("Hel"));
        server.sendJson(QJsonObject{
            {QStringLiteral("type"), QStringLiteral("message.update")},
            {QStringLiteral("payload"),
             QJsonObject{{QStringLiteral("content"), QStringLiteral("Hello world")},
                         {QStringLiteral("message_id"), QStringLiteral("a1")}}}});
        QTRY_COMPARE(controller.messages().size(), 2);
        QTRY_COMPARE(controller.messages().at(1).toMap().value(QStringLiteral("content")).toString(),
                     QStringLiteral("Hello world"));
        QVERIFY(!controller.isGenerating());
        QVERIFY(controller.createChat(QStringLiteral("Other")));
        QCOMPARE(controller.messages().size(), 0);
        controller.selectChat(chatId);
        QCOMPARE(controller.messages().size(), 2);
        QCOMPARE(controller.messages().at(1).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("Hello world"));
    }
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    controller.selectChat(chatId);
    QCOMPARE(controller.messages().size(), 2);
    QCOMPARE(controller.messages().at(1).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("Hello world"));
}

void AppControllerTest::stopGenerationEndsTurnAndPreservesHistory()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Alpha")));
    QVERIFY(controller.createChat(QStringLiteral("Chat")));
    const QString firstHistory = QStringLiteral("keep me");
    QVERIFY(controller.addUserMessage(firstHistory));
    controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QVERIFY(controller.sendChatMessage(QStringLiteral("long job")));
    QVERIFY(controller.isGenerating());
    QVERIFY(controller.stopGeneration());
    QVERIFY(!controller.isGenerating());
    QTRY_VERIFY([&server]() {
        for (const QJsonObject &msg : server.clientMessages()) {
            if (msg.value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString()
                == QStringLiteral("/stop")) {
                return true;
            }
        }
        return false;
    }());
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("content")).toString(), firstHistory);
    QVERIFY(controller.sendChatMessage(QStringLiteral("after stop")));
    QCOMPARE(controller.messages().last().toMap().value(QStringLiteral("content")).toString(), QStringLiteral("after stop"));
}

void AppControllerTest::retryRegeneratesLatestAssistant()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Alpha")));
    QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
    QVERIFY(controller.createChat(QStringLiteral("Chat")));
    controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QVERIFY(controller.sendChatMessage(QStringLiteral("hello")));
    confirmPendingModelSwitch(server);
    server.sendJson(picoAssistant(QStringLiteral("first"), QStringLiteral("a1")));
    QTRY_COMPARE(controller.messages().size(), 2);
    QVERIFY(controller.retryOrRegenerate());
    QTRY_COMPARE(controller.messages().size(), 1);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    server.sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("message.create")},
        {QStringLiteral("payload"),
         QJsonObject{{QStringLiteral("content"), QStringLiteral("second")},
                     {QStringLiteral("message_id"), QStringLiteral("a2")}}}});
    QTRY_COMPARE(controller.messages().size(), 2);
    QCOMPARE(controller.messages().at(1).toMap().value(QStringLiteral("content")).toString(), QStringLiteral("second"));
}

void AppControllerTest::gatewayErrorPreservesDraftAndHistory()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Alpha")));
    QVERIFY(controller.createChat(QStringLiteral("Chat")));
    QVERIFY(controller.addUserMessage(QStringLiteral("history")));
    QVERIFY(controller.setCurrentDraft(QStringLiteral("draft text")));
    controller.configureGateway(QUrl(QStringLiteral("ws://127.0.0.1:1/pico/ws")), QStringLiteral("test-token"));
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("error"));
    QVERIFY(!controller.sendChatMessage(QStringLiteral("draft text")));
    QVERIFY(!controller.requestError().isEmpty());
    QCOMPARE(controller.currentDraft(), QStringLiteral("draft text"));
    QCOMPARE(controller.messages().size(), 1);

    controller.configureGateway(server.wsUrl(), QStringLiteral("test-token"));
    controller.connectToGateway();
    QTRY_COMPARE(controller.gatewayState(), QStringLiteral("connected"));
    QVERIFY(controller.sendChatMessage(QStringLiteral("draft text")));
    QCOMPARE(controller.currentDraft(), QString());
    QCOMPARE(controller.messages().size(), 2);
    server.sendJson(picoAssistant(QStringLiteral("ok"), QStringLiteral("a1")));
    QTRY_COMPARE(controller.messages().size(), 3);
    QCOMPARE(controller.currentDraft(), QString());
}

void AppControllerTest::liveGatewaySendIfEnabled()
{
    if (!qEnvironmentVariableIsSet("PIKATALK_LIVE_GATEWAY")) {
        QSKIP("Set PIKATALK_LIVE_GATEWAY=1 to run a real PicoClaw end-to-end send");
    }
    const PicoClawConnectionSettings settings = loadPicoClawConnectionSettings(QDir::tempPath());
    QVERIFY2(!settings.token.isEmpty(), "Pico channel token is required for live gateway test");

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QVERIFY(controller.createProject(QStringLiteral("Live")));
    QVERIFY(controller.setCurrentProjectModel(QStringLiteral("step-3.7-flash")));
    QVERIFY(controller.setCurrentProjectWorkspace(QDir::homePath() + QStringLiteral("/.picoclaw/workspace")));
    QVERIFY(controller.createChat(QStringLiteral("Live chat")));
    controller.loadGatewaySettings(tmp.path());
    controller.setGatewayAutoReconnect(false);
    controller.connectToGateway();
    QTRY_COMPARE_WITH_TIMEOUT(controller.gatewayState(), QStringLiteral("connected"), 10000);
    QVERIFY(controller.sendChatMessage(QStringLiteral("Reply with exactly the word LIVEOK and nothing else.")));
    QTRY_VERIFY_WITH_TIMEOUT(controller.messages().size() == 2, 20000);
    QCOMPARE(controller.messages().at(0).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("user"));
    QCOMPARE(controller.messages().at(1).toMap().value(QStringLiteral("role")).toString(), QStringLiteral("assistant"));
    QVERIFY(controller.messages().at(1).toMap().value(QStringLiteral("content")).toString().contains(QStringLiteral("LIVEOK")));
}

QTEST_GUILESS_MAIN(AppControllerTest)
#include "appcontroller_test.moc"
