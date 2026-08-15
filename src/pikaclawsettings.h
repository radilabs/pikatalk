#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>

struct PicoClawConnectionSettings {
    QUrl endpoint;
    QString token;
    QString picoConfigPath;
};

QUrl defaultPicoClawEndpoint();
QString readPicoChannelToken(const QString &securityFilePath);
QStringList loadPicoClawModelNames(const QString &configPath);
QString loadPicoClawDefaultModelName(const QString &configPath);
PicoClawConnectionSettings loadPicoClawConnectionSettings(const QString &pikaTalkConfigDirectory);
