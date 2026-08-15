#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

class PicoClawClient : public QObject
{
    Q_OBJECT

public:
    explicit PicoClawClient(QObject *parent = nullptr);
    ~PicoClawClient() override;

    void setEndpoint(const QUrl &endpoint);
    void setToken(const QString &token);
    void setReconnectIntervalMs(int milliseconds);
    void setAutoReconnect(bool enabled);

    QUrl endpoint() const;
    QString connectionState() const;
    QString lastError() const;

    void connectToGateway();
    void disconnectFromGateway();
    void sendJson(const QJsonObject &object);

Q_SIGNALS:
    void connectionStateChanged();
    void lastErrorChanged();
    void messageReceived(const QJsonObject &object);

private:
    void setState(const QString &state);
    void setError(const QString &error);
    void beginConnect();
    void scheduleReconnect();
    void sendHandshake();
    void onConnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onDisconnected();
    bool consumeHttpResponse();
    void consumeFrames();
    void handleFrame(int opcode, bool fin, const QByteArray &payload);
    QByteArray encodeMasked(int opcode, const QByteArray &payload) const;
    static quint16 readU16(const QByteArray &buffer, int offset);
    static quint64 readU64(const QByteArray &buffer, int offset);

    QTcpSocket m_socket;
    QTimer m_reconnectTimer;
    QUrl m_endpoint;
    QString m_token;
    QString m_state = QStringLiteral("disconnected");
    QString m_error;
    QByteArray m_buffer;
    QByteArray m_handshakeKey;
    QByteArray m_fragment;
    int m_fragmentOpcode = 0;
    int m_reconnectIntervalMs = 2000;
    bool m_autoReconnect = true;
    bool m_wantConnected = false;
    bool m_upgraded = false;
    bool m_suppressSocketEvents = false;
};
