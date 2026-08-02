import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    Label {
        anchors.centerIn: parent
        width: parent.width - 2*Theme.paddingMedium
        wrapMode: Text.WordWrap
        maximumLineCount: 4
        elide: Text.ElideRight
        text: qsTr("AI Companion")
    }

    CoverActionList {
        CoverAction { iconSource: "image://theme/icon-cover-new" }
        // TODO M6: zweite Action "Diktieren" -> springt direkt in die Aufnahme
    }
}
