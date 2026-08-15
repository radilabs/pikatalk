#include "appcontroller.h"
#include "database.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

namespace {

constexpr int kTurnCount = 60;
constexpr int kToolCount = 8;
constexpr int kReloadBudgetMs = 2000;
constexpr int kScrollBudgetMs = 2000;

const char *kCodeFence = R"md(Here is a helper:

```cpp
int fib(int n)
{
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}
```

End of block.)md";

struct SeededChat {
    qint64 projectId = 0;
    qint64 longChatId = 0;
    qint64 shortChatId = 0;
    qint64 lastAssistantId = 0;
    QString firstUser;
    QString lastAssistant;
    QString codeSnippet;
};

QString longUserText(int turn)
{
    return QStringLiteral("User turn %1: please walk through the kitchen plan, wiring notes, "
                          "and leftover questions from last week. Marker=U%1")
        .arg(turn);
}

QString longAssistantText(int turn)
{
    if (turn % 10 == 0) {
        return QStringLiteral("Assistant turn %1.\n\n%2\n\nMarker=A%1").arg(turn).arg(QString::fromUtf8(kCodeFence));
    }
    return QStringLiteral("Assistant turn %1 with a few paragraphs of local advice about "
                          "cabinets, lighting, and sequencing. Marker=A%1")
        .arg(turn);
}

bool seedLongConversation(const QString &dbPath, SeededChat *out, QString *error)
{
    LocalDatabase db;
    if (!db.open(dbPath, error)) {
        return false;
    }
    out->projectId = db.createProject(QStringLiteral("Long Chat Project"),
                                     QStringLiteral("."),
                                     QStringLiteral("local-model"),
                                     error);
    if (out->projectId <= 0) {
        return false;
    }
    out->longChatId = db.createChat(out->projectId, QStringLiteral("Long conversation"), error);
    out->shortChatId = db.createChat(out->projectId, QStringLiteral("Short chat"), error);
    if (out->longChatId <= 0 || out->shortChatId <= 0) {
        return false;
    }
    if (db.addMessage(out->shortChatId, QStringLiteral("user"), QStringLiteral("short hello"), error) <= 0) {
        return false;
    }
    if (db.addMessage(out->shortChatId, QStringLiteral("assistant"), QStringLiteral("short reply"), error) <= 0) {
        return false;
    }

    out->firstUser = longUserText(1);
    out->codeSnippet = QStringLiteral("int fib(int n)");
    for (int turn = 1; turn <= kTurnCount; ++turn) {
        if (db.addMessage(out->longChatId, QStringLiteral("user"), longUserText(turn), error) <= 0) {
            return false;
        }
        out->lastAssistant = longAssistantText(turn);
        out->lastAssistantId = db.addMessage(out->longChatId, QStringLiteral("assistant"), out->lastAssistant, error);
        if (out->lastAssistantId <= 0) {
            return false;
        }
    }

    for (int i = 0; i < kToolCount; ++i) {
        const QString status = (i == kToolCount - 1) ? QStringLiteral("error") : QStringLiteral("ok");
        const QString result = QStringLiteral("tool-result-%1\n%2").arg(i).arg(QString(400, QLatin1Char('x')));
        if (db.addToolActivity(out->longChatId,
                               out->lastAssistantId,
                               QStringLiteral("call-%1").arg(i),
                               QStringLiteral("read_file"),
                               QStringLiteral("{\"path\":\"notes.md\"}"),
                               QStringLiteral("{\"id\":\"call-%1\"}").arg(i),
                               result,
                               status,
                               status == QStringLiteral("error") ? QStringLiteral("permission denied") : QString(),
                               i + 1,
                               error)
            <= 0) {
            return false;
        }
    }

    if (!db.saveDraft(out->longChatId, QStringLiteral("unsent long-chat draft"), error)) {
        return false;
    }
    if (!db.touchChat(out->longChatId, error)) {
        return false;
    }
    db.close();
    return true;
}

