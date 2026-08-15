#include "applicationpaths.h"

#include <QDir>
#include <QStandardPaths>

ApplicationPaths resolveApplicationPaths()
{
    ApplicationPaths paths;
    paths.data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    paths.config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    paths.cache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return paths;
}

bool ensureApplicationDirectories(const ApplicationPaths &paths, QString *error)
{
    const QString dirs[] = {paths.data, paths.config, paths.cache};
    for (const QString &dir : dirs) {
        if (dir.isEmpty() || !QDir().mkpath(dir)) {
            if (error != nullptr) {
                *error = QStringLiteral("Failed to create directory: %1").arg(dir);
            }
            return false;
        }
    }
    return true;
}
