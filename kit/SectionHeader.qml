// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// Small, quiet heading that groups related rows in sidebars and lists.
Text {
    leftPadding: 10
    topPadding: 10
    bottomPadding: 4
    font: Theme.caption
    color: Theme.textTertiary
    elide: Text.ElideRight
    Accessible.role: Accessible.Heading
}
