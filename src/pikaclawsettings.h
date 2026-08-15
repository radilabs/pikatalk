#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>

struct PicoClawConnectionSettings {
    QUrl endpoint;
    QString token;
    QString picoConfigPath;
    QUrl launcherUrl;
    QString launcherPassword;
};

QUrl defaultPicoClawEndpoint();
QString readPicoChannelToken(const QString &securityFilePath);
QStringList loadPicoClawModelNames(const QString &configPath);
QString loadPicoClawDefaultModelName(const QString &configPath);
QString loadPicoClawDefaultWorkspace(const QString &configPath);
QString picoClawSessionsDirectory(const QString &configPath);
QHash<QString, QString> loadPicoClawToolResults(const QString &sessionsDirectory, const QString &sessionId);
PicoClawConnectionSettings loadPicoClawConnectionSettings(const QString &pikaTalkConfigDirectory);
