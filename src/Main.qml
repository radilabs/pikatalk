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

            contentItem: Flow {
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    objectName: "projectPlaceholder"
                    text: i18n("Project: PikaTalk")
                }
                Controls.Label {
                    objectName: "workspacePlaceholder"
                    text: i18n("Workspace: ~/code/pikatalk")
                }
                Controls.Label {
                    objectName: "modelPlaceholder"
                    text: i18n("Model: example-model")
                }
                Controls.Label {
                    objectName: "gatewayPlaceholder"
                    text: i18n("Gateway: Offline")
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
                    Controls.Label {
                        text: i18n("PikaTalk")
                    }
                    Controls.Label {
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        text: i18n("Chats")
                        font.bold: true
                    }
                    Controls.Label {
                        text: i18n("Getting started")
                    }
                    Controls.Label {
                        text: i18n("Example chat")
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            ColumnLayout {
                id: chatColumn
                Controls.SplitView.fillWidth: true
                spacing: 0

                Controls.ScrollView {
                    id: conversationArea
                    objectName: "conversationArea"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: conversationArea.availableWidth
                        spacing: Kirigami.Units.largeSpacing

                        Controls.Frame {
                            id: userMessage
                            objectName: "userMessage"
                            Layout.alignment: Qt.AlignRight
                            Layout.maximumWidth: conversationArea.availableWidth * 0.8
                            Kirigami.Theme.inherit: false
                            Kirigami.Theme.colorSet: Kirigami.Theme.Selection

                            background: Rectangle {
                                color: Kirigami.Theme.backgroundColor
                                radius: Kirigami.Units.cornerRadius
                            }

                            Controls.Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: Kirigami.Theme.textColor
                                text: i18n("Can you show me the intended PikaTalk layout?")
                            }
                        }

                        Controls.Frame {
                            id: assistantMessage
                            objectName: "assistantMessage"
                            Layout.alignment: Qt.AlignLeft
                            Layout.maximumWidth: conversationArea.availableWidth * 0.8
                            Kirigami.Theme.inherit: false
                            Kirigami.Theme.colorSet: Kirigami.Theme.View

                            background: Rectangle {
                                color: Kirigami.Theme.backgroundColor
                                radius: Kirigami.Units.cornerRadius
                            }

                            Controls.Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                color: Kirigami.Theme.textColor
                                text: i18n("This is a static assistant reply. The sidebar, context bar, and input field are layout placeholders only.")
                            }
                        }
                    }
                }

                Controls.Pane {
                    id: inputArea
                    objectName: "inputArea"
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 6

                    Controls.TextArea {
                        id: messageInput
                        objectName: "messageInput"
                        anchors.fill: parent
                        placeholderText: i18n("Message")
                        wrapMode: TextEdit.Wrap
                    }
                }
            }
        }
    }
}
