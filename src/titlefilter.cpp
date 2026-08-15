#include "titlefilter.h"

#include <QVariantMap>

TitleMatch::TitleMatch(QObject *parent)
    : QObject(parent)
{
}

QVariantList TitleMatch::apply(const QVariantList &items, const QString &titleKey, const QString &filter) const
{
    return filterItemsByTitle(items, titleKey, filter);
}

bool titleMatches(const QString &title, const QString &filter)
{
    const QString needle = filter.trimmed();
    if (needle.isEmpty()) {
        return true;
    }
    return title.contains(needle, Qt::CaseInsensitive);
}

QVariantList filterItemsByTitle(const QVariantList &items, const QString &titleKey, const QString &filter)
{
    QVariantList matched;
    for (const QVariant &item : items) {
        const QString title = item.toMap().value(titleKey).toString();
        if (titleMatches(title, filter)) {
            matched.append(item);
        }
    }
    return matched;
}
