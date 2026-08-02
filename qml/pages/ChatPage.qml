import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

/*
 * Chat mit inkrementellem Streaming.
 * Ohne Streaming fühlt sich die App auf Mobilfunk kaputt an — Deltas
 * gehen direkt ins Model, nicht erst die fertige Antwort.
 */
Page {
    id: page
    property int conversationId: -1
    allowedOrientations: Orientation.All

    SilicaListView {
        id: chatView
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: input.top }
        clip: true
        model: History
        delegate: MessageDelegate {}
        verticalLayoutDirection: ListView.BottomToTop

        PullDownMenu {
            MenuItem {
                text: qsTr("Modell wählen")
                onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml"))
            }
            MenuItem {
                text: qsTr("Antwort abbrechen")
                visible: AI.streaming
                onClicked: AI.cancel()
            }
        }

        header: PageHeader {
            title: AI.model.length ? AI.model : qsTr("Kein Modell")
            description: Consent.localOnly
                ? qsTr("Lokal — nichts verlässt das Gerät")
                : qsTr("%1 Tools aktiv").arg(Tools.activeToolCount)
        }
    }

    TextArea {
        id: input
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        placeholderText: qsTr("Nachricht")
        EnterKey.onClicked: {
            if (text.length === 0) return
            AI.sendMessage(text, page.conversationId)
            text = ""
        }
    }
}
