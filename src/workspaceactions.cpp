#include "workspaceactions.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace {

WorkspaceLaunchResult invalidWorkspace(const QString &workspacePath)
{
    WorkspaceLaunchResult result;
    if (workspacePath.trimmed().isEmpty()) {
        result.error = QStringLiteral("No active workspace is set");
        return result;
    }
    const QFileInfo info(workspacePath);
    if (!info.exists() || !info.isDir()) {
        result.error = QStringLiteral("Workspace path is not a usable local directory: %1").arg(workspacePath);
        return result;
    }
    result.ok = true;
    result.workingDirectory = info.absoluteFilePath();
    return result;
}

} // namespace

WorkspaceLaunchResult prepareOpenWorkspaceInFileManager(const QString &workspacePath)
{
    WorkspaceLaunchResult result = invalidWorkspace(workspacePath);
    if (!result.ok) {
        return result;
    }
    result.command = QStringLiteral("xdg-open");
    result.arguments = QStringList{result.workingDirectory};
    return result;
}

WorkspaceLaunchResult prepareOpenWorkspaceInTerminal(const QString &workspacePath, const QString &terminalCommand)
{
    WorkspaceLaunchResult result = invalidWorkspace(workspacePath);
    if (!result.ok) {
        return result;
    }
    const QString terminal = terminalCommand.trimmed().isEmpty() ? QStringLiteral("konsole") : terminalCommand.trimmed();
    result.command = terminal;
    if (terminal.endsWith(QStringLiteral("konsole")) || terminal == QStringLiteral("konsole")) {
        result.arguments = QStringList{QStringLiteral("--workdir"), result.workingDirectory};
    } else if (terminal.contains(QStringLiteral("gnome-terminal"))) {
        result.arguments = QStringList{QStringLiteral("--working-directory"), result.workingDirectory};
    } else {
        result.arguments = QStringList{QStringLiteral("--workdir"), result.workingDirectory};
    }
    return result;
}

WorkspaceLaunchResult prepareOpenWorkspaceInEditor(const QString &workspacePath, const QString &editorCommand)
{
    WorkspaceLaunchResult result = invalidWorkspace(workspacePath);
    if (!result.ok) {
        return result;
    }
    const QString editor = editorCommand.trimmed().isEmpty() ? QStringLiteral("kate") : editorCommand.trimmed();
    result.command = editor;
    result.arguments = QStringList{result.workingDirectory};
    return result;
}

bool launchPreparedWorkspaceAction(const WorkspaceLaunchResult &prepared, QString *error)
{
    if (!prepared.ok) {
        if (error) {
            *error = prepared.error;
        }
        return false;
    }
    if (!QProcess::startDetached(prepared.command, prepared.arguments, prepared.workingDirectory)) {
        if (error) {
            *error = QStringLiteral("Failed to launch %1").arg(prepared.command);
        }
        return false;
    }
    return true;
}
