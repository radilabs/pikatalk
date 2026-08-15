#include "applicationpaths.h"
#include "database.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <KIconTheme>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <cstdio>

static QtMessageHandler previousMessageHandler = nullptr;

static void pikatalkMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    fprintf(stderr, "%s\n", qUtf8Printable(msg));
    if (previousMessageHandler != nullptr) {
        previousMessageHandler(type, context, msg);
    }
}

static void logApplicationPath(const char *label, const QString &path)
{
    qInfo().nospace() << label << ' ' << qUtf8Printable(path);
}

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    previousMessageHandler = qInstallMessageHandler(pikatalkMessageHandler);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("pikatalk");
    QApplication::setOrganizationName(QStringLiteral("Radilabs"));
    QApplication::setOrganizationDomain(QStringLiteral("radilabs.org"));
    QApplication::setApplicationName(QStringLiteral("PikaTalk"));
    QApplication::setDesktopFileName(QStringLiteral("org.radilabs.pikatalk"));

    const ApplicationPaths paths = resolveApplicationPaths();
    logApplicationPath("PikaTalk data directory:", paths.data);
    logApplicationPath("PikaTalk config directory:", paths.config);
    logApplicationPath("PikaTalk cache directory:", paths.cache);
    QString pathError;
    if (!ensureApplicationDirectories(paths, &pathError)) {
        qCritical() << pathError;
        return -1;
    }

    const QString dbPath = phase0DatabasePath(paths.data);
    QString dbError;
    if (!initializePhase0Database(dbPath, &dbError)
        || !writePhase0Marker(dbPath, QStringLiteral("initialized"), QStringLiteral("1"), &dbError)) {
        qCritical() << "SQLite initialization failed:" << dbError;
    } else {
        QString marker;
        if (!readPhase0Marker(dbPath, QStringLiteral("initialized"), &marker, &dbError)) {
            qCritical() << "SQLite read failed:" << dbError;
        } else {
            logApplicationPath("PikaTalk sqlite database:", dbPath);
            qInfo().nospace() << "PikaTalk sqlite marker: " << qUtf8Printable(marker);
        }
    }

    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("org.radilabs.pikatalk", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load the PikaTalk QML interface";
        return -1;
    }

    return app.exec();
}
