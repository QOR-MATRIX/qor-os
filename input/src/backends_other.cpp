// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Platforms without a controller backend yet (macOS: GameController.framework
// is the planned one). Virtual controllers still work.

#include "backends.h"

namespace qor::input {

std::vector<std::unique_ptr<Backend>> createHardwareBackends() { return {}; }

} // namespace qor::input
