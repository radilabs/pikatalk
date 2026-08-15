#include "applicationpaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

class ApplicationPathsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void resolvesAbsoluteXdgLocations();
    void respectsXdgEnvironmentOverrides();
    void canCreateDirectoriesWithoutPrivileges();
    void doesNotUseTheSourceRepository();
};

void ApplicationPathsTest::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("Radilabs"));
    QCoreApplication::setApplicationName(QStringLiteral("PikaTalk"));
}

void ApplicationPathsTest::resolvesAbsoluteXdgLocations()
{
    const auto paths = resolveApplicationPaths();
    QVERIFY2(QDir::isAbsolutePath(paths.data), qUtf8Printable(paths.data));
    QVERIFY2(QDir::isAbsolutePath(paths.config), qUtf8Printable(paths.config));
    QVERIFY2(QDir::isAbsolutePath(paths.cache), qUtf8Printable(paths.cache));
}

void ApplicationPathsTest::respectsXdgEnvironmentOverrides()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString dataHome = tmp.filePath(QStringLiteral("xdg-data"));
    const QString configHome = tmp.filePath(QStringLiteral("xdg-config"));
    const QString cacheHome = tmp.filePath(QStringLiteral("xdg-cache"));

    QVERIFY(qputenv("XDG_DATA_HOME", QFile::encodeName(dataHome)));
    QVERIFY(qputenv("XDG_CONFIG_HOME", QFile::encodeName(configHome)));
    QVERIFY(qputenv("XDG_CACHE_HOME", QFile::encodeName(cacheHome)));

    const auto paths = resolveApplicationPaths();
    QVERIFY2(paths.data.startsWith(dataHome), qUtf8Printable(paths.data));
    QVERIFY2(paths.config.startsWith(configHome), qUtf8Printable(paths.config));
    QVERIFY2(paths.cache.startsWith(cacheHome), qUtf8Printable(paths.cache));
}

void ApplicationPathsTest::canCreateDirectoriesWithoutPrivileges()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(qputenv("XDG_DATA_HOME", QFile::encodeName(tmp.filePath(QStringLiteral("xdg-data")))));
    QVERIFY(qputenv("XDG_CONFIG_HOME", QFile::encodeName(tmp.filePath(QStringLiteral("xdg-config")))));
    QVERIFY(qputenv("XDG_CACHE_HOME", QFile::encodeName(tmp.filePath(QStringLiteral("xdg-cache")))));

    const auto paths = resolveApplicationPaths();
    QString error;
    QVERIFY2(ensureApplicationDirectories(paths, &error), qUtf8Printable(error));
    QVERIFY(QFileInfo::exists(paths.data));
    QVERIFY(QFileInfo::exists(paths.config));
    QVERIFY(QFileInfo::exists(paths.cache));
}

void ApplicationPathsTest::doesNotUseTheSourceRepository()
{
    const auto paths = resolveApplicationPaths();
    const QString repo = QStringLiteral(PIKATALK_SOURCE_DIR);
    QVERIFY(!paths.data.startsWith(repo));
    QVERIFY(!paths.config.startsWith(repo));
    QVERIFY(!paths.cache.startsWith(repo));
}

QTEST_GUILESS_MAIN(ApplicationPathsTest)
#include "applicationpaths_test.moc"

