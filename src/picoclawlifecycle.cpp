#include "picoclawlifecycle.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

QString jsonErrorMessage(const QJsonObject &object, const QByteArray &raw, int statusCode)
{
    const QString fromJson = object.value(QStringLiteral("error")).toString();
    if (!fromJson.isEmpty()) {
        return fromJson;
    }
    const QString message = object.value(QStringLiteral("message")).toString();
    if (!message.isEmpty()) {
        return message;
    }
    if (!raw.isEmpty()) {
        return QString::fromUtf8(raw);
    }
    return QStringLiteral("HTTP %1").arg(statusCode);
}

} // namespace

PicoClawLifecycleClient::PicoClawLifecycleClient(QObject *parent)
    : QObject(parent)
{
    m_baseUrl = QUrl(QStringLiteral("http://127.0.0.1:18800"));
    m_nam.setCookieJar(new QNetworkCookieJar(&m_nam));
}

void PicoClawLifecycleClient::setBaseUrl(const QUrl &baseUrl)
{
    m_baseUrl = baseUrl;
}

QUrl PicoClawLifecycleClient::baseUrl() const
{
    return m_baseUrl;
}

void PicoClawLifecycleClient::setPassword(const QString &password)
{
    m_password = password;
}

QString PicoClawLifecycleClient::password() const
{
    return m_password;
}

void PicoClawLifecycleClient::setSessionCookie(const QNetworkCookie &cookie)
{
    m_sessionCookie = cookie;
}

QNetworkCookie PicoClawLifecycleClient::sessionCookie() const
{
    return m_sessionCookie;
}

bool PicoClawLifecycleClient::hasSession() const
{
    return !m_sessionCookie.value().isEmpty();
}

QNetworkRequest PicoClawLifecycleClient::authedRequest(const QString &path) const
{
    QUrl url = m_baseUrl;
    url.setPath(path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_sessionCookie.value().isEmpty()) {
        request.setRawHeader(QByteArrayLiteral("Cookie"),
                             m_sessionCookie.name() + '=' + m_sessionCookie.value());
    }
    return request;
}

void PicoClawLifecycleClient::login()
{
    QNetworkRequest request = authedRequest(QStringLiteral("/api/auth/login"));
    const QJsonObject body{{QStringLiteral("password"), m_password}};
    QNetworkReply *reply =
        m_nam.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && status == 0) {
            Q_EMIT loginFinished(false, reply->errorString());
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(raw).object();
        if (status < 200 || status >= 300) {
            Q_EMIT loginFinished(false, jsonErrorMessage(object, raw, status));
            return;
        }
        const QList<QNetworkCookie> cookies =
            QNetworkCookie::parseCookies(reply->rawHeader(QByteArrayLiteral("Set-Cookie")));
        for (const QNetworkCookie &cookie : cookies) {
            if (cookie.name() == QByteArrayLiteral("picoclaw_launcher_auth")) {
                m_sessionCookie = cookie;
                break;
            }
        }
        if (m_sessionCookie.value().isEmpty()) {
            Q_EMIT loginFinished(false, QStringLiteral("login succeeded without session cookie"));
            return;
        }
        Q_EMIT loginFinished(true, QString());
    });
}

void PicoClawLifecycleClient::refreshStatus()
{
    QNetworkReply *reply = m_nam.get(authedRequest(QStringLiteral("/api/gateway/status")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        PicoClawLifecycleStatus status;
        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpStatus == 0) {
            status.error = reply->errorString();
            Q_EMIT statusFinished(status);
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(raw).object();
        if (httpStatus < 200 || httpStatus >= 300) {
            status.error = jsonErrorMessage(object, raw, httpStatus);
            Q_EMIT statusFinished(status);
            return;
        }
        status.ok = true;
        status.gatewayStatus = object.value(QStringLiteral("gateway_status")).toString();
        status.gatewayVersion = object.value(QStringLiteral("gateway_version")).toString();
        status.pid = object.value(QStringLiteral("pid")).toVariant().toLongLong();
        status.startAllowed = object.value(QStringLiteral("gateway_start_allowed")).toBool(true);
        status.startReason = object.value(QStringLiteral("gateway_start_reason")).toString();
        status.restartRequired = object.value(QStringLiteral("gateway_restart_required")).toBool(false);
        status.configDefaultModel = object.value(QStringLiteral("config_default_model")).toString();
        Q_EMIT statusFinished(status);
    });
}

void PicoClawLifecycleClient::refreshVersion()
{
    QNetworkReply *reply = m_nam.get(authedRequest(QStringLiteral("/api/system/version")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        PicoClawLifecycleVersion version;
        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpStatus == 0) {
            version.error = reply->errorString();
            Q_EMIT versionFinished(version);
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(raw).object();
        if (httpStatus < 200 || httpStatus >= 300) {
            version.error = jsonErrorMessage(object, raw, httpStatus);
            Q_EMIT versionFinished(version);
            return;
        }
        version.ok = true;
        version.version = object.value(QStringLiteral("version")).toString();
        version.gitCommit = object.value(QStringLiteral("git_commit")).toString();
        version.buildTime = object.value(QStringLiteral("build_time")).toString();
        Q_EMIT versionFinished(version);
    });
}

void PicoClawLifecycleClient::postCommand(const QString &command, const QString &path)
{
    QNetworkReply *reply = m_nam.post(authedRequest(path), QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, command]() {
        reply->deleteLater();
        PicoClawLifecycleCommandResult result;
        const QByteArray raw = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpStatus == 0) {
            result.error = reply->errorString();
            Q_EMIT commandFinished(command, result);
            return;
        }
        const QJsonObject object = QJsonDocument::fromJson(raw).object();
        if (httpStatus < 200 || httpStatus >= 300) {
            result.error = jsonErrorMessage(object, raw, httpStatus);
            result.status = object.value(QStringLiteral("status")).toString();
            result.message = object.value(QStringLiteral("message")).toString();
            Q_EMIT commandFinished(command, result);
            return;
        }
        result.ok = true;
        result.status = object.value(QStringLiteral("status")).toString();
        result.pid = object.value(QStringLiteral("pid")).toVariant().toLongLong();
        result.message = object.value(QStringLiteral("message")).toString();
        Q_EMIT commandFinished(command, result);
    });
}

void PicoClawLifecycleClient::startGateway()
{
    postCommand(QStringLiteral("start"), QStringLiteral("/api/gateway/start"));
}

void PicoClawLifecycleClient::stopGateway()
{
    postCommand(QStringLiteral("stop"), QStringLiteral("/api/gateway/stop"));
}

void PicoClawLifecycleClient::restartGateway()
{
    postCommand(QStringLiteral("restart"), QStringLiteral("/api/gateway/restart"));
}
