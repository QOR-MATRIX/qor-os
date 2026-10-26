// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "touchpad.h"

namespace qtos::input {

// Qt reports touchpad gestures itself here (Wayland pointer gestures, macOS).
// On bare metal Prism will read them from libinput with its KMS backend.
TouchpadSource *TouchpadSource::instance() { return nullptr; }

} // namespace qtos::input
