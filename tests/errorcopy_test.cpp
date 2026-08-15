#include "errorcopy.h"

#include <QtTest>

class ErrorCopyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyInputStaysEmpty();
    void shortHumanMessagesStayReadable();
    void jsonPayloadsBecomeGenericFailure();
    void protocolNoiseBecomesConnectionFailed();
    void longDumpsAreTruncated();
    void whitespaceIsCollapsed();
};

void ErrorCopyTest::emptyInputStaysEmpty()
{
    QCOMPARE(sanitizeUserFacingError(QString()), QString());
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("   ")), QString());
}

void ErrorCopyTest::shortHumanMessagesStayReadable()
{
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("gateway unavailable")),
             QStringLiteral("Gateway unavailable"));
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("connection lost")),
             QStringLiteral("Connection lost"));
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("gateway error")),
             QStringLiteral("Request failed"));
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("Model not found")),
             QStringLiteral("Model not found"));
}

void ErrorCopyTest::jsonPayloadsBecomeGenericFailure()
{
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("{\"code\":\"E_PROTO\",\"stack\":[]}")),
             QStringLiteral("Request failed"));
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("[{\"type\":\"error\"}]")),
             QStringLiteral("Request failed"));
}

void ErrorCopyTest::protocolNoiseBecomesConnectionFailed()
{
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("QAbstractSocket::ConnectionRefusedError")),
             QStringLiteral("Connection failed"));
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("WebSocket handshake failed: 401")),
             QStringLiteral("Connection failed"));
}

void ErrorCopyTest::longDumpsAreTruncated()
{
    const QString dump = QStringLiteral("Failed to complete chat turn because the upstream ").repeated(8);
    const QString sanitized = sanitizeUserFacingError(dump);
    QVERIFY(sanitized.size() <= 120);
    QVERIFY(sanitized.endsWith(QStringLiteral("…")));
    QVERIFY(!sanitized.contains(QLatin1Char('\n')));
}

void ErrorCopyTest::whitespaceIsCollapsed()
{
    QCOMPARE(sanitizeUserFacingError(QStringLiteral("timed out\n\nafter 30s")),
             QStringLiteral("timed out after 30s"));
}

QTEST_GUILESS_MAIN(ErrorCopyTest)
#include "errorcopy_test.moc"
