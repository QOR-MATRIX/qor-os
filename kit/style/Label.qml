// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QTOS.Kit

T.Label {
    color: enabled ? Theme.textPrimary : Theme.textDisabled
    linkColor: Theme.accentText
    font: Theme.body
}