QString loadMainQml()
{
    QFile file(QStringLiteral(PIKATALK_SOURCE_DIR "/src/Main.qml"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

} // namespace

class LongChatTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void longConversationReloadsAfterRestartAndChatSwitch();
    void draftsSurviveOnLongChatAcrossSwitchAndRestart();
    void copyTextAndCodeSegmentsRemainUsableOnLongHistory();
    void conversationListViewStaysPlainQtListView();
    void offscreenListViewScrollsLongConversationWithoutFreeze();
    void offscreenPikaTalkLoadsSeededLongChat();
};

void LongChatTest::longConversationReloadsAfterRestartAndChatSwitch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    SeededChat seed;
    QString error;
    QVERIFY2(seedLongConversation(path, &seed, &error), qUtf8Printable(error));

    AppController controller;
    QElapsedTimer reloadTimer;
    reloadTimer.start();
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    const qint64 reloadMs = reloadTimer.elapsed();
    QVERIFY2(reloadMs < kReloadBudgetMs, qUtf8Printable(QStringLiteral("openStore of long chat took %1 ms").arg(reloadMs)));

    QCOMPARE(controller.currentChatId(), seed.longChatId);
    QCOMPARE(controller.messages().size(), kTurnCount * 2);
    QCOMPARE(controller.toolActivities().size(), kToolCount);
    QCOMPARE(controller.messages().first().toMap().value(QStringLiteral("content")).toString(), seed.firstUser);
    QCOMPARE(controller.messages().last().toMap().value(QStringLiteral("content")).toString(), seed.lastAssistant);
    QCOMPARE(controller.currentDraft(), QStringLiteral("unsent long-chat draft"));

    controller.selectChat(seed.shortChatId);
    QCOMPARE(controller.messages().size(), 2);
    QCOMPARE(controller.messages().first().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("short hello"));
    QCOMPARE(controller.toolActivities().size(), 0);

    controller.selectChat(seed.longChatId);
    QCOMPARE(controller.messages().size(), kTurnCount * 2);
    QCOMPARE(controller.toolActivities().size(), kToolCount);
    QCOMPARE(controller.currentDraft(), QStringLiteral("unsent long-chat draft"));

    QVERIFY(controller.addUserMessage(QStringLiteral("one more question after the long history")));
    QCOMPARE(controller.messages().size(), kTurnCount * 2 + 1);
    QCOMPARE(controller.messages().last().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("one more question after the long history"));
    QCOMPARE(controller.currentDraft(), QString());

    AppController restarted;
    QVERIFY2(restarted.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(restarted.currentChatId(), seed.longChatId);
    QCOMPARE(restarted.messages().size(), kTurnCount * 2 + 1);
    QCOMPARE(restarted.toolActivities().size(), kToolCount);
    QCOMPARE(restarted.messages().last().toMap().value(QStringLiteral("content")).toString(),
             QStringLiteral("one more question after the long history"));
}

void LongChatTest::draftsSurviveOnLongChatAcrossSwitchAndRestart()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    SeededChat seed;
    QString error;
    QVERIFY2(seedLongConversation(path, &seed, &error), qUtf8Printable(error));

    {
        AppController controller;
        QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
        QVERIFY(controller.setCurrentDraft(QStringLiteral("keep this long-chat draft")));
        controller.selectChat(seed.shortChatId);
        QVERIFY(controller.setCurrentDraft(QStringLiteral("short draft")));
        controller.selectChat(seed.longChatId);
        QCOMPARE(controller.currentDraft(), QStringLiteral("keep this long-chat draft"));
        QCOMPARE(controller.messages().size(), kTurnCount * 2);
    }

    AppController restarted;
    QVERIFY2(restarted.openStore(path, &error), qUtf8Printable(error));
    QCOMPARE(restarted.currentChatId(), seed.longChatId);
    QCOMPARE(restarted.currentDraft(), QStringLiteral("keep this long-chat draft"));
    restarted.selectChat(seed.shortChatId);
    QCOMPARE(restarted.currentDraft(), QStringLiteral("short draft"));
    QCOMPARE(restarted.messages().size(), 2);
}

void LongChatTest::copyTextAndCodeSegmentsRemainUsableOnLongHistory()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    SeededChat seed;
    QString error;
    QVERIFY2(seedLongConversation(path, &seed, &error), qUtf8Printable(error));

    AppController controller;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));
    const int before = controller.messages().size();

    const QString prose = controller.messages().first().toMap().value(QStringLiteral("content")).toString();
    controller.copyText(prose);
    QCOMPARE(controller.lastCopiedText(), prose);

    QVariantList segments;
    for (const QVariant &row : controller.messages()) {
        const QVariantMap map = row.toMap();
        if (map.value(QStringLiteral("role")).toString() != QStringLiteral("assistant")) {
            continue;
        }
        const QVariantList parts = controller.messageSegments(map.value(QStringLiteral("content")).toString());
        if (parts.size() > 1) {
            segments = parts;
            break;
        }
    }
    QVERIFY(segments.size() >= 2);
    QString code;
    for (const QVariant &part : segments) {
        const QVariantMap map = part.toMap();
        if (map.value(QStringLiteral("kind")).toString() == QStringLiteral("code")) {
            code = map.value(QStringLiteral("text")).toString();
            break;
        }
    }
    QVERIFY(code.contains(seed.codeSnippet));
    controller.copyText(code);
    QCOMPARE(controller.lastCopiedText(), code);
    QCOMPARE(controller.messages().size(), before);
}

void LongChatTest::conversationListViewStaysPlainQtListView()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("id: conversationArea")));
    QVERIFY(qml.contains(QStringLiteral("objectName: \"conversationArea\"")));
    QVERIFY(qml.contains(QStringLiteral("clip: true")));
    QVERIFY(qml.contains(QStringLiteral("objectName: \"copyMessage\"")));
    QVERIFY(qml.contains(QStringLiteral("objectName: \"copyCodeBlock\"")));
    QVERIFY(qml.contains(QStringLiteral("objectName: \"toolActivityDetails\"")));
    QVERIFY(qml.contains(QStringLiteral("visible: false")));
    QVERIFY(qml.contains(QStringLiteral("toolDetails.visible = !toolDetails.visible")));
    QVERIFY(!qml.contains(QStringLiteral("pagination")));
    QVERIFY(!qml.contains(QStringLiteral("messageWindow")));
    QVERIFY(!qml.contains(QStringLiteral("lazyLoad")));
}

