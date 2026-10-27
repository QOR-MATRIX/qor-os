// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma Singleton

import QtQml
import QOR.Kit

// Spring presets for Lumen motion. Assign one to `Spring.preset`.
//   snappy – controls, hover, small state changes
//   smooth – windows, sheets, panels
//   bouncy – playful moments (drops, confirmations); use sparingly
//   gentle – large, ambient transitions (appearance, wallpaper)
QtObject {
    readonly property var snappy: Tokens.motion.snappy
    readonly property var smooth: Tokens.motion.smooth
    readonly property var bouncy: Tokens.motion.bouncy
    readonly property var gentle: Tokens.motion.gentle
    readonly property int staggerMs: Tokens.motion.staggerMs
    readonly property bool reduced: Theme.reduceMotion
}
