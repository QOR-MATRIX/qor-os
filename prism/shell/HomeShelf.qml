// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit

// One row of Home: its title and its cards, side by side. Left and Right go
// along it and stop at its ends; Up and Down ask Home for the row above or
// below (leave()); Return, Enter or Space plays or opens the card the
// keyboard is on. It keeps the keyboard while its cards change.
FocusScope {
    id: shelf

    required property string title
    required property var items
    property alias currentIndex: list.currentIndex

    signal chosen(var item)
    signal leave(int direction)

    visible: items.length > 0
    implicitHeight: visible ? heading.height + 14 + list.height : 0

    function choose() {
        if (list.currentIndex >= 0 && list.currentIndex < shelf.items.length)
            shelf.chosen(shelf.items[list.currentIndex]);
    }

    Text {
        id: heading
        text: shelf.title
        color: Theme.textPrimary
        font: Theme.title3
    }
    ListView {
        id: list
        objectName: "homeShelfList"
        anchors.top: heading.bottom
        anchors.topMargin: 14
        width: parent.width
        height: 226
        focus: true
        orientation: ListView.Horizontal
        spacing: 24
        clip: false
        model: shelf.items
        keyNavigationEnabled: false
        boundsBehavior: Flickable.StopAtBounds
        highlightMoveDuration: Theme.reduceMotion ? 0 : 200
        preferredHighlightBegin: 0
        preferredHighlightEnd: width - 260
        highlightRangeMode: ListView.ApplyRange
        onCountChanged: if (currentIndex >= count) currentIndex = count - 1

        delegate: HomeCard {
            required property var modelData
            required property int index
            item: modelData
            current: list.activeFocus && index === list.currentIndex
            onChosen: {
                list.currentIndex = index;
                shelf.choose();
            }
        }

        // Every arrow is taken, at an end too: the controller interface would
        // otherwise move on as Tab does.
        Keys.onLeftPressed: currentIndex = Math.max(0, currentIndex - 1)
        Keys.onRightPressed: currentIndex = Math.min(count - 1, currentIndex + 1)
        Keys.onUpPressed: shelf.leave(-1)
        Keys.onDownPressed: shelf.leave(1)
        Keys.onReturnPressed: shelf.choose()
        Keys.onEnterPressed: shelf.choose()
        Keys.onSpacePressed: shelf.choose()
    }
}
