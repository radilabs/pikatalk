#include "appcontroller.h"

#include <QTemporaryDir>
#include <QtTest>

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

QTEST_GUILESS_MAIN(AppControllerTest)
#include "appcontroller_test.moc"
