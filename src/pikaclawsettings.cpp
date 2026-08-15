#include "pikaclawsettings.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

QUrl defaultPicoClawEndpoint()
{
    return QUrl(QStringLiteral("ws://127.0.0.1:18790/pico/ws"));
}

QString readPicoChannelToken(const QString &securityFilePath)
{
    QFile file(securityFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    bool inPico = false;
    bool inSettings = false;
    int picoIndent = -1;
    int settingsIndent = -1;
    for (const QString &rawLine : lines) {
        const QString withoutComment = rawLine.section(QLatin1Char('#'), 0, 0);
        if (withoutComment.trimmed().isEmpty()) {
            continue;
        }
        int indent = 0;
        while (indent < withoutComment.size() && withoutComment.at(indent).isSpace()) {
            ++indent;
        }
        const QString trimmed = withoutComment.trimmed();
        const QString key = trimmed.section(QLatin1Char(':'), 0, 0).trimmed();
        const QString value = trimmed.section(QLatin1Char(':'), 1).trimmed();
        if (indent == 2 && key == QLatin1String("pico")) {
            inPico = true;
            picoIndent = indent;
            inSettings = false;
            continue;
        }
        if (inPico && indent <= 2 && key != QLatin1String("pico")) {
            inPico = false;
            inSettings = false;
        }
        if (inPico && key == QLatin1String("settings") && indent > picoIndent) {
            inSettings = true;
            settingsIndent = indent;
            continue;
        }
        if (inPico && inSettings && key == QLatin1String("token") && indent > settingsIndent) {
            QString token = value;
            if ((token.startsWith(QLatin1Char('"')) && token.endsWith(QLatin1Char('"')))
                || (token.startsWith(QLatin1Char('\'')) && token.endsWith(QLatin1Char('\'')))) {
                token = token.mid(1, token.size() - 2);
            }
            return token;
        }
    }
    return {};
}

QStringList loadPicoClawModelNames(const QString &configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }
    const QJsonArray models = document.object().value(QStringLiteral("model_list")).toArray();
    QStringList names;
    for (const QJsonValue &value : models) {
        const QString name = value.toObject().value(QStringLiteral("model_name")).toString().trimmed();
        if (!name.isEmpty()) {
            names.append(name);
        }
    }
    return names;
}

QString loadPicoClawDefaultModelName(const QString &configPath)
{
    if (configPath.isEmpty()) {
        return {};
    }
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }
    return document.object()
        .value(QStringLiteral("agents"))
        .toObject()
        .value(QStringLiteral("defaults"))
        .toObject()
        .value(QStringLiteral("model_name"))
        .toString()
        .trimmed();
}

PicoClawConnectionSettings loadPicoClawConnectionSettings(const QString &pikaTalkConfigDirectory)
{
    PicoClawConnectionSettings settings;
    settings.endpoint = defaultPicoClawEndpoint();
    settings.picoConfigPath = QDir::homePath() + QStringLiteral("/.picoclaw/config.json");

    QSettings ini(QDir(pikaTalkConfigDirectory).filePath(QStringLiteral("pikatalk.conf")), QSettings::IniFormat);
    const QString endpoint = ini.value(QStringLiteral("picoClaw/endpoint")).toString().trimmed();
    if (!endpoint.isEmpty()) {
        settings.endpoint = QUrl(endpoint);
    }
    settings.token = ini.value(QStringLiteral("picoClaw/token")).toString();
    const QString configPath = ini.value(QStringLiteral("picoClaw/configPath")).toString().trimmed();
    if (!configPath.isEmpty()) {
        settings.picoConfigPath = configPath;
    }

    if (settings.token.isEmpty()) {
        const QString securityPath = QDir::homePath() + QStringLiteral("/.picoclaw/.security.yml");
        settings.token = readPicoChannelToken(securityPath);
    }
    return settings;
}
