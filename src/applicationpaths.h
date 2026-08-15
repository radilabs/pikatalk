#pragma once

#include <QString>

struct ApplicationPaths {
    QString data;
    QString config;
    QString cache;
};

ApplicationPaths resolveApplicationPaths();
bool ensureApplicationDirectories(const ApplicationPaths &paths, QString *error = nullptr);
