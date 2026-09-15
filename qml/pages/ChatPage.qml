import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import Nemo.KeepAlive 1.2
import "../components"

/*
 * Chat mit inkrementellem Streaming.
 * Ohne Streaming fühlt sich die App auf Mobilfunk kaputt an — Deltas
 * gehen direkt ins Model, nicht erst die fertige Antwort.
 */
Page {
    id: page
    property int conversationId: -1
    // Aus dem ImagePickerPage, bis zum naechsten Senden oder Abbrechen.
    // Kein Kopieren in App-Storage -- der Pfad bleibt gueltig, solange die
    // Pictures-Permission ihn sichtbar haelt.
    property string pendingImagePath: ""
    allowedOrientations: Orientation.All

    Component {
        id: imagePickerPageComponent
        ImagePickerPage {
            onSelectedContentPropertiesChanged: {
                page.pendingImagePath = selectedContentProperties.filePath
            }
        }
    }

    // Verhindert Suspend, waehrend eine Antwort streamt — das Display darf
    // trotzdem blanken, der Request laeuft im Hintergrund zu Ende (H8).
    KeepAlive {
        enabled: AI.streaming
    }

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
        anchors { top: parent.top; left: parent.left; right: parent.right; bottom: inputArea.top }
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

        anchors { left: parent.left; right: parent.right; bottom: inputArea.top }
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

    Column {
        id: inputArea
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }

        // Vorschau des angehaengten Bilds, bevor gesendet wird -- ohne das
        // waere nicht ersichtlich, dass "Senden" auch das Bild mitschickt.
        Row {
            id: attachmentPreview
            visible: page.pendingImagePath.length > 0
            height: visible ? Theme.itemSizeMedium : 0
            x: Theme.horizontalPageMargin
            width: parent.width - 2*Theme.horizontalPageMargin
            spacing: Theme.paddingMedium

            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.itemSizeMedium
                height: Theme.itemSizeMedium
                fillMode: Image.PreserveAspectCrop
                source: page.pendingImagePath.length > 0
                        ? "file://" + page.pendingImagePath : ""
            }
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-clear"
                onClicked: page.pendingImagePath = ""
            }
        }

        Row {
            width: parent.width

            IconButton {
                id: attachButton
                width: Caps.filesystem ? Theme.itemSizeMedium : 0
                height: input.height
                visible: Caps.filesystem
                icon.source: "image://theme/icon-m-attach"
                enabled: !AI.streaming
                onClicked: pageStack.push(imagePickerPageComponent)
            }

            TextArea {
                id: input
                width: parent.width - attachButton.width
                placeholderText: AI.model.length ? qsTr("Message")
                                                 : qsTr("Choose a model first")
                enabled: !AI.streaming && AI.model.length > 0
                EnterKey.enabled: text.trim().length > 0 || page.pendingImagePath.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    AI.sendMessage(text.trim(), page.conversationId, page.pendingImagePath)
                    text = ""
                    page.pendingImagePath = ""
                }
            }
        }
    }
}
