// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "touchpad.h"

namespace qor::input {

// Qt reports touchpad gestures itself here (Wayland pointer gestures, macOS).
// On bare metal Prism will read them from libinput with its KMS backend.
TouchpadSource *TouchpadSource::instance() { return nullptr; }

} // namespace qor::input
