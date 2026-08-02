import QtQuick 2.6
import Sailfish.Silica 1.0

// Konversationsübersicht. Pulley-Down = neuer Chat + Einstellungen.
Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent

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
                onClicked: pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
            }
        }

        header: PageHeader {
            title: qsTr("AI Companion")
            description: Caps.sandboxed ? qsTr("Store-Version")
                                        : qsTr("Vollzugriff")
        }

        // TODO M1: model an eine Konversationsliste aus ConversationStore binden
        model: 0

        ViewPlaceholder {
            enabled: true
            text: qsTr("Noch keine Konversation")
            hintText: qsTr("Von oben ziehen, um zu starten")
        }
    }
}
