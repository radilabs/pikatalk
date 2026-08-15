#include "database.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class DatabaseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void databasePathIsUnderDataDirectory();
    void initializesSchemaAndRoundTripsRecords();
    void dataSurvivesReopen();
    void schemaVersionIsIdentifiable();
    void projectRenameAndDeleteAreIsolated();
    void chatsAreFilteredRenamedArchivedAndDeleted();
    void messagesAreOrderedAndIsolatedPerChat();
};

void DatabaseTest::databasePathIsUnderDataDirectory()
{
    const QString dataDir = QStringLiteral("/tmp/pikatalk-data");
    const QString path = LocalDatabase::databaseFilePath(dataDir);
    QVERIFY(path.startsWith(dataDir));
    QVERIFY(path.endsWith(QStringLiteral("pikatalk.sqlite")));
    QVERIFY(!path.endsWith(QStringLiteral("phase0.sqlite")));
}

void DatabaseTest::initializesSchemaAndRoundTripsRecords()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = LocalDatabase::databaseFilePath(tmp.path());
    QVERIFY(!QFileInfo::exists(dbPath));

    LocalDatabase db;
    QString error;
    QVERIFY2(db.open(dbPath, &error), qUtf8Printable(error));
    QVERIFY(QFileInfo::exists(dbPath));

    const QStringList tables = db.tableNames(&error);
    QVERIFY(tables.contains(QStringLiteral("schema_version")));
    QVERIFY(tables.contains(QStringLiteral("projects")));
    QVERIFY(tables.contains(QStringLiteral("chats")));
    QVERIFY(tables.contains(QStringLiteral("messages")));
    QVERIFY(tables.contains(QStringLiteral("drafts")));

    const qint64 projectId = db.createProject(QStringLiteral("PikaTalk"),
                                              QStringLiteral("/tmp/ws"),
                                              QStringLiteral("example-model"),
                                              &error);
    QVERIFY2(projectId > 0, qUtf8Printable(error));

    QString projectName;
    QString workspace;
    QString model;
    QVERIFY2(db.readProject(projectId, &projectName, &workspace, &model, &error), qUtf8Printable(error));
    QCOMPARE(projectName, QStringLiteral("PikaTalk"));
    QCOMPARE(workspace, QStringLiteral("/tmp/ws"));
    QCOMPARE(model, QStringLiteral("example-model"));

    const qint64 chatId = db.createChat(projectId, QStringLiteral("Getting started"), &error);
    QVERIFY2(chatId > 0, qUtf8Printable(error));
    qint64 chatProjectId = 0;
    QString chatTitle;
    QVERIFY2(db.readChat(chatId, &chatProjectId, &chatTitle, &error), qUtf8Printable(error));
    QCOMPARE(chatProjectId, projectId);
    QCOMPARE(chatTitle, QStringLiteral("Getting started"));

    const qint64 messageId = db.addMessage(chatId, QStringLiteral("user"), QStringLiteral("hello"), &error);
    QVERIFY2(messageId > 0, qUtf8Printable(error));
    qint64 messageChatId = 0;
    QString role;
    QString content;
    qint64 position = -1;
    QVERIFY2(db.readMessage(messageId, &messageChatId, &role, &content, &position, &error), qUtf8Printable(error));
    QCOMPARE(messageChatId, chatId);
    QCOMPARE(role, QStringLiteral("user"));
    QCOMPARE(content, QStringLiteral("hello"));
    QCOMPARE(position, qint64(1));

    QVERIFY2(db.saveDraft(chatId, QStringLiteral("unfinished"), &error), qUtf8Printable(error));
    QString draft;
    QVERIFY2(db.readDraft(chatId, &draft, &error), qUtf8Printable(error));
    QCOMPARE(draft, QStringLiteral("unfinished"));
}

