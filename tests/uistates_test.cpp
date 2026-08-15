#include <QFile>
#include <QtTest>

class UiStatesTest : public QObject
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

private Q_SLOTS:
    void emptyProjectsPlaceholderIsDeclared();
    void emptyChatsPlaceholderIsDeclared();
    void emptyMessagesPlaceholderIsDeclared();
    void generationStatusUsesExistingGeneratingCopy();
    void requestErrorUsesSanitizedCopy();
    void gatewayToolbarKeepsConnectingStoppedAndError();
};

void UiStatesTest::emptyProjectsPlaceholderIsDeclared()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"emptyProjectsPlaceholder\"")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"No projects\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Create a project\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"No matching projects\")")));
}

void UiStatesTest::emptyChatsPlaceholderIsDeclared()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"emptyChatsPlaceholder\"")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"No chats\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Create a chat\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"No matching chats\")")));
}

void UiStatesTest::emptyMessagesPlaceholderIsDeclared()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"emptyMessagesPlaceholder\"")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"No messages\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Send a message\")")));
    QVERIFY(qml.contains(QStringLiteral("!app.isGenerating")));
}

void UiStatesTest::generationStatusUsesExistingGeneratingCopy()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"generationStatusLabel\"")));
    QVERIFY(qml.contains(QStringLiteral("visible: app.isGenerating")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Generating…\")")));
}

void UiStatesTest::requestErrorUsesSanitizedCopy()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"requestErrorLabel\"")));
    QVERIFY(qml.contains(QStringLiteral("errorCopy.sanitize(app.requestError)")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Error: %1\"")));
}

void UiStatesTest::gatewayToolbarKeepsConnectingStoppedAndError()
{
    const QString qml = loadMainQml();
    QVERIFY(qml.contains(QStringLiteral("objectName: \"gatewayPlaceholder\"")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Gateway: Connecting\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Gateway: Reconnecting…\")")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Gateway: Stopped\")")));
    QVERIFY(qml.contains(QStringLiteral("errorCopy.sanitize")));
    QVERIFY(qml.contains(QStringLiteral("i18n(\"Gateway: Error — %1\"")));
}

QTEST_GUILESS_MAIN(UiStatesTest)
#include "uistates_test.moc"
