import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: item
    contentHeight: body.height + Theme.paddingLarge
    property bool isUser: model.role === "user"

    Label {
        id: body
        x: Theme.horizontalPageMargin
        width: parent.width - 2*Theme.horizontalPageMargin
        wrapMode: Text.WordWrap
        horizontalAlignment: item.isUser ? Text.AlignRight : Text.AlignLeft
        color: item.isUser ? Theme.highlightColor : Theme.primaryColor
        text: model.content
    }

    // TODO M1: Tool-Calls visuell abheben (model.toolName)
    // TODO M1: pending -> BusyIndicator während des Streamings
}