void LongChatTest::offscreenListViewScrollsLongConversationWithoutFreeze()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = LocalDatabase::databaseFilePath(tmp.path());
    SeededChat seed;
    QString error;
    QVERIFY2(seedLongConversation(path, &seed, &error), qUtf8Printable(error));

    AppController controller;
    QVERIFY2(controller.openStore(path, &error), qUtf8Printable(error));

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(R"(
import QtQuick
ListView {
    id: conversationArea
    objectName: "conversationArea"
    width: 640
    height: 480
    clip: true
    model: {
        const messages = app.messages;
        const tools = app.toolActivities;
        const items = [];
        for (let i = 0; i < messages.length; ++i) {
            items.push({ kind: "message", data: messages[i] });
        }
        for (let i = 0; i < tools.length; ++i) {
            items.push({ kind: "tool", data: tools[i] });
        }
        return items;
    }
    delegate: Item {
        required property var modelData
        width: ListView.view.width
        height: body.height
        Text {
            id: body
            width: parent.width
            wrapMode: Text.Wrap
            text: modelData.kind === "message"
                  ? (modelData.data.content || "")
                  : ("Tool: " + (modelData.data.toolName || ""))
        }
    }
}
)"),
                      QUrl(QStringLiteral("qrc:/longchat-list.qml")));
    QVERIFY2(component.errors().isEmpty(), qUtf8Printable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    auto *view = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(view);
    QQuickWindow window;
    view->setParentItem(window.contentItem());
    window.resize(640, 480);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QCOMPARE(view->property("count").toInt(), kTurnCount * 2 + kToolCount);

    QElapsedTimer scrollTimer;
    scrollTimer.start();
    QMetaObject::invokeMethod(view, "positionViewAtBeginning");
    QCoreApplication::processEvents();
    QMetaObject::invokeMethod(view, "positionViewAtEnd");
    QCoreApplication::processEvents();
    QMetaObject::invokeMethod(view, "positionViewAtBeginning");
    QCoreApplication::processEvents();
    const qint64 scrollMs = scrollTimer.elapsed();
    QVERIFY2(scrollMs < kScrollBudgetMs, qUtf8Printable(QStringLiteral("ListView scroll took %1 ms").arg(scrollMs)));
    QCOMPARE(controller.currentDraft(), QStringLiteral("unsent long-chat draft"));
}

void LongChatTest::offscreenPikaTalkLoadsSeededLongChat()
{
    const QString binary = QStringLiteral(PIKATALK_BIN);
    QVERIFY(QFile::exists(binary));

    QTemporaryDir xdg;
    QVERIFY(xdg.isValid());
    const QString dataHome = xdg.filePath(QStringLiteral("share"));
    const QString dataDir = QDir(dataHome).filePath(QStringLiteral("Radilabs/PikaTalk"));
    QVERIFY(QDir().mkpath(dataDir));
    QVERIFY(QDir().mkpath(xdg.filePath(QStringLiteral("config"))));
    QVERIFY(QDir().mkpath(xdg.filePath(QStringLiteral("cache"))));

    SeededChat seed;
    QString error;
    QVERIFY2(seedLongConversation(LocalDatabase::databaseFilePath(dataDir), &seed, &error), qUtf8Printable(error));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    env.insert(QStringLiteral("XDG_DATA_HOME"), dataHome);
    env.insert(QStringLiteral("XDG_CONFIG_HOME"), xdg.filePath(QStringLiteral("config")));
    env.insert(QStringLiteral("XDG_CACHE_HOME"), xdg.filePath(QStringLiteral("cache")));
    env.remove(QStringLiteral("PIKATALK_LIVE_GATEWAY"));

    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    QElapsedTimer startTimer;
    startTimer.start();
    proc.start(binary, {});
    QVERIFY2(proc.waitForStarted(5000), qUtf8Printable(proc.errorString()));
    QTest::qWait(2500);
    const qint64 startMs = startTimer.elapsed();
    QVERIFY(proc.state() == QProcess::Running);
    proc.terminate();
    if (!proc.waitForFinished(3000)) {
        proc.kill();
        proc.waitForFinished(2000);
    }

    const QString output = QString::fromUtf8(proc.readAll());
    QVERIFY2(!output.contains(QStringLiteral("Failed to load the PikaTalk QML interface")), qUtf8Printable(output));
    QVERIFY2(!output.contains(QStringLiteral("SQLite initialization failed")), qUtf8Printable(output));
    QVERIFY2(!output.contains(QStringLiteral("picoclaw-launcher")), qUtf8Printable(output));
    QVERIFY2(startMs < 8000, qUtf8Printable(QStringLiteral("offscreen start took %1 ms").arg(startMs)));
}

QTEST_MAIN(LongChatTest)
#include "longchat_test.moc"
