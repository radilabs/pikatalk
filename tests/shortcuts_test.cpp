#include <QFile>
#include <QRegularExpression>
#include <QtTest>

class ShortcutsTest : public QObject
{
    Q_OBJECT

private:
    static QString loadMainQml()
    {
        QFile file(QStringLiteral(PIKATALK_SOURCE_DIR "/src/Main.qml"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }

    static QString shortcutBlock(const QString &qml, const QString &objectName)
    {
        const QRegularExpression re(
            QStringLiteral("Shortcut\\s*\\{([^}]*objectName:\\s*\"%1\"[^}]*)\\}").arg(objectName));
        const QRegularExpressionMatch match = re.match(qml);
        return match.hasMatch() ? match.captured(1) : QString();
    }

private Q_SLOTS:
    void ctrlNCreatesChatWhenProjectIsSelected();
    void ctrlFFocusesTitleFilter();
    void ctrlLFocusesMessageInput();
    void escapeStopsGenerationOnlyWhileActive();
};

void ShortcutsTest::ctrlNCreatesChatWhenProjectIsSelected()
{
    const QString qml = loadMainQml();
    QVERIFY(!qml.isEmpty());

    const QString block = shortcutBlock(qml, QStringLiteral("newChatShortcut"));
    QVERIFY2(!block.isEmpty(), "Main.qml must declare Shortcut objectName newChatShortcut");
    QVERIFY(block.contains(QStringLiteral("StandardKey.New")));
    QVERIFY(block.contains(QStringLiteral("app.currentProjectId > 0")));
    QVERIFY(block.contains(QStringLiteral("app.createChat")));
}

void ShortcutsTest::ctrlFFocusesTitleFilter()
{
    const QString qml = loadMainQml();
    const QString block = shortcutBlock(qml, QStringLiteral("titleFilterShortcut"));
    QVERIFY2(!block.isEmpty(), "Main.qml must declare Shortcut objectName titleFilterShortcut");
    QVERIFY(block.contains(QStringLiteral("StandardKey.Find")));
    QVERIFY(block.contains(QStringLiteral("titleFilterField.forceActiveFocus()")));
    QVERIFY(block.contains(QStringLiteral("titleFilterField.selectAll()")));
}

void ShortcutsTest::ctrlLFocusesMessageInput()
{
    const QString qml = loadMainQml();
    const QString block = shortcutBlock(qml, QStringLiteral("messageInputShortcut"));
    QVERIFY2(!block.isEmpty(), "Main.qml must declare Shortcut objectName messageInputShortcut");
    QVERIFY(block.contains(QStringLiteral("\"Ctrl+L\"")));
    QVERIFY(block.contains(QStringLiteral("messageInput.forceActiveFocus()")));
}

void ShortcutsTest::escapeStopsGenerationOnlyWhileActive()
{
    const QString qml = loadMainQml();
    const QString block = shortcutBlock(qml, QStringLiteral("stopGenerationShortcut"));
    QVERIFY2(!block.isEmpty(), "Main.qml must declare Shortcut objectName stopGenerationShortcut");
    QVERIFY(block.contains(QStringLiteral("\"Escape\"")) || block.contains(QStringLiteral("StandardKey.Cancel")));
    QVERIFY(block.contains(QStringLiteral("enabled: app.isGenerating")));
    QVERIFY(block.contains(QStringLiteral("app.stopGeneration()")));
}

QTEST_GUILESS_MAIN(ShortcutsTest)
#include "shortcuts_test.moc"
