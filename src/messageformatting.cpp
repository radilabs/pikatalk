#include "messageformatting.h"

#include <QRegularExpression>
#include <QVariantMap>

QVariantList splitMessageSegments(const QString &content)
{
    QVariantList segments;
    static const QRegularExpression fence(QStringLiteral("```([^\\n]*)\\n([\\s\\S]*?)```"));
    int cursor = 0;
    auto it = fence.globalMatch(content);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int start = match.capturedStart();
        if (start > cursor) {
            QVariantMap text;
            text.insert(QStringLiteral("kind"), QStringLiteral("text"));
            text.insert(QStringLiteral("text"), content.mid(cursor, start - cursor));
            segments.append(text);
        }
        QVariantMap code;
        code.insert(QStringLiteral("kind"), QStringLiteral("code"));
        code.insert(QStringLiteral("language"), match.captured(1).trimmed());
        code.insert(QStringLiteral("text"), match.captured(2));
        segments.append(code);
        cursor = match.capturedEnd();
    }
    if (cursor < content.size() || segments.isEmpty()) {
        QVariantMap text;
        text.insert(QStringLiteral("kind"), QStringLiteral("text"));
        text.insert(QStringLiteral("text"), content.mid(cursor));
        segments.append(text);
    }
    return segments;
}
