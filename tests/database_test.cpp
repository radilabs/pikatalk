#include "database.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
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
    void migratesSchemaV1ToV2AndPersistsToolActivity();
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
    QVERIFY(tables.contains(QStringLiteral("tool_activities")));

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
    QCOMPARE(db.schemaVersion(&error), 2);
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

void DatabaseTest::migratesSchemaV1ToV2AndPersistsToolActivity()
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
        // Force a v1 database by rewriting schema_version after creating v1-shaped content via open.
        // Fresh opens create current schema; simulate v1 by creating DB then verifying migration path
        // with an intentionally opened v1 file constructed below.
    }
    QFile::remove(dbPath);

    {
        QSqlDatabase bootstrap = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("bootstrap-v1"));
        bootstrap.setDatabaseName(dbPath);
        QVERIFY(bootstrap.open());
        QSqlQuery q(bootstrap);
        QVERIFY(q.exec(QStringLiteral("PRAGMA foreign_keys = ON")));
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE schema_version (version INTEGER PRIMARY KEY NOT NULL)")));
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE projects ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " name TEXT NOT NULL,"
            " default_workspace TEXT NOT NULL DEFAULT '',"
            " default_model TEXT NOT NULL DEFAULT '',"
            " created_at INTEGER NOT NULL,"
            " sort_order INTEGER NOT NULL DEFAULT 0)")));
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE chats ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " project_id INTEGER NOT NULL REFERENCES projects(id) ON DELETE CASCADE,"
            " title TEXT NOT NULL,"
            " workspace_override TEXT,"
            " model_override TEXT,"
            " archived INTEGER NOT NULL DEFAULT 0,"
            " created_at INTEGER NOT NULL,"
            " last_active_at INTEGER NOT NULL,"
            " sort_order INTEGER NOT NULL DEFAULT 0)")));
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE messages ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " chat_id INTEGER NOT NULL REFERENCES chats(id) ON DELETE CASCADE,"
            " role TEXT NOT NULL CHECK (role IN ('user', 'assistant')),"
            " content TEXT NOT NULL,"
            " created_at INTEGER NOT NULL,"
            " position INTEGER NOT NULL)")));
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE drafts ("
            " chat_id INTEGER PRIMARY KEY REFERENCES chats(id) ON DELETE CASCADE,"
            " content TEXT NOT NULL DEFAULT '',"
            " updated_at INTEGER NOT NULL)")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO schema_version(version) VALUES (1)")));
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO projects(name, default_workspace, default_model, created_at, sort_order) "
            "VALUES ('Keep', '/ws', 'm', 1, 0)")));
        projectId = q.lastInsertId().toLongLong();
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO chats(project_id, title, archived, created_at, last_active_at, sort_order) "
            "VALUES (%1, 'Chat', 0, 1, 1, 0)").arg(projectId)));
        chatId = q.lastInsertId().toLongLong();
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO messages(chat_id, role, content, created_at, position) "
            "VALUES (%1, 'assistant', 'hi', 1, 1)").arg(chatId)));
        messageId = q.lastInsertId().toLongLong();
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO drafts(chat_id, content, updated_at) VALUES (%1, 'draft-a', 1)").arg(chatId)));
        bootstrap.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("bootstrap-v1"));

    qint64 toolOkId = 0;
    qint64 toolErrId = 0;
    {
        LocalDatabase db;
        QVERIFY2(db.open(dbPath, &error), qUtf8Printable(error));
        QCOMPARE(db.schemaVersion(&error), 2);
        QVERIFY(db.tableNames(&error).contains(QStringLiteral("tool_activities")));

        QString name;
        QString workspace;
        QString model;
        QVERIFY2(db.readProject(projectId, &name, &workspace, &model, &error), qUtf8Printable(error));
        QCOMPARE(name, QStringLiteral("Keep"));
        QString draft;
        QVERIFY2(db.readDraft(chatId, &draft, &error), qUtf8Printable(error));
        QCOMPARE(draft, QStringLiteral("draft-a"));

        toolOkId = db.addToolActivity(chatId,
                                      messageId,
                                      QStringLiteral("chatcmpl-tool-ok"),
                                      QStringLiteral("list_dir"),
                                      QStringLiteral("{\"path\":\".\"}"),
                                      QStringLiteral("{\"id\":\"chatcmpl-tool-ok\"}"),
                                      QStringLiteral("FILE: AGENT.md"),
                                      QStringLiteral("ok"),
                                      QString(),
                                      1,
                                      &error);
        QVERIFY2(toolOkId > 0, qUtf8Printable(error));
        toolErrId = db.addToolActivity(chatId,
                                       0,
                                       QStringLiteral("chatcmpl-tool-err"),
                                       QStringLiteral("list_dir"),
                                       QStringLiteral("{\"path\":\"/home\"}"),
                                       QStringLiteral("{\"id\":\"chatcmpl-tool-err\"}"),
                                       QStringLiteral("failed to read directory: path escapes workspace"),
                                       QStringLiteral("error"),
                                       QStringLiteral("failed to read directory: path escapes workspace"),
                                       2,
                                       &error);
        QVERIFY2(toolErrId > 0, qUtf8Printable(error));
    }

    {
        LocalDatabase db;
        QVERIFY2(db.open(dbPath, &error), qUtf8Printable(error));
        const QList<qint64> ids = db.listToolActivityIds(chatId, &error);
        QCOMPARE(ids, QList<qint64>({toolOkId, toolErrId}));
        qint64 outChat = 0;
        qint64 outMessage = 0;
        QString callId;
        QString name;
        QString args;
        QString rawCall;
        QString result;
        QString status;
        QString errText;
        qint64 position = 0;
        QVERIFY2(db.readToolActivity(toolOkId,
                                     &outChat,
                                     &outMessage,
                                     &callId,
                                     &name,
                                     &args,
                                     &rawCall,
                                     &result,
                                     &status,
                                     &errText,
                                     &position,
                                     &error),
                 qUtf8Printable(error));
        QCOMPARE(outChat, chatId);
        QCOMPARE(outMessage, messageId);
        QCOMPARE(callId, QStringLiteral("chatcmpl-tool-ok"));
        QCOMPARE(name, QStringLiteral("list_dir"));
        QCOMPARE(args, QStringLiteral("{\"path\":\".\"}"));
        QCOMPARE(result, QStringLiteral("FILE: AGENT.md"));
        QCOMPARE(status, QStringLiteral("ok"));
        QCOMPARE(position, qint64(1));

        QVERIFY2(db.readToolActivity(toolErrId,
                                     &outChat,
                                     &outMessage,
                                     &callId,
                                     &name,
                                     &args,
                                     &rawCall,
                                     &result,
                                     &status,
                                     &errText,
                                     &position,
                                     &error),
                 qUtf8Printable(error));
        QCOMPARE(status, QStringLiteral("error"));
        QCOMPARE(errText, QStringLiteral("failed to read directory: path escapes workspace"));
        QCOMPARE(result, QStringLiteral("failed to read directory: path escapes workspace"));

        QVERIFY2(db.deleteChat(chatId, &error), qUtf8Printable(error));
        QCOMPARE(db.listToolActivityIds(chatId, &error), QList<qint64>());
    }
}

QTEST_GUILESS_MAIN(DatabaseTest)
#include "database_test.moc"
