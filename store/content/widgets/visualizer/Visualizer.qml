// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit
import QTOS.Services

// Visualizer: the live spectrum of what is playing, in the accent colour.
Item {
    id: widget

    Spectrum {
        anchors.fill: parent
        anchors.margins: 18
        anchors.bottomMargin: 34
        values: Reactive.spectrum.slice(2, 34)
        bars: 32
        spacing: 3
        color: Theme.accent
        mirrored: false
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        text: Reactive.playing && Services.media.title.length > 0 ? Services.media.title
                                                                   : qsTr("Play something to see it move")
        color: Theme.textTertiary
        font: Theme.caption
        elide: Text.ElideRight
    }
}
