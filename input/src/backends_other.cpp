// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Platforms without a controller backend yet (macOS: GameController.framework
// is the planned one). Virtual controllers still work.

#include "backends.h"

namespace qtos::input {

std::vector<std::unique_ptr<Backend>> createHardwareBackends() { return {}; }

} // namespace qtos::input
