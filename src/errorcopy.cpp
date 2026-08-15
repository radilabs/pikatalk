#include "errorcopy.h"

#include <QHash>
#include <QRegularExpression>

ErrorCopy::ErrorCopy(QObject *parent)
    : QObject(parent)
{
}

QString ErrorCopy::sanitize(const QString &raw) const
{
    return sanitizeUserFacingError(raw);
}

QString sanitizeUserFacingError(const QString &raw)
{
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    if (trimmed.startsWith(QLatin1Char('{')) || trimmed.startsWith(QLatin1Char('['))) {
        return QStringLiteral("Request failed");
    }
    if (trimmed.contains(QStringLiteral("QAbstractSocket"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("WebSocket"), Qt::CaseInsensitive)
        || trimmed.contains(QStringLiteral("qt.network"), Qt::CaseInsensitive)) {
        return QStringLiteral("Connection failed");
    }

    static const QHash<QString, QString> known{
        {QStringLiteral("gateway unavailable"), QStringLiteral("Gateway unavailable")},
        {QStringLiteral("connection lost"), QStringLiteral("Connection lost")},
        {QStringLiteral("gateway error"), QStringLiteral("Request failed")},
    };
    const auto knownIt = known.constFind(trimmed.toLower());
    if (knownIt != known.cend()) {
        return knownIt.value();
    }

    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    QString compact = trimmed;
    compact.replace(whitespace, QStringLiteral(" "));
    if (compact.size() > 120) {
        compact = compact.left(117).trimmed();
        compact.append(QStringLiteral("…"));
    }
    return compact;
}
