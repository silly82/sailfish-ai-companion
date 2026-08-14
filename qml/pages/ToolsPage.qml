import QtQuick 2.6
import Sailfish.Silica 1.0

/*
 * Die Datenschleuse, sichtbar gemacht.
 * Alles ausser Sensitivity.Low ist per Default AUS.
 *
 * Die Liste kommt aus der ToolRegistry und enthaelt nur, was das laufende
 * Target kann — deshalb steht hier kein einziges #ifdef und keine
 * Fallunterscheidung zwischen Harbour und Full.
 */
Page {
    id: page
    allowedOrientations: Orientation.All

    // Muss zu ConsentGate::Sensitivity passen.
    function sensitivityLabel(level) {
        switch (level) {
        case 0:  return qsTr("System info")
        case 1:  return qsTr("Personal")
        default: return qsTr("Critical")
        }
    }

    function sensitivityColor(level) {
        return level === 0 ? Theme.secondaryColor : Theme.errorColor
    }

    SilicaListView {
        anchors.fill: parent
        model: Tools.tools

        header: PageHeader {
            title: qsTr("Tools & Permissions")
            description: Consent.localOnly
                ? qsTr("Local — %1 active, nothing leaves the device").arg(Tools.activeToolCount)
                : qsTr("%1 active").arg(Tools.activeToolCount)
        }

        delegate: TextSwitch {
            width: parent.width
            text: modelData.name
            description: modelData.description
            checked: modelData.enabled
            automaticCheck: false
            onClicked: Tools.setToolEnabled(modelData.name, !modelData.enabled)

            Label {
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    top: parent.top
                    topMargin: Theme.paddingMedium
                }
                font.pixelSize: Theme.fontSizeTiny
                color: page.sensitivityColor(modelData.sensitivity)
                text: page.sensitivityLabel(modelData.sensitivity)
            }
        }

        footer: Label {
            x: Theme.horizontalPageMargin
            width: page.width - 2*Theme.horizontalPageMargin
            topPadding: Theme.paddingLarge
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: qsTr("From “Personal” onward, every access asks for "
                       + "confirmation first, and the result is anonymised before "
                       + "it's sent.")
        }

        VerticalScrollDecorator {}
    }
}