void DatabaseTest::dataSurvivesReopen()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = LocalDatabase::databaseFilePath(tmp.path());
    QString error;
    qint64 projectId = 0;
    qint64 chatId = 0;
    qint64 messageId = 0;
    {
        LocalDatabase db;
        QVERIFY2(db.open(dbPath, &error), qUtf8Printable(error));
        projectId = db.createProject(QStringLiteral("Keep"), QStringLiteral("/ws"), QStringLiteral("m"), &error);
        QVERIFY2(projectId > 0, qUtf8Printable(error));
        chatId = db.createChat(projectId, QStringLiteral("Chat"), &error);
        QVERIFY2(chatId > 0, qUtf8Printable(error));
        messageId = db.addMessage(chatId, QStringLiteral("assistant"), QStringLiteral("hi"), &error);
        QVERIFY2(messageId > 0, qUtf8Printable(error));
        QVERIFY2(db.saveDraft(chatId, QStringLiteral("draft-a"), &error), qUtf8Printable(error));
    }

    LocalDatabase db;
    QVERIFY2(db.open(dbPath, &error), qUtf8Printable(error));
    QString name;
    QString workspace;
    QString model;
    QVERIFY2(db.readProject(projectId, &name, &workspace, &model, &error), qUtf8Printable(error));
    QCOMPARE(name, QStringLiteral("Keep"));
    qint64 chatProjectId = 0;
    QString title;
    QVERIFY2(db.readChat(chatId, &chatProjectId, &title, &error), qUtf8Printable(error));
    QCOMPARE(title, QStringLiteral("Chat"));
    qint64 messageChatId = 0;
    QString role;
    QString content;
    qint64 position = 0;
    QVERIFY2(db.readMessage(messageId, &messageChatId, &role, &content, &position, &error), qUtf8Printable(error));
    QCOMPARE(content, QStringLiteral("hi"));
    QString draft;
    QVERIFY2(db.readDraft(chatId, &draft, &error), qUtf8Printable(error));
    QCOMPARE(draft, QStringLiteral("draft-a"));
}

void DatabaseTest::schemaVersionIsIdentifiable()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalDatabase db;
    QString error;
    QVERIFY2(db.open(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));
    QCOMPARE(db.schemaVersion(&error), 1);
}

void DatabaseTest::projectRenameAndDeleteAreIsolated()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalDatabase db;
    QString error;
    QVERIFY2(db.open(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));

    const qint64 a = db.createProject(QStringLiteral("Alpha"), QStringLiteral("/a"), QStringLiteral("m-a"), &error);
    const qint64 b = db.createProject(QStringLiteral("Beta"), QStringLiteral("/b"), QStringLiteral("m-b"), &error);
    QVERIFY2(a > 0, qUtf8Printable(error));
    QVERIFY2(b > 0, qUtf8Printable(error));
    QCOMPARE(db.listProjectIds(&error).size(), 2);

    QVERIFY2(db.renameProject(a, QStringLiteral("Alpha Renamed"), &error), qUtf8Printable(error));
    QString name;
    QString workspace;
    QString model;
    QVERIFY2(db.readProject(a, &name, &workspace, &model, &error), qUtf8Printable(error));
    QCOMPARE(name, QStringLiteral("Alpha Renamed"));
    QVERIFY2(db.readProject(b, &name, &workspace, &model, &error), qUtf8Printable(error));
    QCOMPARE(name, QStringLiteral("Beta"));

    QVERIFY2(db.deleteProject(a, &error), qUtf8Printable(error));
    QCOMPARE(db.listProjectIds(&error), QList<qint64>{b});
    QVERIFY2(db.readProject(b, &name, &workspace, &model, &error), qUtf8Printable(error));
    QCOMPARE(name, QStringLiteral("Beta"));
    QCOMPARE(workspace, QStringLiteral("/b"));
}

