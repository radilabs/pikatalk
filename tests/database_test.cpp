#include "database.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class DatabaseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void placesTheDatabaseUnderTheDataDirectory();
    void createsWritesAndReadsAMarker();
    void survivesReopen();
    void reportsAnErrorForAnUnusablePath();
};

void DatabaseTest::placesTheDatabaseUnderTheDataDirectory()
{
    const QString dataDir = QStringLiteral("/tmp/pikatalk-data");
    const QString path = phase0DatabasePath(dataDir);
    QVERIFY(path.startsWith(dataDir));
    QVERIFY(path.endsWith(QStringLiteral(".sqlite")));
}

void DatabaseTest::createsWritesAndReadsAMarker()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = phase0DatabasePath(tmp.path());

    QString error;
    QVERIFY2(initializePhase0Database(dbPath, &error), qUtf8Printable(error));
    QVERIFY(QFileInfo::exists(dbPath));
    QVERIFY2(writePhase0Marker(dbPath, QStringLiteral("probe"), QStringLiteral("ok"), &error), qUtf8Printable(error));

    QString value;
    QVERIFY2(readPhase0Marker(dbPath, QStringLiteral("probe"), &value, &error), qUtf8Printable(error));
    QCOMPARE(value, QStringLiteral("ok"));
}

void DatabaseTest::survivesReopen()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dbPath = phase0DatabasePath(tmp.path());
    QString error;
    QVERIFY2(initializePhase0Database(dbPath, &error), qUtf8Printable(error));
    QVERIFY2(writePhase0Marker(dbPath, QStringLiteral("probe"), QStringLiteral("persist"), &error), qUtf8Printable(error));

    QString value;
    QVERIFY2(readPhase0Marker(dbPath, QStringLiteral("probe"), &value, &error), qUtf8Printable(error));
    QCOMPARE(value, QStringLiteral("persist"));
}

void DatabaseTest::reportsAnErrorForAnUnusablePath()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString blockerPath = tmp.filePath(QStringLiteral("not-a-directory"));
    QFile blocker(blockerPath);
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.write("x");
    blocker.close();

    const QString dbPath = blockerPath + QStringLiteral("/phase0.sqlite");
    QString error;
    QVERIFY(!initializePhase0Database(dbPath, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(DatabaseTest)
#include "database_test.moc"
