#include "titlefilter.h"
#include "appcontroller.h"
#include "database.h"

#include <QTemporaryDir>
#include <QVariantMap>
#include <QtTest>

namespace {

QVariantMap projectRow(qint64 id, const QString &name, const QString &body = QString())
{
    QVariantMap row;
    row.insert(QStringLiteral("id"), id);
    row.insert(QStringLiteral("name"), name);
    if (!body.isEmpty()) {
        row.insert(QStringLiteral("body"), body);
    }
    return row;
}

QVariantMap chatRow(qint64 id, const QString &title, const QString &body = QString())
{
    QVariantMap row;
    row.insert(QStringLiteral("id"), id);
    row.insert(QStringLiteral("title"), title);
    if (!body.isEmpty()) {
        row.insert(QStringLiteral("body"), body);
    }
    return row;
}

QStringList namesOf(const QVariantList &items, const QString &key)
{
    QStringList names;
    for (const QVariant &item : items) {
        names.append(item.toMap().value(key).toString());
    }
    return names;
}

}

class TitleFilterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyFilterKeepsAllItems();
    void projectNameMatchIsCaseInsensitive();
    void chatTitleMatchIsCaseInsensitive();
    void nonMatchingTitlesAreOmitted();
    void clearingFilterRestoresFullList();
    void messageBodiesAreNotSearched();
    void filterDoesNotMutateSourceList();
    void persistenceAndArchiveAreUnchangedByFiltering();
};

void TitleFilterTest::emptyFilterKeepsAllItems()
{
    const QVariantList projects = {
        projectRow(1, QStringLiteral("Alpha")),
        projectRow(2, QStringLiteral("Beta")),
    };

    QCOMPARE(filterItemsByTitle(projects, QStringLiteral("name"), QString()).size(), 2);
    QCOMPARE(filterItemsByTitle(projects, QStringLiteral("name"), QStringLiteral("   ")).size(), 2);
    QVERIFY(titleMatches(QStringLiteral("Alpha"), QString()));
}

void TitleFilterTest::projectNameMatchIsCaseInsensitive()
{
    const QVariantList projects = {
        projectRow(1, QStringLiteral("Kitchen Reno")),
        projectRow(2, QStringLiteral("Garden")),
        projectRow(3, QStringLiteral("Work Notes")),
    };

    const QVariantList filtered = filterItemsByTitle(projects, QStringLiteral("name"), QStringLiteral("kItChEn"));
    QCOMPARE(namesOf(filtered, QStringLiteral("name")), QStringList{QStringLiteral("Kitchen Reno")});
}

void TitleFilterTest::chatTitleMatchIsCaseInsensitive()
{
    const QVariantList chats = {
        chatRow(1, QStringLiteral("Weekly standup")),
        chatRow(2, QStringLiteral("Shopping list")),
        chatRow(3, QStringLiteral("Vacation plan")),
    };

    const QVariantList filtered = filterItemsByTitle(chats, QStringLiteral("title"), QStringLiteral("SHOP"));
    QCOMPARE(namesOf(filtered, QStringLiteral("title")), QStringList{QStringLiteral("Shopping list")});
}

void TitleFilterTest::nonMatchingTitlesAreOmitted()
{
    const QVariantList projects = {
        projectRow(1, QStringLiteral("Alpha")),
        projectRow(2, QStringLiteral("Beta")),
        projectRow(3, QStringLiteral("Gamma")),
    };

    const QVariantList filtered = filterItemsByTitle(projects, QStringLiteral("name"), QStringLiteral("zzz"));
    QVERIFY(filtered.isEmpty());
}

void TitleFilterTest::clearingFilterRestoresFullList()
{
    const QVariantList chats = {
        chatRow(1, QStringLiteral("Alpha One")),
        chatRow(2, QStringLiteral("Beta One")),
    };

    QCOMPARE(filterItemsByTitle(chats, QStringLiteral("title"), QStringLiteral("Alpha")).size(), 1);
    QCOMPARE(filterItemsByTitle(chats, QStringLiteral("title"), QString()).size(), 2);
}

void TitleFilterTest::messageBodiesAreNotSearched()
{
    const QVariantList chats = {
        chatRow(1, QStringLiteral("Status"), QStringLiteral("secret needle in the body")),
        chatRow(2, QStringLiteral("Needle in title")),
    };

    const QVariantList filtered = filterItemsByTitle(chats, QStringLiteral("title"), QStringLiteral("needle"));
    QCOMPARE(namesOf(filtered, QStringLiteral("title")), QStringList{QStringLiteral("Needle in title")});
}

void TitleFilterTest::filterDoesNotMutateSourceList()
{
    QVariantList projects = {
        projectRow(1, QStringLiteral("Alpha")),
        projectRow(2, QStringLiteral("Beta")),
    };
    const int originalSize = projects.size();

    QCOMPARE(filterItemsByTitle(projects, QStringLiteral("name"), QStringLiteral("Alpha")).size(), 1);
    QCOMPARE(projects.size(), originalSize);
    QCOMPARE(projects.at(1).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Beta"));
}

void TitleFilterTest::persistenceAndArchiveAreUnchangedByFiltering()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    qint64 workId = 0;
    qint64 keepChatId = 0;
    {
        AppController controller;
        QString error;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.createProject(QStringLiteral("Kitchen")));
        QVERIFY(controller.createProject(QStringLiteral("Garden")));
        QVERIFY(controller.createProject(QStringLiteral("Work")));
        workId = controller.currentProjectId();
        QVERIFY(controller.createChat(QStringLiteral("Standup notes")));
        QVERIFY(controller.createChat(QStringLiteral("Release checklist")));
        QVERIFY(controller.createChat(QStringLiteral("Archived planning")));
        QVERIFY(controller.archiveCurrentChat());
        QCOMPARE(controller.chats().size(), 2);
        keepChatId = controller.currentChatId();

        const QVariantList filteredProjects =
            filterItemsByTitle(controller.projects(), QStringLiteral("name"), QStringLiteral("work"));
        QCOMPARE(namesOf(filteredProjects, QStringLiteral("name")), QStringList{QStringLiteral("Work")});
        QCOMPARE(controller.projects().size(), 3);

        const QVariantList filteredChats =
            filterItemsByTitle(controller.chats(), QStringLiteral("title"), QStringLiteral("stand"));
        QCOMPARE(namesOf(filteredChats, QStringLiteral("title")), QStringList{QStringLiteral("Standup notes")});
        QCOMPARE(controller.chats().size(), 2);
        QCOMPARE(controller.currentChatId(), keepChatId);

        QVERIFY(controller.createChat(QStringLiteral("Delete me")));
        QVERIFY(controller.deleteCurrentChat());
        QCOMPARE(controller.chats().size(), 2);
    }

    AppController controller;
    QString error;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(controller.projects().size(), 3);
    QCOMPARE(filterItemsByTitle(controller.projects(), QStringLiteral("name"), QString()).size(), 3);
    controller.selectProject(workId);
    QCOMPARE(controller.chats().size(), 2);
    QCOMPARE(namesOf(controller.chats(), QStringLiteral("title")),
             (QStringList{QStringLiteral("Release checklist"), QStringLiteral("Standup notes")}));

    LocalDatabase db;
    QVERIFY2(db.open(path, &error), qUtf8Printable(error));
    QCOMPARE(db.listProjectIds(&error).size(), 3);
    QCOMPARE(db.listChatIds(workId, false, &error).size(), 2);
    QCOMPARE(db.listChatIds(workId, true, &error).size(), 3);
}

QTEST_GUILESS_MAIN(TitleFilterTest)
#include "titlefilter_test.moc"
