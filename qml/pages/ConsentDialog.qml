import QtQuick 2.6
import Sailfish.Silica 1.0

/*
 * Bestaetigung vor dem Zugriff, nicht danach. Der Dialog zeigt genau das an,
 * was das Tool herausgeben will — Zustimmung ohne Angabe der Daten waere
 * keine.
 *
 * Ablehnen ist der Rueckweg: wer wegnavigiert, hat nicht zugestimmt.
 */
Dialog {
    property string toolName
    property string preview

    allowedOrientations: Orientation.All

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        DialogHeader {
            acceptText: qsTr("Freigeben")
            cancelText: qsTr("Ablehnen")
            title: qsTr("Zugriff bestätigen")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            color: Theme.highlightColor
            text: toolName
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeSmall
            text: preview
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*Theme.horizontalPageMargin
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: qsTr("Personenbezogene Felder werden vor dem Versand durch "
                       + "Platzhalter ersetzt und erst in der Antwort wieder "
                       + "aufgelöst.")
        }
    }
}
