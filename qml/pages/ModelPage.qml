import QtQuick 2.6
import Sailfish.Silica 1.0

// Modelle NICHT hardcoden — Liste kommt zur Laufzeit von /models,
// gefiltert nach Function-Calling, Kontextlänge und Preis.
Page {
    id: page
    allowedOrientations: Orientation.All
    onStatusChanged: if (status === PageStatus.Active) AI.refreshModels()

    SilicaListView {
        id: view
        anchors.fill: parent
        model: AI.models

        header: PageHeader { title: qsTr("Modell") }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            highlighted: down || modelData.id === AI.model
            onClicked: { AI.model = modelData.id; pageStack.pop() }

            Column {
                // Siehe MainPage.qml: Breite direkt von item statt ueber
                // Anchors, sonst hinkt sie bei wiederverwendeten Delegates
                // einen Frame hinterher und TruncationMode.Fade zeigt kurz
                // Text der vorherigen Zeile — doppelt, leicht versetzt.
                x: Theme.horizontalPageMargin
                width: item.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    text: modelData.name.length ? modelData.name : modelData.id
                    color: item.highlighted ? Theme.highlightColor : Theme.primaryColor
                }
                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: modelData.free
                          ? qsTr("%1k Kontext · gratis").arg(Math.round(modelData.context/1000))
                          : qsTr("%1k Kontext · %2 $/Mtok")
                                .arg(Math.round(modelData.context/1000))
                                .arg((modelData.prompt * 1000000).toFixed(2))
                }
            }
        }

        ViewPlaceholder {
            enabled: view.count === 0
            text: qsTr("Keine Modelle geladen")
            hintText: KeyStore.hasKey ? qsTr("Liste wird abgerufen")
                                      : qsTr("Zuerst einen API-Key hinterlegen")
        }

        VerticalScrollDecorator {}
    }
}
