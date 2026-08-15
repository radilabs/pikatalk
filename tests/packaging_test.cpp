#include <QFile>
#include <QtTest>

class PackagingTest : public QObject
{
    Q_OBJECT

private:
    static QString load(const QString &relativePath)
    {
        QFile file(QStringLiteral(PIKATALK_SOURCE_DIR "/") + relativePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        return QString::fromUtf8(file.readAll());
    }

private Q_SLOTS:
    void desktopFileUsesAppIdIconAndIdentity();
    void scalableIconExists();
    void metainfoDeclaresAppIdAndVersion();
    void cmakeInstallsDesktopIconAndMetainfo();
};

void PackagingTest::desktopFileUsesAppIdIconAndIdentity()
{
    const QString desktop = load(QStringLiteral("org.radilabs.pikatalk.desktop"));
    QVERIFY2(!desktop.isEmpty(), "org.radilabs.pikatalk.desktop must exist");
    QVERIFY(desktop.contains(QStringLiteral("Name=PikaTalk")));
    QVERIFY(desktop.contains(QStringLiteral("Exec=pikatalk")));
    QVERIFY(desktop.contains(QStringLiteral("Icon=org.radilabs.pikatalk")));
    QVERIFY(desktop.contains(QStringLiteral("StartupWMClass=pikatalk")));
    QVERIFY(!desktop.contains(QStringLiteral("Icon=internet-chat")));
    QVERIFY(!desktop.contains(QStringLiteral("Icon=utilities-terminal")));
}

void PackagingTest::scalableIconExists()
{
    const QString svg = load(QStringLiteral("icons/org.radilabs.pikatalk.svg"));
    QVERIFY2(!svg.isEmpty(), "icons/org.radilabs.pikatalk.svg must exist");
    QVERIFY(svg.contains(QStringLiteral("<svg")));
}

void PackagingTest::metainfoDeclaresAppIdAndVersion()
{
    const QString metainfo = load(QStringLiteral("org.radilabs.pikatalk.metainfo.xml"));
    QVERIFY2(!metainfo.isEmpty(), "org.radilabs.pikatalk.metainfo.xml must exist");
    QVERIFY(metainfo.contains(QStringLiteral("<id>org.radilabs.pikatalk</id>")));
    QVERIFY(metainfo.contains(QStringLiteral("type=\"desktop-application\"")));
    QVERIFY(metainfo.contains(QStringLiteral("<launchable type=\"desktop-id\">org.radilabs.pikatalk.desktop</launchable>")));
    QVERIFY(metainfo.contains(QStringLiteral("<release version=\"0.1.0\"")));
    QVERIFY(metainfo.contains(QStringLiteral("<binary>pikatalk</binary>")));
}

void PackagingTest::cmakeInstallsDesktopIconAndMetainfo()
{
    const QString cmake = load(QStringLiteral("CMakeLists.txt"));
    QVERIFY(cmake.contains(QStringLiteral("install(PROGRAMS org.radilabs.pikatalk.desktop DESTINATION ${KDE_INSTALL_APPDIR})")));
    QVERIFY(cmake.contains(QStringLiteral("org.radilabs.pikatalk.metainfo.xml")));
    QVERIFY(cmake.contains(QStringLiteral("${KDE_INSTALL_METAINFODIR}")));
    QVERIFY(cmake.contains(QStringLiteral("icons/org.radilabs.pikatalk.svg")));
    QVERIFY(cmake.contains(QStringLiteral("hicolor/scalable/apps")));
}

QTEST_GUILESS_MAIN(PackagingTest)
#include "packaging_test.moc"
