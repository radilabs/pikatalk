#pragma once

#include <QString>
#include <QStringList>

struct WorkspaceLaunchResult {
    bool ok = false;
    QString error;
    QString command;
    QStringList arguments;
    QString workingDirectory;
};

WorkspaceLaunchResult prepareOpenWorkspaceInFileManager(const QString &workspacePath);
WorkspaceLaunchResult prepareOpenWorkspaceInTerminal(const QString &workspacePath,
                                                     const QString &terminalCommand = QString());
WorkspaceLaunchResult prepareOpenWorkspaceInEditor(const QString &workspacePath,
                                                   const QString &editorCommand = QString());
bool launchPreparedWorkspaceAction(const WorkspaceLaunchResult &prepared, QString *error = nullptr);
