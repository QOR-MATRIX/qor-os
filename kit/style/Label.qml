// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.Label {
    color: enabled ? Theme.textPrimary : Theme.textDisabled
    linkColor: Theme.accentText
    font: Theme.body
}
