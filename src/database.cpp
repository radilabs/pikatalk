#include "database.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

static QString uniqueConnectionName()
{
    return QStringLiteral("pikatalk-phase0-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

static void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

QString phase0DatabasePath(const QString &dataDirectory)
{
    return QDir(dataDirectory).filePath(QStringLiteral("phase0.sqlite"));
}

bool initializePhase0Database(const QString &filePath, QString *error)
{
    if (filePath.isEmpty()) {
        setError(error, QStringLiteral("Database path is empty"));
        return false;
    }

    const QFileInfo info(filePath);
    if (!QDir().mkpath(info.absolutePath())) {
        setError(error, QStringLiteral("Failed to create database directory: %1").arg(info.absolutePath()));
        return false;
    }

    const QString connectionName = uniqueConnectionName();
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(filePath);
        if (!db.open()) {
            setError(error, QStringLiteral("Failed to open database %1: %2").arg(filePath, db.lastError().text()));
        } else {
            QSqlQuery query(db);
            ok = query.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS phase0_init ("
                " key TEXT PRIMARY KEY NOT NULL,"
                " value TEXT NOT NULL"
                ")"));
            if (!ok) {
                setError(error, QStringLiteral("Failed to initialize database %1: %2").arg(filePath, query.lastError().text()));
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

bool writePhase0Marker(const QString &filePath, const QString &key, const QString &value, QString *error)
{
    const QString connectionName = uniqueConnectionName();
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(filePath);
        if (!db.open()) {
            setError(error, QStringLiteral("Failed to open database %1: %2").arg(filePath, db.lastError().text()));
        } else {
            QSqlQuery query(db);
            query.prepare(QStringLiteral("INSERT OR REPLACE INTO phase0_init(key, value) VALUES(?, ?)"));
            query.addBindValue(key);
            query.addBindValue(value);
            ok = query.exec();
            if (!ok) {
                setError(error, QStringLiteral("Failed to write database marker: %1").arg(query.lastError().text()));
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

bool readPhase0Marker(const QString &filePath, const QString &key, QString *value, QString *error)
{
    const QString connectionName = uniqueConnectionName();
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(filePath);
        if (!db.open()) {
            setError(error, QStringLiteral("Failed to open database %1: %2").arg(filePath, db.lastError().text()));
        } else {
            QSqlQuery query(db);
            query.prepare(QStringLiteral("SELECT value FROM phase0_init WHERE key = ?"));
            query.addBindValue(key);
            if (!query.exec()) {
                setError(error, QStringLiteral("Failed to read database marker: %1").arg(query.lastError().text()));
            } else if (!query.next()) {
                setError(error, QStringLiteral("Database marker not found: %1").arg(key));
            } else if (value != nullptr) {
                *value = query.value(0).toString();
                ok = true;
            } else {
                ok = true;
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}
