import QtQuick 2.6
import Sailfish.Silica 1.0

// Konversationsübersicht. Pulley-Down = neuer Chat + Einstellungen.
Page {
    id: page
    allowedOrientations: Orientation.All

    function openConversation(id) {
        History.loadConversation(id)
        pageStack.push(Qt.resolvedUrl("ChatPage.qml"), { conversationId: id })
    }

    SilicaListView {
        id: view
        anchors.fill: parent
        model: History.conversations

        PullDownMenu {
            MenuItem {
                text: qsTr("Einstellungen")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("Tools & Freigaben")
                onClicked: pageStack.push(Qt.resolvedUrl("ToolsPage.qml"))
            }
            MenuItem {
                text: qsTr("Neue Konversation")
                onClicked: page.openConversation(
                    History.createConversation(qsTr("Neue Konversation")))
            }
        }

        header: PageHeader {
            title: qsTr("AI Companion")
            description: Caps.sandboxed ? qsTr("Store-Version")
                                        : qsTr("Vollzugriff")
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            onClicked: page.openConversation(modelData.conversationId)

            Column {
                // Breite direkt von item ableiten statt ueber die Anchors
                // dieser Column: bei wiederverwendeten Delegates (Scrollen)
                // hinkt die anchor-vermittelte Breite sonst einen Frame
                // hinterher, und TruncationMode.Fade rendert Titel/Preview
                // dann kurz mit der Breite der vorherigen Zeile — sichtbar
                // als doppelter, leicht versetzter Text.
                x: Theme.horizontalPageMargin
                width: item.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    text: modelData.title
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    maximumLineCount: 1
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: modelData.preview.length
                          ? modelData.preview
                          : qsTr("%n Nachricht(en)", "", modelData.messageCount)
                }
            }

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Löschen")
                    onClicked: item.remorseAction(qsTr("Löschen"), function() {
                        History.deleteConversation(modelData.conversationId)
                    })
                }
            }
        }

        ViewPlaceholder {
            enabled: view.count === 0
            text: qsTr("Noch keine Konversation")
            hintText: qsTr("Von oben ziehen, um zu starten")
        }

        VerticalScrollDecorator {}
    }
}
