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

            PageHeader { title: qsTr("Settings") }

            SectionHeader { text: qsTr("Backend") }

            TextField {
                id: keyField
                width: parent.width
                label: qsTr("OpenRouter API key")
                echoMode: TextInput.Password
                placeholderText: KeyStore.hasKey ? qsTr("saved") : qsTr("not set")
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
                onKeyChanged: if (KeyStore.hasKey) keyStatus.show(qsTr("Key saved"), false)
            }

            SectionHeader { text: qsTr("Model") }

            ValueButton {
                width: parent.width
                label: qsTr("Default model")
                value: AI.model.length ? AI.model : qsTr("None selected")
                onClicked: pageStack.push(Qt.resolvedUrl("ModelPage.qml"))
            }

            TextSwitch {
                text: qsTr("Local only")
                description: qsTr("Use a local model. Nothing leaves the device, "
                                + "all tools are enabled without confirmation.")
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
                text: qsTr("Local inference needs the sailfishai-llama package "
                         + "and isn't available in the Store version.")
            }
        }
    }
}
