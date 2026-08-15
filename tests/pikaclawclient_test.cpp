#include "fake_pico_server.h"
#include "pikaclawclient.h"
#include "pikaclawsettings.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class PicoClawClientTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void connectsWhenGatewayIsReachable();
    void reportsErrorWhenGatewayIsUnreachable();
    void reportsErrorWhenTokenIsRejected();
    void reconnectsAfterTemporaryFailure();
    void readsPicoTokenFromSecurityFile();
    void loadsEndpointFromPikaTalkConfig();
    void loadsModelNamesFromPicoConfig();
    void loadsDefaultModelFromPicoConfig();
    void sendJsonDeliversMessageSend();
    void sendJsonDeliversLongMessageSend();
};

void PicoClawClientTest::connectsWhenGatewayIsReachable()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());

    PicoClawClient client;
    client.setAutoReconnect(false);
    client.setEndpoint(server.wsUrl());
    client.setToken(QStringLiteral("test-token"));
    client.connectToGateway();

    QTRY_COMPARE(client.connectionState(), QStringLiteral("connected"));
    QCOMPARE(client.lastError(), QString());
}

void PicoClawClientTest::reportsErrorWhenGatewayIsUnreachable()
{
    PicoClawClient client;
    client.setAutoReconnect(false);
    client.setEndpoint(QUrl(QStringLiteral("ws://127.0.0.1:1/pico/ws")));
    client.setToken(QStringLiteral("test-token"));
    client.connectToGateway();

    QTRY_COMPARE(client.connectionState(), QStringLiteral("error"));
    QVERIFY(!client.lastError().isEmpty());
}

void PicoClawClientTest::reportsErrorWhenTokenIsRejected()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("expected-token"));
    QVERIFY(server.listen());

    PicoClawClient client;
    client.setAutoReconnect(false);
    client.setEndpoint(server.wsUrl());
    client.setToken(QStringLiteral("wrong-token"));
    client.connectToGateway();

    QTRY_COMPARE(client.connectionState(), QStringLiteral("error"));
    QVERIFY(client.lastError().contains(QStringLiteral("401"))
            || client.lastError().contains(QStringLiteral("unauthorized"), Qt::CaseInsensitive));
}

void PicoClawClientTest::reconnectsAfterTemporaryFailure()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    const quint16 port = server.port();

    PicoClawClient client;
    client.setAutoReconnect(true);
    client.setReconnectIntervalMs(100);
    client.setEndpoint(server.wsUrl());
    client.setToken(QStringLiteral("test-token"));
    client.connectToGateway();
    QTRY_COMPARE(client.connectionState(), QStringLiteral("connected"));

    server.stopListening();
    QTRY_VERIFY(client.connectionState() != QStringLiteral("connected"));

    server.setPreferredPort(port);
    QVERIFY(server.listen());
    QTRY_COMPARE(client.connectionState(), QStringLiteral("connected"));
}

void PicoClawClientTest::readsPicoTokenFromSecurityFile()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral(".security.yml"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("channel_list:\n"
               "  pico:\n"
               "    settings:\n"
               "      token: unit-test-token\n");
    file.close();
    QCOMPARE(readPicoChannelToken(path), QStringLiteral("unit-test-token"));
}

void PicoClawClientTest::loadsEndpointFromPikaTalkConfig()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QFile file(QDir(tmp.path()).filePath(QStringLiteral("pikatalk.conf")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("[picoClaw]\nendpoint=ws://127.0.0.1:18791/pico/ws\ntoken=from-config\n");
    file.close();

    const PicoClawConnectionSettings settings = loadPicoClawConnectionSettings(tmp.path());
    QCOMPARE(settings.endpoint, QUrl(QStringLiteral("ws://127.0.0.1:18791/pico/ws")));
    QCOMPARE(settings.token, QStringLiteral("from-config"));
}

void PicoClawClientTest::loadsModelNamesFromPicoConfig()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("config.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"model_list":[{"model_name":"step-3.7-flash"},{"model_name":"glm-4.7"}]})");
    file.close();
    QCOMPARE(loadPicoClawModelNames(path),
             QStringList({QStringLiteral("step-3.7-flash"), QStringLiteral("glm-4.7")}));
}

void PicoClawClientTest::loadsDefaultModelFromPicoConfig()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("config.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"agents":{"defaults":{"model_name":"step-3.7-flash"}},"model_list":[{"model_name":"glm-4.7"}]})");
    file.close();
    QCOMPARE(loadPicoClawDefaultModelName(path), QStringLiteral("step-3.7-flash"));
    QCOMPARE(loadPicoClawDefaultModelName(tmp.filePath(QStringLiteral("missing.json"))), QString());
}

void PicoClawClientTest::sendJsonDeliversMessageSend()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    PicoClawClient client;
    client.setAutoReconnect(false);
    client.setEndpoint(server.wsUrl());
    client.setToken(QStringLiteral("test-token"));
    client.connectToGateway();
    QTRY_COMPARE(client.connectionState(), QStringLiteral("connected"));
    client.sendJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("message.send")},
                                {QStringLiteral("payload"), QJsonObject{{QStringLiteral("content"), QStringLiteral("hello")}}}});
    QTRY_COMPARE(server.clientMessages().size(), 1);
    QCOMPARE(server.clientMessages().at(0).value(QStringLiteral("type")).toString(), QStringLiteral("message.send"));
    QCOMPARE(server.clientMessages().at(0).value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("hello"));
}

void PicoClawClientTest::sendJsonDeliversLongMessageSend()
{
    FakePicoServer server;
    server.setRequiredToken(QStringLiteral("test-token"));
    QVERIFY(server.listen());
    PicoClawClient client;
    client.setAutoReconnect(false);
    client.setEndpoint(server.wsUrl());
    client.setToken(QStringLiteral("test-token"));
    client.connectToGateway();
    QTRY_COMPARE(client.connectionState(), QStringLiteral("connected"));
    const QString longContent = QStringLiteral("x").repeated(200);
    client.sendJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("message.send")},
                                {QStringLiteral("payload"), QJsonObject{{QStringLiteral("content"), longContent}}}});
    QTRY_COMPARE(server.clientMessages().size(), 1);
    QCOMPARE(server.clientMessages().at(0).value(QStringLiteral("payload")).toObject().value(QStringLiteral("content")).toString(),
             longContent);
}

QTEST_GUILESS_MAIN(PicoClawClientTest)
#include "pikaclawclient_test.moc"
