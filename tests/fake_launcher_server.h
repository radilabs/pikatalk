#pragma once

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

class FakeLauncherServer : public QObject
{
public:
    explicit FakeLauncherServer(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, &FakeLauncherServer::onNewConnection);
    }

    bool listen()
    {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    QUrl baseUrl() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort()));
    }

    void setPassword(const QString &password) { m_password = password; }
    void setGatewayStatus(const QString &status) { m_gatewayStatus = status; }
    void setGatewayVersion(const QString &version) { m_gatewayVersion = version; }
    void setStartAllowed(bool allowed) { m_startAllowed = allowed; }
    void setStartReason(const QString &reason) { m_startReason = reason; }
    void setForceUnauthorized(bool value) { m_forceUnauthorized = value; }
    void setCommandFailStatus(int status, const QByteArray &body)
    {
        m_failStatus = status;
        m_failBody = body;
    }

    int startCount() const { return m_startCount; }
    int stopCount() const { return m_stopCount; }
    int restartCount() const { return m_restartCount; }

private:
    void onNewConnection()
    {
        while (QTcpSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { onReadyRead(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    void onReadyRead(QTcpSocket *socket)
    {
        m_buffers[socket].append(socket->readAll());
        QByteArray &buffer = m_buffers[socket];
        const int headerEnd = buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const QByteArray header = buffer.left(headerEnd);
        const QList<QByteArray> lines = header.split('\n');
        if (lines.isEmpty()) {
            return;
        }
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        if (requestLine.size() < 2) {
            return;
        }
        const QByteArray method = requestLine.at(0);
        const QByteArray path = requestLine.at(1);
        int contentLength = 0;
        QByteArray cookie;
        for (const QByteArray &rawLine : lines) {
            const QByteArray line = rawLine.trimmed();
            if (line.toLower().startsWith("content-length:")) {
                contentLength = line.mid(15).trimmed().toInt();
            } else if (line.toLower().startsWith("cookie:")) {
                cookie = line.mid(7).trimmed();
            }
        }
        if (buffer.size() < headerEnd + 4 + contentLength) {
            return;
        }
        const QByteArray body = buffer.mid(headerEnd + 4, contentLength);
        buffer.remove(0, headerEnd + 4 + contentLength);
        handle(socket, method, path, cookie, body);
    }

    bool authed(const QByteArray &cookie) const
    {
        const QByteArray needle = QByteArrayLiteral("picoclaw_launcher_auth=") + m_session.toUtf8();
        return !m_forceUnauthorized && cookie.contains(needle);
    }

    void reply(QTcpSocket *socket, int status, const QByteArray &body, const QByteArray &extraHeaders = {})
    {
        QByteArray reason = status == 200 ? QByteArrayLiteral("OK")
                          : status == 400 ? QByteArrayLiteral("Bad Request")
                          : status == 401 ? QByteArrayLiteral("Unauthorized")
                                          : QByteArrayLiteral("Error");
        QByteArray response;
        response += "HTTP/1.1 " + QByteArray::number(status) + ' ' + reason + "\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        response += "Connection: close\r\n";
        if (!extraHeaders.isEmpty()) {
            response += extraHeaders;
        }
        response += "\r\n";
        response += body;
        socket->write(response);
        socket->disconnectFromHost();
    }

    void handle(QTcpSocket *socket,
                const QByteArray &method,
                const QByteArray &path,
                const QByteArray &cookie,
                const QByteArray &body)
    {
        if (method == "POST" && path == "/api/auth/login") {
            if (!body.contains(m_password.toUtf8())) {
                reply(socket, 401, QByteArrayLiteral("{\"error\":\"invalid password\"}"));
                return;
            }
            m_session = QStringLiteral("test-session-cookie");
            reply(socket,
                  200,
                  QByteArrayLiteral("{\"status\":\"ok\"}"),
                  "Set-Cookie: picoclaw_launcher_auth=" + m_session.toUtf8()
                      + "; Path=/; HttpOnly; SameSite=Lax\r\n");
            return;
        }
        if (!authed(cookie)
            && (path.startsWith("/api/gateway") || path == "/api/system/version")) {
            reply(socket, 401, QByteArrayLiteral("{\"error\":\"unauthorized\"}"));
            return;
        }
        if (method == "GET" && path == "/api/gateway/status") {
            QByteArray json = QByteArrayLiteral("{\"gateway_status\":\"") + m_gatewayStatus.toUtf8()
                + QByteArrayLiteral("\",\"gateway_start_allowed\":")
                + (m_startAllowed ? QByteArrayLiteral("true") : QByteArrayLiteral("false"));
            if (!m_startReason.isEmpty()) {
                json += QByteArrayLiteral(",\"gateway_start_reason\":\"") + m_startReason.toUtf8()
                    + '"';
            }
            if (m_gatewayStatus == QLatin1String("running")) {
                json += QByteArrayLiteral(",\"gateway_version\":\"") + m_gatewayVersion.toUtf8()
                    + QByteArrayLiteral("\",\"pid\":42");
            }
            json += '}';
            reply(socket, 200, json);
            return;
        }
        if (method == "GET" && path == "/api/system/version") {
            reply(socket,
                  200,
                  QByteArrayLiteral(
                      "{\"version\":\"0.3.1\",\"git_commit\":\"2cf030d2\",\"build_time\":\"t\"}"));
            return;
        }
        if (method == "POST" && path.startsWith("/api/gateway/") && m_failStatus > 0) {
            reply(socket, m_failStatus, m_failBody);
            return;
        }
        if (method == "POST" && path == "/api/gateway/start") {
            ++m_startCount;
            m_gatewayStatus = QStringLiteral("running");
            reply(socket, 200, QByteArrayLiteral("{\"status\":\"ok\",\"pid\":42}"));
            return;
        }
        if (method == "POST" && path == "/api/gateway/stop") {
            ++m_stopCount;
            m_gatewayStatus = QStringLiteral("stopped");
            reply(socket, 200, QByteArrayLiteral("{\"status\":\"ok\",\"pid\":42}"));
            return;
        }
        if (method == "POST" && path == "/api/gateway/restart") {
            ++m_restartCount;
            m_gatewayStatus = QStringLiteral("running");
            reply(socket, 200, QByteArrayLiteral("{\"status\":\"ok\",\"pid\":43}"));
            return;
        }
        reply(socket, 404, QByteArrayLiteral("{\"error\":\"not found\"}"));
    }

    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QString m_password = QStringLiteral("secret");
    QString m_session;
    QString m_gatewayStatus = QStringLiteral("stopped");
    QString m_gatewayVersion = QStringLiteral("0.3.1");
    bool m_startAllowed = true;
    QString m_startReason;
    bool m_forceUnauthorized = false;
    int m_failStatus = 0;
    QByteArray m_failBody;
    int m_startCount = 0;
    int m_stopCount = 0;
    int m_restartCount = 0;
};
