#pragma once

#include <QString>

QString phase0DatabasePath(const QString &dataDirectory);
bool initializePhase0Database(const QString &filePath, QString *error = nullptr);
bool writePhase0Marker(const QString &filePath, const QString &key, const QString &value, QString *error = nullptr);
bool readPhase0Marker(const QString &filePath, const QString &key, QString *value, QString *error = nullptr);
