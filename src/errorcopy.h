#pragma once

#include <QObject>
#include <QString>

QString sanitizeUserFacingError(const QString &raw);

class ErrorCopy : public QObject
{
    Q_OBJECT

public:
    explicit ErrorCopy(QObject *parent = nullptr);

    Q_INVOKABLE QString sanitize(const QString &raw) const;
};
