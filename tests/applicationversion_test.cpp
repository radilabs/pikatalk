#include "applicationidentity.h"

#include <QCoreApplication>
#include <QtTest>

class ApplicationVersionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void runtimeVersionMatchesBuildVersion();
};

void ApplicationVersionTest::runtimeVersionMatchesBuildVersion()
{
    configureApplicationIdentity();

    QCOMPARE(QCoreApplication::applicationVersion(), QStringLiteral(PIKATALK_VERSION));
    QCOMPARE(QCoreApplication::applicationVersion(), QStringLiteral("1.0.0"));
}

QTEST_GUILESS_MAIN(ApplicationVersionTest)
#include "applicationversion_test.moc"
