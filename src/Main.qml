import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 960
    height: 640
    minimumWidth: 640
    minimumHeight: 480
    title: i18nc("@title:window", "PikaTalk")

    pageStack.initialPage: Kirigami.Page {
        id: mainPage
        title: i18n("PikaTalk")
        padding: 0

        header: Controls.ToolBar {
            id: contextArea
            objectName: "contextArea"

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        objectName: "projectPlaceholder"
                        text: app.currentProjectName.length > 0
                              ? i18n("Project: %1", app.currentProjectName)
                              : i18n("Project: None")
                    }
                    Controls.Label {
                        objectName: "workspacePlaceholder"
                        text: app.currentWorkspace.length > 0
                              ? i18n("Workspace: %1", app.currentWorkspace)
                              : i18n("Workspace: None")
                    }
                    Controls.Label {
                        objectName: "modelPlaceholder"
                        text: {
                            if (app.currentModel.length === 0)
                                return i18n("Model: None");
                            if (app.selectedModelUnavailable)
                                return i18n("Model: %1 (not in gateway list)", app.currentModel);
                            return i18n("Model: %1", app.currentModel);
                        }
                    }
                    Controls.Label {
                        objectName: "gatewayPlaceholder"
                        text: {
                            if (app.lifecyclePhase === "starting")
                                return i18n("Gateway: Starting…");
                            if (app.lifecyclePhase === "stopping")
                                return i18n("Gateway: Stopping…");
                            if (app.lifecyclePhase === "restarting" || app.lifecyclePhase === "reconnecting")
                                return i18n("Gateway: Reconnecting…");
                            if (app.lifecycleStatus === "stopped")
                                return i18n("Gateway: Stopped");
                            if (app.gatewayState === "connected")
                                return i18n("Gateway: Connected");
                            if (app.gatewayState === "connecting")
                                return i18n("Gateway: Connecting");
                            if (app.gatewayState === "error" || app.lifecycleError.length > 0)
                                return i18n("Gateway: Error — %1", app.lifecycleError.length > 0 ? app.lifecycleError : app.gatewayError);
                            return i18n("Gateway: Disconnected");
                        }
                    }
                    Controls.Label {
                        objectName: "gatewayEndpointLabel"
                        text: app.gatewayEndpointDisplay.length > 0
                              ? i18n("Endpoint: %1", app.gatewayEndpointDisplay)
                              : i18n("Endpoint: None")
                    }
                    Controls.Label {
                        objectName: "gatewayVersionLabel"
                        text: app.gatewayVersion.length > 0
                              ? i18n("Version: %1", app.gatewayVersion)
                              : i18n("Version: Unknown")
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Button {
                        text: i18n("Project workspace")
                        enabled: app.currentProjectId > 0
                        onClicked: {
                            workspaceField.text = app.currentProjectWorkspace;
                            workspaceDialog.mode = "project";
                            workspaceDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Chat workspace")
                        enabled: app.currentChatId > 0
                        onClicked: {
                            workspaceField.text = app.currentChatHasWorkspaceOverride ? app.currentWorkspace : "";
                            workspaceDialog.mode = "chat";
                            workspaceDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Use project workspace")
                        enabled: app.currentChatHasWorkspaceOverride
                        onClicked: app.clearCurrentChatWorkspaceOverride()
                    }
                    Controls.Button {
                        text: i18n("Project model")
                        enabled: app.currentProjectId > 0
                        onClicked: {
                            modelField.editText = app.currentProjectModel;
                            modelDialog.mode = "project";
                            modelDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Chat model")
                        enabled: app.currentChatId > 0
                        onClicked: {
                            modelField.editText = app.currentChatHasModelOverride ? app.currentModel : "";
                            modelDialog.mode = "chat";
                            modelDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Use project model")
                        enabled: app.currentChatHasModelOverride
                        onClicked: app.clearCurrentChatModelOverride()
                    }
                    Controls.Button {
                        objectName: "openWorkspaceFileManager"
                        text: i18n("Open folder")
                        enabled: app.currentWorkspace.length > 0
                        onClicked: app.openWorkspaceInFileManager()
                    }
                    Controls.Button {
                        objectName: "openWorkspaceTerminal"
                        text: i18n("Open terminal")
                        enabled: app.currentWorkspace.length > 0
                        onClicked: app.openWorkspaceInTerminal()
                    }
                    Controls.Button {
                        objectName: "openWorkspaceEditor"
                        text: i18n("Open editor")
                        enabled: app.currentWorkspace.length > 0
                        onClicked: app.openWorkspaceInEditor()
                    }
                    Controls.Button {
                        objectName: "startGatewayButton"
                        text: i18n("Start gateway")
                        enabled: app.canStartGateway
                        onClicked: app.startLocalGateway()
                    }
                    Controls.Button {
                        objectName: "stopGatewayButton"
                        text: i18n("Stop gateway")
                        enabled: app.canStopGateway
                        onClicked: app.stopLocalGateway()
                    }
                    Controls.Button {
                        objectName: "restartGatewayButton"
                        text: i18n("Restart gateway")
                        enabled: app.canRestartGateway
                        onClicked: app.restartLocalGateway()
                    }
                }

                Controls.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    visible: app.workspaceActionError.length > 0 || app.lifecycleError.length > 0
                    text: {
                        if (app.lifecycleError.length > 0)
                            return i18n("Gateway action: %1", app.lifecycleError);
                        return i18n("Workspace action: %1", app.workspaceActionError);
                    }
                }
            }
        }

        Controls.Dialog {
            id: workspaceDialog
            property string mode: "project"
            title: mode === "project" ? i18n("Project workspace") : i18n("Chat workspace override")
            standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
            modal: true
            anchors.centerIn: parent
            onAccepted: {
                if (mode === "project") {
                    app.setCurrentProjectWorkspace(workspaceField.text);
                } else {
                    app.setCurrentChatWorkspaceOverride(workspaceField.text);
                }
            }
            Controls.TextField {
                id: workspaceField
                width: parent.width
            }
        }

        Controls.Dialog {
            id: modelDialog
            property string mode: "project"
            title: mode === "project" ? i18n("Project model") : i18n("Chat model override")
            standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
            modal: true
            anchors.centerIn: parent
            onAccepted: {
                const value = modelField.editText.length > 0 ? modelField.editText : modelField.currentText;
                if (mode === "project") {
                    app.setCurrentProjectModel(value);
                } else {
                    app.setCurrentChatModelOverride(value);
                }
            }
            ColumnLayout {
                Controls.ComboBox {
                    id: modelField
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 22
                    editable: true
                    model: app.availableModels
                }
                Controls.Label {
                    visible: app.selectedModelUnavailable
                    wrapMode: Text.Wrap
                    text: i18n("The stored model is not in the PikaClaw list. Choose another model.")
                }
            }
        }

        Connections {
            target: app
            function onCurrentDraftChanged() {
                if (messageInput.text !== app.currentDraft) {
                    messageInput.text = app.currentDraft;
                }
            }
            function onPendingDeletionChanged() {
                if (app.hasPendingDeletion) {
                    deleteConfirmDialog.open();
                } else if (deleteConfirmDialog.visible) {
                    deleteConfirmDialog.close();
                }
            }
        }

        Controls.Dialog {
            id: deleteConfirmDialog
            objectName: "deleteConfirmDialog"
            title: app.pendingDeletionTitle
            standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
            modal: true
            anchors.centerIn: parent
            onAccepted: app.confirmPendingDeletion()
            onRejected: app.cancelPendingDeletion()
            Controls.Label {
                width: Kirigami.Units.gridUnit * 22
                wrapMode: Text.Wrap
                text: app.pendingDeletionMessage
            }
        }

        Controls.SplitView {
            id: mainSplit
            anchors.fill: parent
            orientation: Qt.Horizontal

            Controls.Pane {
                id: sidebarArea
                objectName: "sidebarArea"
                Controls.SplitView.preferredWidth: Kirigami.Units.gridUnit * 16
                Controls.SplitView.minimumWidth: Kirigami.Units.gridUnit * 12

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        text: i18n("Projects")
                        font.bold: true
                    }
                    ListView {
                        id: projectList
                        objectName: "projectList"
                        Layout.fillWidth: true
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 8
                        clip: true
                        model: app.projects
                        currentIndex: {
                            for (let i = 0; i < app.projects.length; ++i) {
                                if (app.projects[i].id === app.currentProjectId) {
                                    return i;
                                }
                            }
                            return -1;
                        }
                        delegate: Controls.ItemDelegate {
                            required property var modelData
                            width: ListView.view.width
                            text: modelData.name
                            highlighted: app.currentProjectId === modelData.id
                            onClicked: app.selectProject(modelData.id)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Controls.Button {
                            text: i18n("New")
                            onClicked: {
                                projectNameField.text = i18n("New project");
                                projectNameDialog.mode = "create";
                                projectNameDialog.open();
                            }
                        }
                        Controls.Button {
                            text: i18n("Rename")
                            enabled: app.currentProjectId > 0
                            onClicked: {
                                projectNameField.text = app.currentProjectName;
                                projectNameDialog.mode = "rename";
                                projectNameDialog.open();
                            }
                        }
                        Controls.Button {
                            text: i18n("Delete")
                            enabled: app.currentProjectId > 0
                            onClicked: app.requestDeleteCurrentProject()
                        }
                    }
                    Controls.Dialog {
                        id: projectNameDialog
                        property string mode: "create"
                        title: mode === "create" ? i18n("New project") : i18n("Rename project")
                        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
                        modal: true
                        anchors.centerIn: parent
                        onAccepted: {
                            if (mode === "create") {
                                app.createProject(projectNameField.text);
                            } else {
                                app.renameCurrentProject(projectNameField.text);
                            }
                        }
                        Controls.TextField {
                            id: projectNameField
                            width: parent.width
                        }
                    }
                    Controls.Label {
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        text: i18n("Chats")
                        font.bold: true
                    }
                    ListView {
                        id: chatList
                        objectName: "chatList"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: app.chats
                        currentIndex: {
                            for (let i = 0; i < app.chats.length; ++i) {
                                if (app.chats[i].id === app.currentChatId) {
                                    return i;
                                }
                            }
                            return -1;
                        }
                        delegate: Controls.ItemDelegate {
                            required property var modelData
                            width: ListView.view.width
                            text: modelData.title
                            highlighted: app.currentChatId === modelData.id
                            onClicked: app.selectChat(modelData.id)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Controls.Button {
                            text: i18n("New")
                            enabled: app.currentProjectId > 0
                            onClicked: {
                                chatNameField.text = i18n("New chat");
                                chatNameDialog.mode = "create";
                                chatNameDialog.open();
                            }
                        }
                        Controls.Button {
                            text: i18n("Rename")
                            enabled: app.currentChatId > 0
                            onClicked: {
                                chatNameField.text = app.currentChatTitle;
                                chatNameDialog.mode = "rename";
                                chatNameDialog.open();
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Controls.Button {
                            text: i18n("Archive")
                            enabled: app.currentChatId > 0
                            onClicked: app.archiveCurrentChat()
                        }
                        Controls.Button {
                            text: i18n("Delete")
                            enabled: app.currentChatId > 0
                            onClicked: app.requestDeleteCurrentChat()
                        }
                    }
                    Controls.Dialog {
                        id: chatNameDialog
                        property string mode: "create"
                        title: mode === "create" ? i18n("New chat") : i18n("Rename chat")
                        standardButtons: Controls.Dialog.Ok | Controls.Dialog.Cancel
                        modal: true
                        anchors.centerIn: parent
                        onAccepted: {
                            if (mode === "create") {
                                app.createChat(chatNameField.text);
                            } else {
                                app.renameCurrentChat(chatNameField.text);
                            }
                        }
                        Controls.TextField {
                            id: chatNameField
                            width: parent.width
                        }
                    }
                }
            }

            ColumnLayout {
                id: chatColumn
                Controls.SplitView.fillWidth: true
                spacing: 0

                ListView {
                    id: conversationArea
                    objectName: "conversationArea"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: Kirigami.Units.largeSpacing
                    model: {
                        const messages = app.messages;
                        const tools = app.toolActivities;
                        const byMessage = {};
                        const unassigned = [];
                        for (let i = 0; i < tools.length; ++i) {
                            const tool = tools[i];
                            const messageId = tool.messageId || 0;
                            if (messageId > 0) {
                                if (!byMessage[messageId]) {
                                    byMessage[messageId] = [];
                                }
                                byMessage[messageId].push(tool);
                            } else {
                                unassigned.push(tool);
                            }
                        }
                        const items = [];
                        for (let i = 0; i < messages.length; ++i) {
                            const message = messages[i];
                            if (message.role === "assistant") {
                                const linked = byMessage[message.id] || [];
                                for (let j = 0; j < linked.length; ++j) {
                                    items.push({ kind: "tool", data: linked[j] });
                                }
                            }
                            items.push({ kind: "message", data: message });
                        }
                        for (let i = 0; i < unassigned.length; ++i) {
                            items.push({ kind: "tool", data: unassigned[i] });
                        }
                        return items;
                    }
                    delegate: Item {
                        id: row
                        required property var modelData
                        width: ListView.view.width
                        height: modelData.kind === "tool" ? toolFrame.height : bubble.height

                        Controls.Frame {
                            id: bubble
                            objectName: modelData.data.role === "user" ? "userMessage" : "assistantMessage"
                            visible: modelData.kind === "message"
                            width: parent.width * 0.8
                            anchors.right: modelData.data.role === "user" ? parent.right : undefined
                            anchors.left: modelData.data.role === "user" ? undefined : parent.left
                            Kirigami.Theme.inherit: false
                            Kirigami.Theme.colorSet: modelData.data.role === "user" ? Kirigami.Theme.Selection : Kirigami.Theme.View

                            background: Rectangle {
                                color: Kirigami.Theme.backgroundColor
                                radius: Kirigami.Units.cornerRadius
                            }

                            ColumnLayout {
                                width: parent.width
                                spacing: Kirigami.Units.smallSpacing

                                Repeater {
                                    model: app.messageSegments(modelData.data.content || "")
                                    delegate: ColumnLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: Kirigami.Units.smallSpacing

                                        Controls.Label {
                                            Layout.fillWidth: true
                                            wrapMode: Text.Wrap
                                            color: Kirigami.Theme.textColor
                                            visible: modelData.kind === "text" && (modelData.text || "").length > 0
                                            text: modelData.text || ""
                                        }

                                        Controls.Frame {
                                            Layout.fillWidth: true
                                            visible: modelData.kind === "code"
                                            objectName: "codeBlock"

                                            ColumnLayout {
                                                width: parent.width
                                                spacing: Kirigami.Units.smallSpacing

                                                Controls.Label {
                                                    Layout.fillWidth: true
                                                    wrapMode: Text.Wrap
                                                    font.family: "monospace"
                                                    color: Kirigami.Theme.textColor
                                                    text: modelData.text || ""
                                                }
                                                Controls.Button {
                                                    objectName: "copyCodeBlock"
                                                    text: i18n("Copy code")
                                                    onClicked: app.copyText(modelData.text || "")
                                                }
                                            }
                                        }
                                    }
                                }

                                Controls.Button {
                                    objectName: "copyMessage"
                                    text: i18n("Copy")
                                    onClicked: app.copyText(modelData.data.content || "")
                                }
                            }
                        }

                        Controls.Frame {
                            id: toolFrame
                            objectName: "toolActivity"
                            visible: modelData.kind === "tool"
                            width: parent.width * 0.8
                            anchors.left: parent.left
                            Kirigami.Theme.inherit: false
                            Kirigami.Theme.colorSet: Kirigami.Theme.View

                            background: Rectangle {
                                color: Kirigami.Theme.backgroundColor
                                radius: Kirigami.Units.cornerRadius
                                opacity: 0.85
                            }

                            ColumnLayout {
                                width: parent.width
                                spacing: Kirigami.Units.smallSpacing

                                Controls.Button {
                                    objectName: "toolActivitySummary"
                                    Layout.fillWidth: true
                                    flat: true
                                    text: {
                                        const status = modelData.data.status || "running";
                                        const statusLabel = status === "ok" ? i18n("ok")
                                                          : status === "error" ? i18n("failed")
                                                          : i18n("running");
                                        return i18n("Tool: %1 — %2", modelData.data.toolName || i18n("unknown"), statusLabel);
                                    }
                                    onClicked: toolDetails.visible = !toolDetails.visible
                                }

                                ColumnLayout {
                                    id: toolDetails
                                    objectName: "toolActivityDetails"
                                    Layout.fillWidth: true
                                    visible: false
                                    spacing: Kirigami.Units.smallSpacing

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                        visible: (modelData.data.argumentsJson || "").length > 0
                                        text: i18n("Input: %1", modelData.data.argumentsJson || "")
                                    }
                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                        visible: (modelData.data.resultText || "").length > 0
                                        text: modelData.data.status === "error"
                                              ? i18n("Error: %1", modelData.data.errorText || modelData.data.resultText || "")
                                              : i18n("Result: %1", modelData.data.resultText || "")
                                    }
                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.Wrap
                                        visible: modelData.data.status === "error"
                                                 && (modelData.data.errorText || "").length > 0
                                                 && modelData.data.errorText !== modelData.data.resultText
                                        text: i18n("Detail: %1", modelData.data.errorText || "")
                                    }
                                }
                            }
                        }
                    }
                }

                Controls.Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    wrapMode: Text.Wrap
                    visible: app.isGenerating
                    text: app.streamingAssistantText.length > 0
                          ? app.streamingAssistantText
                          : i18n("Generating…")
                }

                Controls.Pane {
                    id: inputArea
                    objectName: "inputArea"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 8

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Kirigami.Units.smallSpacing

                        Controls.TextArea {
                            id: messageInput
                            objectName: "messageInput"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            enabled: app.currentChatId > 0
                            placeholderText: i18n("Message")
                            wrapMode: TextEdit.Wrap
                            Component.onCompleted: text = app.currentDraft
                            onTextChanged: {
                                if (app.currentChatId > 0 && text !== app.currentDraft) {
                                    app.setCurrentDraft(text);
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Controls.Button {
                                text: i18n("Send")
                                enabled: app.currentChatId > 0 && !app.isGenerating
                                onClicked: {
                                    if (app.sendChatMessage(messageInput.text)) {
                                        messageInput.text = "";
                                    }
                                }
                            }
                            Controls.Button {
                                text: i18n("Stop")
                                visible: app.isGenerating
                                enabled: app.isGenerating
                                onClicked: app.stopGeneration()
                            }
                            Controls.Button {
                                text: i18n("Retry / Regenerate")
                                visible: !app.isGenerating
                                enabled: app.canRetryOrRegenerate
                                onClicked: app.retryOrRegenerate()
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                visible: app.requestError.length > 0
                                text: i18n("Error: %1", app.requestError)
                            }
                        }
                    }
                }
            }
        }
    }
}
