#include "appcontroller.h"
#include "applicationidentity.h"
#include "applicationpaths.h"
#include "database.h"
#include "errorcopy.h"
#include "titlefilter.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
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
    configureApplicationIdentity();
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

    const QString dbPath = LocalDatabase::databaseFilePath(paths.data);
    auto *controller = new AppController(&app);
    QString dbError;
    if (!controller->openStore(dbPath, &dbError)) {
        qCritical() << "SQLite initialization failed:" << dbError;
    } else {
        logApplicationPath("PikaTalk sqlite database:", dbPath);
    }

    controller->loadGatewaySettings(paths.config);
    logApplicationPath("PikaTalk PikaClaw endpoint:", controller->gatewayEndpoint().toString());
    controller->connectToGateway();

    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("app"), controller);
    engine.rootContext()->setContextProperty(QStringLiteral("titleMatch"), new TitleMatch(&app));
    engine.rootContext()->setContextProperty(QStringLiteral("errorCopy"), new ErrorCopy(&app));
    engine.loadFromModule("org.radilabs.pikatalk", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load the PikaTalk QML interface";
        return -1;
    }

    return app.exec();
}