void DatabaseTest::chatsAreFilteredRenamedArchivedAndDeleted()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalDatabase db;
    QString error;
    QVERIFY2(db.open(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));

    const qint64 alpha = db.createProject(QStringLiteral("Alpha"), QStringLiteral(""), QStringLiteral(""), &error);
    const qint64 beta = db.createProject(QStringLiteral("Beta"), QStringLiteral(""), QStringLiteral(""), &error);
    QVERIFY2(alpha > 0, qUtf8Printable(error));
    QVERIFY2(beta > 0, qUtf8Printable(error));

    const qint64 a1 = db.createChat(alpha, QStringLiteral("A1"), &error);
    QVERIFY2(a1 > 0, qUtf8Printable(error));
    QTest::qWait(2);
    const qint64 a2 = db.createChat(alpha, QStringLiteral("A2"), &error);
    QVERIFY2(a2 > 0, qUtf8Printable(error));
    const qint64 b1 = db.createChat(beta, QStringLiteral("B1"), &error);
    QVERIFY2(b1 > 0, qUtf8Printable(error));

    QCOMPARE(db.listChatIds(alpha, false, &error), QList<qint64>({a2, a1}));
    QCOMPARE(db.listChatIds(beta, false, &error), QList<qint64>({b1}));

    QVERIFY2(db.renameChat(a1, QStringLiteral("A1 Renamed"), &error), qUtf8Printable(error));
    qint64 projectId = 0;
    QString title;
    QVERIFY2(db.readChat(a1, &projectId, &title, &error), qUtf8Printable(error));
    QCOMPARE(title, QStringLiteral("A1 Renamed"));
    QVERIFY2(db.readChat(a2, &projectId, &title, &error), qUtf8Printable(error));
    QCOMPARE(title, QStringLiteral("A2"));

    QVERIFY2(db.touchChat(a1, &error), qUtf8Printable(error));
    QCOMPARE(db.listChatIds(alpha, false, &error), QList<qint64>({a1, a2}));

    QVERIFY2(db.archiveChat(a1, &error), qUtf8Printable(error));
    QCOMPARE(db.listChatIds(alpha, false, &error), QList<qint64>({a2}));
    QCOMPARE(db.listChatIds(alpha, true, &error), QList<qint64>({a1, a2}));
    QVERIFY2(db.readChat(a1, &projectId, &title, &error), qUtf8Printable(error));
    QCOMPARE(title, QStringLiteral("A1 Renamed"));

    QVERIFY2(db.deleteChat(a2, &error), qUtf8Printable(error));
    QCOMPARE(db.listChatIds(alpha, true, &error), QList<qint64>({a1}));
    QVERIFY2(db.readChat(b1, &projectId, &title, &error), qUtf8Printable(error));
    QCOMPARE(title, QStringLiteral("B1"));
    QVERIFY(!db.readChat(a2, &projectId, &title, &error));
}

void DatabaseTest::messagesAreOrderedAndIsolatedPerChat()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    LocalDatabase db;
    QString error;
    QVERIFY2(db.open(LocalDatabase::databaseFilePath(tmp.path()), &error), qUtf8Printable(error));

    const qint64 projectId = db.createProject(QStringLiteral("P"), QStringLiteral(""), QStringLiteral(""), &error);
    const qint64 chatA = db.createChat(projectId, QStringLiteral("A"), &error);
    const qint64 chatB = db.createChat(projectId, QStringLiteral("B"), &error);
    QVERIFY2(chatA > 0, qUtf8Printable(error));
    QVERIFY2(chatB > 0, qUtf8Printable(error));

    const qint64 a1 = db.addMessage(chatA, QStringLiteral("user"), QStringLiteral("A user"), &error);
    const qint64 a2 = db.addMessage(chatA, QStringLiteral("assistant"), QStringLiteral("A assistant"), &error);
    const qint64 b1 = db.addMessage(chatB, QStringLiteral("user"), QStringLiteral("B user"), &error);
    QVERIFY2(a1 > 0, qUtf8Printable(error));
    QVERIFY2(a2 > 0, qUtf8Printable(error));
    QVERIFY2(b1 > 0, qUtf8Printable(error));

    QCOMPARE(db.listMessageIds(chatA, &error), QList<qint64>({a1, a2}));
    QCOMPARE(db.listMessageIds(chatB, &error), QList<qint64>({b1}));

    qint64 messageChatId = 0;
    QString role;
    QString content;
    qint64 position = 0;
    QVERIFY2(db.readMessage(a2, &messageChatId, &role, &content, &position, &error), qUtf8Printable(error));
    QCOMPARE(role, QStringLiteral("assistant"));
    QCOMPARE(position, qint64(2));

    QVERIFY2(db.deleteChat(chatA, &error), qUtf8Printable(error));
    QCOMPARE(db.listMessageIds(chatA, &error), QList<qint64>());
    QCOMPARE(db.listMessageIds(chatB, &error), QList<qint64>({b1}));
    QVERIFY2(db.readMessage(b1, &messageChatId, &role, &content, &position, &error), qUtf8Printable(error));
    QCOMPARE(content, QStringLiteral("B user"));
}

QTEST_GUILESS_MAIN(DatabaseTest)
#include "database_test.moc"
