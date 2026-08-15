#include "fake_launcher_server.h"
#include "picoclawlifecycle.h"

#include <QMetaType>
#include <QSignalSpy>
#include <QtTest>

Q_DECLARE_METATYPE(PicoClawLifecycleStatus)
Q_DECLARE_METATYPE(PicoClawLifecycleVersion)
Q_DECLARE_METATYPE(PicoClawLifecycleCommandResult)

class PicoClawLifecycleTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        qRegisterMetaType<PicoClawLifecycleStatus>("PicoClawLifecycleStatus");
        qRegisterMetaType<PicoClawLifecycleVersion>("PicoClawLifecycleVersion");
        qRegisterMetaType<PicoClawLifecycleCommandResult>("PicoClawLifecycleCommandResult");
    }
    void parsesRunningAndStoppedStatus();
    void parsesVersion();
    void executesStartStopRestart();
    void handlesUnauthorizedAndCommandFailure();
};

void PicoClawLifecycleTest::parsesRunningAndStoppedStatus()
{
    FakeLauncherServer server;
    QVERIFY(server.listen());
    server.setPassword(QStringLiteral("secret"));
    server.setGatewayStatus(QStringLiteral("running"));

    PicoClawLifecycleClient client;
    client.setBaseUrl(server.baseUrl());
    client.setPassword(QStringLiteral("secret"));

    QSignalSpy loginSpy(&client, &PicoClawLifecycleClient::loginFinished);
    client.login();
    QTRY_COMPARE(loginSpy.size(), 1);
    QCOMPARE(loginSpy.at(0).at(0).toBool(), true);
    QVERIFY(client.hasSession());

    QSignalSpy statusSpy(&client, &PicoClawLifecycleClient::statusFinished);
    client.refreshStatus();
    QTRY_COMPARE(statusSpy.size(), 1);
    auto running = statusSpy.at(0).at(0).value<PicoClawLifecycleStatus>();
    QVERIFY(running.ok);
    QCOMPARE(running.gatewayStatus, QStringLiteral("running"));
    QCOMPARE(running.gatewayVersion, QStringLiteral("0.3.1"));
    QCOMPARE(running.pid, 42);

    server.setGatewayStatus(QStringLiteral("stopped"));
    client.refreshStatus();
    QTRY_COMPARE(statusSpy.size(), 2);
    auto stopped = statusSpy.at(1).at(0).value<PicoClawLifecycleStatus>();
    QVERIFY(stopped.ok);
    QCOMPARE(stopped.gatewayStatus, QStringLiteral("stopped"));
}

void PicoClawLifecycleTest::parsesVersion()
{
    FakeLauncherServer server;
    QVERIFY(server.listen());
    PicoClawLifecycleClient client;
    client.setBaseUrl(server.baseUrl());
    client.setPassword(QStringLiteral("secret"));
    QSignalSpy loginSpy(&client, &PicoClawLifecycleClient::loginFinished);
    client.login();
    QTRY_COMPARE(loginSpy.size(), 1);
    QCOMPARE(loginSpy.at(0).at(0).toBool(), true);

    QSignalSpy versionSpy(&client, &PicoClawLifecycleClient::versionFinished);
    client.refreshVersion();
    QTRY_COMPARE(versionSpy.size(), 1);
    auto version = versionSpy.at(0).at(0).value<PicoClawLifecycleVersion>();
    QVERIFY(version.ok);
    QCOMPARE(version.version, QStringLiteral("0.3.1"));
    QCOMPARE(version.gitCommit, QStringLiteral("2cf030d2"));
}

void PicoClawLifecycleTest::executesStartStopRestart()
{
    FakeLauncherServer server;
    QVERIFY(server.listen());
    PicoClawLifecycleClient client;
    client.setBaseUrl(server.baseUrl());
    client.setPassword(QStringLiteral("secret"));
    QSignalSpy loginSpy(&client, &PicoClawLifecycleClient::loginFinished);
    client.login();
    QTRY_COMPARE(loginSpy.size(), 1);
    QCOMPARE(loginSpy.at(0).at(0).toBool(), true);

    QSignalSpy commandSpy(&client, &PicoClawLifecycleClient::commandFinished);
    client.startGateway();
    QTRY_COMPARE(commandSpy.size(), 1);
    QCOMPARE(commandSpy.at(0).at(0).toString(), QStringLiteral("start"));
    QVERIFY(commandSpy.at(0).at(1).value<PicoClawLifecycleCommandResult>().ok);
    QCOMPARE(server.startCount(), 1);

    client.stopGateway();
    QTRY_COMPARE(commandSpy.size(), 2);
    QCOMPARE(commandSpy.at(1).at(0).toString(), QStringLiteral("stop"));
    QCOMPARE(server.stopCount(), 1);

    client.restartGateway();
    QTRY_COMPARE(commandSpy.size(), 3);
    QCOMPARE(commandSpy.at(2).at(0).toString(), QStringLiteral("restart"));
    QCOMPARE(commandSpy.at(2).at(1).value<PicoClawLifecycleCommandResult>().pid, 43);
    QCOMPARE(server.restartCount(), 1);
}

void PicoClawLifecycleTest::handlesUnauthorizedAndCommandFailure()
{
    FakeLauncherServer server;
    QVERIFY(server.listen());
    PicoClawLifecycleClient client;
    client.setBaseUrl(server.baseUrl());
    client.setPassword(QStringLiteral("wrong"));
    QSignalSpy loginSpy(&client, &PicoClawLifecycleClient::loginFinished);
    client.login();
    QTRY_COMPARE(loginSpy.size(), 1);
    QCOMPARE(loginSpy.at(0).at(0).toBool(), false);
    QVERIFY(loginSpy.at(0).at(1).toString().contains(QStringLiteral("invalid password")));

    client.setPassword(QStringLiteral("secret"));
    client.login();
    QTRY_COMPARE(loginSpy.size(), 2);
    QCOMPARE(loginSpy.at(1).at(0).toBool(), true);

    server.setCommandFailStatus(
        400,
        QByteArrayLiteral("{\"status\":\"precondition_failed\",\"message\":\"no credentials\"}"));
    QSignalSpy commandSpy(&client, &PicoClawLifecycleClient::commandFinished);
    client.startGateway();
    QTRY_COMPARE(commandSpy.size(), 1);
    auto failed = commandSpy.at(0).at(1).value<PicoClawLifecycleCommandResult>();
    QVERIFY(!failed.ok);
    QVERIFY(failed.error.contains(QStringLiteral("no credentials")));
    QCOMPARE(failed.status, QStringLiteral("precondition_failed"));
}

QTEST_MAIN(PicoClawLifecycleTest)
#include "picoclawlifecycle_test.moc"
