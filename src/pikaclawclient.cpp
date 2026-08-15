#include "pikaclawclient.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QRandomGenerator>
#include <QtEndian>

#include <cstring>

PicoClawClient::PicoClawClient(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_socket, &QTcpSocket::connected, this, &PicoClawClient::onConnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &PicoClawClient::onReadyRead);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &PicoClawClient::onSocketError);
    connect(&m_socket, &QTcpSocket::disconnected, this, &PicoClawClient::onDisconnected);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_wantConnected && m_state != QStringLiteral("connected")) {
            beginConnect();
        }
    });
}

PicoClawClient::~PicoClawClient()
{
    m_wantConnected = false;
    m_reconnectTimer.stop();
    m_socket.abort();
}

void PicoClawClient::setEndpoint(const QUrl &endpoint)
{
    m_endpoint = endpoint;
}

void PicoClawClient::setToken(const QString &token)
{
    m_token = token;
}

void PicoClawClient::setReconnectIntervalMs(int milliseconds)
{
    m_reconnectIntervalMs = milliseconds;
}

void PicoClawClient::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
    if (!enabled) {
        m_reconnectTimer.stop();
    }
}

QUrl PicoClawClient::endpoint() const
{
    return m_endpoint;
}

QString PicoClawClient::connectionState() const
{
    return m_state;
}

QString PicoClawClient::lastError() const
{
    return m_error;
}

void PicoClawClient::connectToGateway()
{
    m_wantConnected = true;
    m_reconnectTimer.stop();
    beginConnect();
}

void PicoClawClient::disconnectFromGateway()
{
    m_wantConnected = false;
    m_reconnectTimer.stop();
    m_socket.abort();
    setError(QString());
    setState(QStringLiteral("disconnected"));
}

void PicoClawClient::sendJson(const QJsonObject &object)
{
    if (!m_upgraded || m_socket.state() != QAbstractSocket::ConnectedState) {
        setError(QStringLiteral("not connected"));
        return;
    }
    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    m_socket.write(encodeMasked(0x1, payload));
    m_socket.flush();
}

