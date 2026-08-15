#include "applicationidentity.h"

#include <QCoreApplication>

void configureApplicationIdentity()
{
    QCoreApplication::setOrganizationName(QStringLiteral("Radilabs"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("radilabs.org"));
    QCoreApplication::setApplicationName(QStringLiteral("PikaTalk"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PIKATALK_VERSION));
}
