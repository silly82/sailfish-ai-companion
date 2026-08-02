import QtQuick 2.6
import Sailfish.Silica 1.0

/*
 * Die Datenschleuse, sichtbar gemacht.
 * Alles ausser Sensitivity.Low ist per Default AUS.
 */
Page {
    allowedOrientations: Orientation.All

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            title: qsTr("Tools & Freigaben")
            description: qsTr("%1 aktiv").arg(Tools.activeToolCount)
        }
        // TODO M2: an ein Tool-Listmodel aus ToolRegistry binden.
        //          Delegate = TextSwitch mit Beschreibung + Sensitivity-Badge.
        model: 0
    }
}
