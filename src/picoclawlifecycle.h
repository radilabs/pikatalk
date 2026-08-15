#pragma once

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QObject>
#include <QString>
#include <QUrl>

struct PicoClawLifecycleStatus {
    QString gatewayStatus;
    QString gatewayVersion;
    qint64 pid = 0;
    bool startAllowed = true;
    QString startReason;
    bool restartRequired = false;
    QString configDefaultModel;
    QString error;
    bool ok = false;
};

struct PicoClawLifecycleVersion {
    QString version;
    QString gitCommit;
    QString buildTime;
    QString error;
    bool ok = false;
};

struct PicoClawLifecycleCommandResult {
    QString status;
    qint64 pid = 0;
    QString message;
    QString error;
    bool ok = false;
};

class PicoClawLifecycleClient : public QObject
{
    Q_OBJECT

public:
    explicit PicoClawLifecycleClient(QObject *parent = nullptr);

    void setBaseUrl(const QUrl &baseUrl);
    QUrl baseUrl() const;

    void setPassword(const QString &password);
    QString password() const;

    void setSessionCookie(const QNetworkCookie &cookie);
    QNetworkCookie sessionCookie() const;
    bool hasSession() const;

    void login();
    void refreshStatus();
    void refreshVersion();
    void startGateway();
    void stopGateway();
    void restartGateway();

Q_SIGNALS:
    void loginFinished(bool ok, const QString &error);
    void statusFinished(const PicoClawLifecycleStatus &status);
    void versionFinished(const PicoClawLifecycleVersion &version);
    void commandFinished(const QString &command, const PicoClawLifecycleCommandResult &result);

private:
    QNetworkRequest authedRequest(const QString &path) const;
    void postCommand(const QString &command, const QString &path);

    QNetworkAccessManager m_nam;
    QUrl m_baseUrl;
    QString m_password;
    QNetworkCookie m_sessionCookie;
};