QByteArray PicoClawClient::encodeMasked(int opcode, const QByteArray &payload) const
{
    QByteArray frame;
    frame.append(static_cast<char>(0x80 | opcode));
    const int n = payload.size();
    const quint64 n64 = static_cast<quint64>(n);
    QByteArray mask(4, Qt::Uninitialized);
    for (int i = 0; i < 4; ++i) {
        mask[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    if (n < 126) {
        frame.append(static_cast<char>(0x80 | n));
    } else if (n < 65536) {
        frame.append(static_cast<char>(0x80 | 126));
        const char len16[] = {static_cast<char>((n >> 8) & 0xff), static_cast<char>(n & 0xff)};
        frame.append(len16, 2);
    } else {
        frame.append(static_cast<char>(0x80 | 127));
        const char len64[] = {
            static_cast<char>((n64 >> 56) & 0xff),
            static_cast<char>((n64 >> 48) & 0xff),
            static_cast<char>((n64 >> 40) & 0xff),
            static_cast<char>((n64 >> 32) & 0xff),
            static_cast<char>((n64 >> 24) & 0xff),
            static_cast<char>((n64 >> 16) & 0xff),
            static_cast<char>((n64 >> 8) & 0xff),
            static_cast<char>(n64 & 0xff)};
        frame.append(len64, 8);
    }
    frame += mask;
    QByteArray masked = payload;
    for (int i = 0; i < masked.size(); ++i) {
        masked[i] = masked[i] ^ mask[i % 4];
    }
    frame += masked;
    return frame;
}

void PicoClawClient::setState(const QString &state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    Q_EMIT connectionStateChanged();
}

void PicoClawClient::setError(const QString &error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    Q_EMIT lastErrorChanged();
}

void PicoClawClient::beginConnect()
{
    m_upgraded = false;
    m_buffer.clear();
    m_fragment.clear();
    m_fragmentOpcode = 0;
    setState(QStringLiteral("connecting"));
    m_suppressSocketEvents = true;
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.abort();
    }
    m_suppressSocketEvents = false;
    const quint16 port = static_cast<quint16>(m_endpoint.port(80));
    m_socket.connectToHost(m_endpoint.host(), port);
}

void PicoClawClient::scheduleReconnect()
{
    if (!m_wantConnected || !m_autoReconnect || m_reconnectTimer.isActive()) {
        return;
    }
    m_reconnectTimer.start(m_reconnectIntervalMs);
}

void PicoClawClient::onConnected()
{
    sendHandshake();
}

void PicoClawClient::sendHandshake()
{
    QByteArray rawKey(16, Qt::Uninitialized);
    for (int i = 0; i < rawKey.size(); ++i) {
        rawKey[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    m_handshakeKey = rawKey.toBase64();
    QString path = m_endpoint.path();
    if (path.isEmpty()) {
        path = QStringLiteral("/");
    }
    if (m_endpoint.hasQuery()) {
        path += QLatin1Char('?') + m_endpoint.query();
    }
    const quint16 port = static_cast<quint16>(m_endpoint.port(80));
    QByteArray request;
    request += "GET ";
    request += path.toUtf8();
    request += " HTTP/1.1\r\n";
    request += "Host: ";
    request += m_endpoint.host().toUtf8();
    request += ':';
    request += QByteArray::number(port);
    request += "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: ";
    request += m_handshakeKey;
    request += "\r\nSec-WebSocket-Version: 13\r\n";
    if (!m_token.isEmpty()) {
        request += "Authorization: Bearer ";
        request += m_token.toUtf8();
        request += "\r\n";
    }
    request += "\r\n";
    m_socket.write(request);
}

void PicoClawClient::onReadyRead()
{
    m_buffer += m_socket.readAll();
    if (!m_upgraded) {
        if (!consumeHttpResponse()) {
            return;
        }
    }
    if (m_upgraded) {
        consumeFrames();
    }
}

bool PicoClawClient::consumeHttpResponse()
{
    const int headerEnd = m_buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return false;
    }
    const QByteArray header = m_buffer.left(headerEnd);
    m_buffer.remove(0, headerEnd + 4);
    const QByteArray statusLine = header.split('\n').value(0).trimmed();
    if (statusLine.contains(" 101")) {
        setError(QString());
        m_upgraded = true;
        setState(QStringLiteral("connected"));
        return true;
    }
    QString message = QString::fromUtf8(statusLine);
    if (statusLine.contains("401")) {
        message = QStringLiteral("401 unauthorized");
    }
    setError(message);
    setState(QStringLiteral("error"));
    m_wantConnected = m_autoReconnect && m_wantConnected;
    m_socket.abort();
    if (m_wantConnected) {
        scheduleReconnect();
    }
    return false;
}

quint16 PicoClawClient::readU16(const QByteArray &buffer, int offset)
{
    quint16 value = 0;
    memcpy(&value, buffer.constData() + offset, sizeof(value));
    return qFromBigEndian(value);
}

quint64 PicoClawClient::readU64(const QByteArray &buffer, int offset)
{
    quint64 value = 0;
    memcpy(&value, buffer.constData() + offset, sizeof(value));
    return qFromBigEndian(value);
}

void PicoClawClient::handleFrame(int opcode, bool fin, const QByteArray &payload)
{
    if (opcode == 0x8) {
        m_socket.disconnectFromHost();
        return;
    }
    if (opcode == 0x9) {
        m_socket.write(encodeMasked(0xA, payload));
        return;
    }
    if (opcode == 0xA) {
        return;
    }
    if (opcode != 0x0 && opcode != 0x1 && opcode != 0x2) {
        return;
    }
    QByteArray message = payload;
    int messageOpcode = opcode;
    if (opcode == 0x0) {
        if (m_fragmentOpcode == 0) {
            return;
        }
        m_fragment += payload;
        if (!fin) {
            return;
        }
        message = m_fragment;
        messageOpcode = m_fragmentOpcode;
        m_fragment.clear();
        m_fragmentOpcode = 0;
    } else if (!fin) {
        m_fragmentOpcode = opcode;
        m_fragment = payload;
        return;
    }
    if (messageOpcode != 0x1) {
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(message, &parseError);
    if (!document.isObject()) {
        return;
    }
    Q_EMIT messageReceived(document.object());
}

void PicoClawClient::consumeFrames()
{
    while (m_buffer.size() >= 2) {
        const bool fin = (static_cast<unsigned char>(m_buffer.at(0)) & 0x80) != 0;
        const int opcode = m_buffer.at(0) & 0x0f;
        const bool masked = (static_cast<unsigned char>(m_buffer.at(1)) & 0x80) != 0;
        quint64 length = static_cast<unsigned char>(m_buffer.at(1)) & 0x7f;
        int offset = 2;
        if (length == 126) {
            if (m_buffer.size() < offset + 2) {
                return;
            }
            length = readU16(m_buffer, offset);
            offset += 2;
        } else if (length == 127) {
            if (m_buffer.size() < offset + 8) {
                return;
            }
            length = readU64(m_buffer, offset);
            offset += 8;
        }
        if (masked) {
            if (m_buffer.size() < offset + 4) {
                return;
            }
            offset += 4;
        }
        if (length > static_cast<quint64>(m_buffer.size())
            || static_cast<quint64>(m_buffer.size()) < static_cast<quint64>(offset) + length) {
            return;
        }
        QByteArray payload = m_buffer.mid(offset, static_cast<int>(length));
        if (masked) {
            const QByteArray mask = m_buffer.mid(offset - 4, 4);
            for (int i = 0; i < payload.size(); ++i) {
                payload[i] = static_cast<char>(payload[i] ^ mask[i % 4]);
            }
        }
        m_buffer.remove(0, offset + static_cast<int>(length));
        handleFrame(opcode, fin, payload);
        if (opcode == 0x8) {
            return;
        }
    }
}

void PicoClawClient::onSocketError(QAbstractSocket::SocketError)
{
    if (m_suppressSocketEvents || !m_wantConnected) {
        return;
    }
    if (m_state == QStringLiteral("connected")) {
        return;
    }
    setError(m_socket.errorString());
    setState(QStringLiteral("error"));
    scheduleReconnect();
}

void PicoClawClient::onDisconnected()
{
    m_upgraded = false;
    m_buffer.clear();
    m_fragment.clear();
    m_fragmentOpcode = 0;
    if (m_suppressSocketEvents) {
        return;
    }
    if (!m_wantConnected) {
        if (m_state != QStringLiteral("error")) {
            setState(QStringLiteral("disconnected"));
        }
        return;
    }
    if (m_state != QStringLiteral("error")) {
        setError(QStringLiteral("connection lost"));
        setState(QStringLiteral("error"));
    }
    scheduleReconnect();
}
