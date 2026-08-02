import QtQuick 2.6
import Sailfish.Silica 1.0

// Modelle NICHT hardcoden — Liste kommt zur Laufzeit von /models.
Page {
    allowedOrientations: Orientation.All
    onStatusChanged: if (status === PageStatus.Active) AI.refreshModels()

    SilicaListView {
        anchors.fill: parent
        header: PageHeader { title: qsTr("Modell") }
        model: 0   // TODO M1
        ViewPlaceholder { enabled: true; text: qsTr("Keine Modelle geladen") }
    }
}
