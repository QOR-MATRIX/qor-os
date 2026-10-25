// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QtTypes>

namespace qtos::input {

// The Linux evdev key code for a Qt key on a US layout (0 when unknown).
// Synthetic key events carry it as their native scan code (+8, as XKB
// expects), so Wayland clients inside Prism receive the right key.
quint32 evdevKeyCode(int qtKey);

// Whether a US layout types the key with Shift (Key_Exclam is Shift+1).
bool needsShift(int qtKey);

} // namespace qtos::input
