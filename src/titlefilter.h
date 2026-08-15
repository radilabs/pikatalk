#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

bool titleMatches(const QString &title, const QString &filter);
QVariantList filterItemsByTitle(const QVariantList &items, const QString &titleKey, const QString &filter);

class TitleMatch : public QObject
{
    Q_OBJECT

public:
    explicit TitleMatch(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList apply(const QVariantList &items, const QString &titleKey, const QString &filter) const;
};
