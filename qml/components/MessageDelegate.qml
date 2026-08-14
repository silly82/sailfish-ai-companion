import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: item
    contentHeight: column.height + Theme.paddingLarge
    property bool isUser: model.role === "user"

    // Tool-Ergebnisse sind JSON fuer das Modell, nicht fuer den Nutzer. Sie
    // gehoeren in den Verlauf, aber als Beleg — nicht als Wortmeldung.
    property bool isTool: model.role === "tool"

    Column {
        id: column
        x: Theme.horizontalPageMargin
        y: Theme.paddingMedium
        width: parent.width - 2*Theme.horizontalPageMargin
        spacing: Theme.paddingSmall

        Label {
            visible: model.toolName.length > 0
            width: parent.width
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
            text: qsTr("Tool: %1").arg(model.toolName)
        }

        Label {
            width: parent.width
            wrapMode: item.isTool ? Text.NoWrap : Text.WordWrap
            truncationMode: item.isTool ? TruncationMode.Fade : TruncationMode.None
            horizontalAlignment: item.isUser ? Text.AlignRight : Text.AlignLeft
            font.pixelSize: item.isTool ? Theme.fontSizeExtraSmall
                                        : Theme.fontSizeMedium
            color: item.isTool ? Theme.secondaryColor
                 : item.isUser ? Theme.highlightColor
                               : Theme.primaryColor
            text: model.content
        }

        // Solange noch kein Zeichen eingetroffen ist, ist die Zeile leer —
        // ohne Indikator wirkt die App in dieser Zeit hängengeblieben.
        BusyIndicator {
            visible: model.pending && model.content.length === 0
            running: visible
            size: BusyIndicatorSize.Small
        }
    }
}
