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
                        text: app.currentModel.length > 0
                              ? i18n("Model: %1", app.currentModel)
                              : i18n("Model: None")
                    }
                    Controls.Label {
                        objectName: "gatewayPlaceholder"
                        text: i18n("Gateway: Offline")
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
                            modelField.text = app.currentProjectModel;
                            modelDialog.mode = "project";
                            modelDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Chat model")
                        enabled: app.currentChatId > 0
                        onClicked: {
                            modelField.text = app.currentChatHasModelOverride ? app.currentModel : "";
                            modelDialog.mode = "chat";
                            modelDialog.open();
                        }
                    }
                    Controls.Button {
                        text: i18n("Use project model")
                        enabled: app.currentChatHasModelOverride
                        onClicked: app.clearCurrentChatModelOverride()
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
                if (mode === "project") {
                    app.setCurrentProjectModel(modelField.text);
                } else {
                    app.setCurrentChatModelOverride(modelField.text);
                }
            }
            Controls.TextField {
                id: modelField
                width: parent.width
            }
        }

        Connections {
            target: app
            function onCurrentDraftChanged() {
                if (messageInput.text !== app.currentDraft) {
                    messageInput.text = app.currentDraft;
                }
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
                            onClicked: app.deleteCurrentProject()
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
                            onClicked: app.deleteCurrentChat()
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
                    model: app.messages
                    delegate: Item {
                        required property var modelData
                        width: ListView.view.width
                        height: bubble.height

                        Controls.Frame {
                            id: bubble
                            objectName: modelData.role === "user" ? "userMessage" : "assistantMessage"
                            width: parent.width * 0.8
                            anchors.right: modelData.role === "user" ? parent.right : undefined
                            anchors.left: modelData.role === "user" ? undefined : parent.left
                            Kirigami.Theme.inherit: false
                            Kirigami.Theme.colorSet: modelData.role === "user" ? Kirigami.Theme.Selection : Kirigami.Theme.View

                            background: Rectangle {
                                color: Kirigami.Theme.backgroundColor
                                radius: Kirigami.Units.cornerRadius
                            }

                            Controls.Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: Kirigami.Theme.textColor
                                text: modelData.content
                            }
                        }
                    }
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
                                enabled: app.currentChatId > 0
                                onClicked: {
                                    if (app.addUserMessage(messageInput.text)) {
                                        messageInput.text = "";
                                    }
                                }
                            }
                            Controls.Button {
                                text: i18n("Local reply")
                                enabled: app.currentChatId > 0
                                onClicked: {
                                    const reply = messageInput.text.length > 0
                                          ? messageInput.text
                                          : i18n("Local assistant reply");
                                    if (app.addAssistantMessage(reply)) {
                                        messageInput.text = "";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
