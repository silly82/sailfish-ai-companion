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
                width: parent.width
                label: qsTr("OpenRouter API-Key")
                echoMode: TextInput.Password
                placeholderText: Keys.hasKey ? qsTr("gespeichert") : qsTr("nicht gesetzt")
                // Ablage über Sailfish.Secrets — nie in QSettings, nie ins Log
                EnterKey.onClicked: { Keys.storeKey("openrouter", text); text = "" }
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
