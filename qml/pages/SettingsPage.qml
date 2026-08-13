import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height

        Column {
            id: col
            width: parent.width

            PageHeader { title: qsTr("Einstellungen") }

            SectionHeader { text: qsTr("Backend") }

            TextField {
                id: keyField
                width: parent.width
                label: qsTr("OpenRouter API-Key")
                echoMode: TextInput.Password
                placeholderText: KeyStore.hasKey ? qsTr("gespeichert") : qsTr("nicht gesetzt")
                // Ablage über Sailfish.Secrets — nie in QSettings, nie ins Log
                EnterKey.onClicked: { KeyStore.storeKey("openrouter", text); text = "" }
            }

            Label {
                id: keyStatus
                width: parent.width - 2*Theme.horizontalPageMargin
                x: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.errorColor
                opacity: 0
                text: ""

                Behavior on opacity { FadeAnimation {} }

                Timer {
                    id: hideTimer
                    interval: 4000
                    onTriggered: keyStatus.opacity = 0
                }

                function show(message, isError) {
                    text = message
                    color = isError ? Theme.errorColor : Theme.secondaryHighlightColor
                    opacity = 1
                    hideTimer.restart()
                }
            }

            Connections {
                target: KeyStore
                onErrorOccurred: keyStatus.show(message, true)
                onKeyChanged: if (KeyStore.hasKey) keyStatus.show(qsTr("Key gespeichert"), false)
            }

            TextSwitch {
                text: qsTr("Nur lokal")
                description: qsTr("Lokales Modell verwenden. Nichts verlässt das Gerät, "
                                + "alle Tools sind ohne Rückfrage freigeschaltet.")
                enabled: Caps.localInference
                checked: Consent.localOnly
                onCheckedChanged: Consent.localOnly = checked
            }

            Label {
                visible: !Caps.localInference
                width: parent.width - 2*Theme.horizontalPageMargin
                x: Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Lokale Inference benötigt das Paket sailfishai-llama "
                         + "und ist in der Store-Version nicht verfügbar.")
            }
        }
    }
}
