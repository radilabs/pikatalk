#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QHash>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

#include <QtEndian>

class FakePicoServer : public QObject
{
public:
    explicit FakePicoServer(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(&m_server, &QTcpServer::newConnection, this, &FakePicoServer::onNewConnection);
    }

    ~FakePicoServer()
    {
        m_server.close();
        const QList<QTcpSocket *> sockets = m_sockets;
        m_sockets.clear();
        m_buffers.clear();
        m_upgraded.clear();
        for (QTcpSocket *socket : sockets) {
            socket->disconnect();
            socket->setParent(nullptr);
            socket->abort();
            delete socket;
        }
    }

    void setRequiredToken(const QString &token)
    {
        m_requiredToken = token;
    }

    bool listen()
    {
        return m_server.listen(QHostAddress::LocalHost, m_preferredPort);
    }

    void setPreferredPort(quint16 port)
    {
        m_preferredPort = port;
    }

    quint16 port() const
    {
        return m_server.serverPort();
    }

    QUrl wsUrl() const
    {
        return QUrl(QStringLiteral("ws://127.0.0.1:%1/pico/ws").arg(port()));
    }

    void closeAll()
    {
        const QList<QTcpSocket *> sockets = m_sockets;
        m_sockets.clear();
        m_buffers.clear();
        m_upgraded.clear();
        for (QTcpSocket *socket : sockets) {
            socket->disconnect();
            socket->setParent(nullptr);
            socket->abort();
            socket->deleteLater();
        }
    }

    void stopListening()
    {
        m_preferredPort = m_server.serverPort();
        m_server.close();
        closeAll();
    }

    int connectionCount() const
    {
        return m_sockets.size();
    }

    QList<QJsonObject> clientMessages() const
    {
        return m_clientMessages;
    }

    void sendJson(const QJsonObject &object)
    {
        const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
        const QByteArray frame = encodeUnmaskedText(payload);
        for (QTcpSocket *socket : m_sockets) {
            if (m_upgraded.value(socket, false)) {
                socket->write(frame);
            }
        }
    }

private:
    void onNewConnection()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *socket = m_server.nextPendingConnection();
            m_sockets.append(socket);
            m_buffers.insert(socket, QByteArray());
            m_upgraded.insert(socket, false);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                onReadyRead(socket);
            });
            connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
                m_sockets.removeAll(socket);
                m_buffers.remove(socket);
                m_upgraded.remove(socket);
            });
        }
    }

    void onReadyRead(QTcpSocket *socket)
    {
        m_buffers[socket] += socket->readAll();
        if (!m_upgraded.value(socket, false)) {
            const int headerEnd = m_buffers[socket].indexOf("\r\n\r\n");
            if (headerEnd < 0) {
                return;
            }
            const QByteArray header = m_buffers[socket].left(headerEnd);
            m_buffers[socket].remove(0, headerEnd + 4);
            if (!completeHandshake(socket, header)) {
                socket->disconnectFromHost();
                return;
            }
            m_upgraded[socket] = true;
        }
        while (tryConsumeFrame(socket)) {
        }
    }

    bool completeHandshake(QTcpSocket *socket, const QByteArray &header)
    {
        const QString auth = headerValue(header, QByteArrayLiteral("Authorization"));
        if (!m_requiredToken.isEmpty()) {
            const QString expected = QStringLiteral("Bearer %1").arg(m_requiredToken);
            if (auth != expected) {
                socket->write("HTTP/1.1 401 Unauthorized\r\nContent-Length: 13\r\n\r\nunauthorized");
                socket->flush();
                return false;
            }
        }
        const QString key = headerValue(header, QByteArrayLiteral("Sec-WebSocket-Key"));
        if (key.isEmpty()) {
            socket->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
            socket->flush();
            return false;
        }
        const QByteArray accept = QCryptographicHash::hash(
                                        QByteArray(key.toUtf8() + QByteArrayLiteral("258EAFA5-E914-47DA-95CA-C5AB0DC85B11")),
                                        QCryptographicHash::Sha1)
                                        .toBase64();
        const QByteArray response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: "
            + accept + "\r\n\r\n";
        socket->write(response);
        socket->flush();
        return true;
    }

    static QString headerValue(const QByteArray &header, const QByteArray &name)
    {
        const QList<QByteArray> lines = header.split('\n');
        for (QByteArray line : lines) {
            if (line.endsWith('\r')) {
                line.chop(1);
            }
            QByteArray prefix = name;
            prefix += QByteArrayLiteral(": ");
            if (line.startsWith(prefix)) {
                return QString::fromUtf8(line.mid(prefix.size()));
            }
        }
        return {};
    }

    bool tryConsumeFrame(QTcpSocket *socket)
    {
        QByteArray &buf = m_buffers[socket];
        if (buf.size() < 2) {
            return false;
        }
        const int opcode = buf.at(0) & 0x0f;
        const bool masked = (static_cast<unsigned char>(buf.at(1)) & 0x80) != 0;
        quint64 length = static_cast<unsigned char>(buf.at(1)) & 0x7f;
        int offset = 2;
        if (length == 126) {
            if (buf.size() < offset + 2) {
                return false;
            }
            length = qFromBigEndian<quint16>(buf.mid(offset, 2).constData());
            offset += 2;
        } else if (length == 127) {
            if (buf.size() < offset + 8) {
                return false;
            }
            length = qFromBigEndian<quint64>(buf.mid(offset, 8).constData());
            offset += 8;
        }
        QByteArray mask;
        if (masked) {
            if (buf.size() < offset + 4) {
                return false;
            }
            mask = buf.mid(offset, 4);
            offset += 4;
        }
        if (static_cast<quint64>(buf.size()) < offset + length) {
            return false;
        }
        QByteArray payload = buf.mid(offset, static_cast<int>(length));
        buf.remove(0, offset + static_cast<int>(length));
        if (masked) {
            for (int i = 0; i < payload.size(); ++i) {
                payload[i] = payload[i] ^ mask[i % 4];
            }
        }
        if (opcode == 0x8) {
            socket->disconnectFromHost();
            return !buf.isEmpty();
        }
        if (opcode == 0x9) {
            socket->write(encodeUnmasked(0xA, payload));
            return !buf.isEmpty();
        }
        if (opcode == 0x1) {
            const QJsonObject object = QJsonDocument::fromJson(payload).object();
            m_clientMessages.append(object);
        }
        return !buf.isEmpty();
    }

    static QByteArray encodeUnmaskedText(const QByteArray &payload)
    {
        return encodeUnmasked(0x1, payload);
    }

    static QByteArray encodeUnmasked(int opcode, const QByteArray &payload)
    {
        QByteArray frame;
        frame.append(static_cast<char>(0x80 | opcode));
        const int n = payload.size();
        if (n < 126) {
            frame.append(static_cast<char>(n));
        } else if (n < 65536) {
            frame.append(static_cast<char>(126));
            quint16 v = qToBigEndian(static_cast<quint16>(n));
            frame.append(reinterpret_cast<const char *>(&v), 2);
        } else {
            frame.append(static_cast<char>(127));
            quint64 v = qToBigEndian(static_cast<quint64>(n));
            frame.append(reinterpret_cast<const char *>(&v), 8);
        }
        frame.append(payload);
        return frame;
    }

    QTcpServer m_server;
    QList<QTcpSocket *> m_sockets;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QHash<QTcpSocket *, bool> m_upgraded;
    QList<QJsonObject> m_clientMessages;
    QString m_requiredToken;
    quint16 m_preferredPort = 0;
};
