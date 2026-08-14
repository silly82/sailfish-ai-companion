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

    Connections {
        target: AI
        onErrorOccurred: banner.showError(message)

        // Der Roundtrip wartet an dieser Stelle. Ohne Antwort laeuft er nicht
        // weiter, und Wegnavigieren zaehlt als Ablehnung.
        onConsentRequired: {
            var dialog = pageStack.push(Qt.resolvedUrl("ConsentDialog.qml"),
                                        { toolName: toolName, preview: preview })
            dialog.accepted.connect(function() { AI.resolveConsent(true) })
            dialog.rejected.connect(function() { AI.resolveConsent(false) })
        }
    }

    SilicaListView {
        id: chatView
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: input.top }
        clip: true
        model: History
        delegate: MessageDelegate {}

        // Model ist chronologisch; die neueste Nachricht steht unten und soll
        // beim Streamen sichtbar bleiben.
        onCountChanged: positionViewAtEnd()

        PullDownMenu {
            MenuItem {
                text: qsTr("Choose model")
                onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml"))
            }
            MenuItem {
                text: qsTr("Cancel response")
                visible: AI.streaming
                onClicked: AI.cancel()
            }
        }

        header: PageHeader {
            title: AI.model.length ? AI.model : qsTr("No model")
            description: Consent.localOnly
                ? qsTr("Local — nothing leaves the device")
                : qsTr("%1 tools active").arg(Tools.activeToolCount)
        }

        VerticalScrollDecorator {}
    }

    Label {
        id: banner
        function showError(text) { banner.text = text; hideTimer.restart() }

        anchors { left: parent.left; right: parent.right; bottom: input.top }
        leftPadding: Theme.horizontalPageMargin
        rightPadding: Theme.horizontalPageMargin
        wrapMode: Text.WordWrap
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.errorColor
        visible: text.length > 0

        Timer {
            id: hideTimer
            interval: 6000
            onTriggered: banner.text = ""
        }
    }

    TextArea {
        id: input
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        placeholderText: AI.model.length ? qsTr("Message")
                                         : qsTr("Choose a model first")
        enabled: !AI.streaming && AI.model.length > 0
        EnterKey.enabled: text.trim().length > 0
        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.onClicked: {
            AI.sendMessage(text.trim(), page.conversationId)
            text = ""
        }
    }
}
