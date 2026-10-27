// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QString>

namespace qor::host {

// Points Qt's RHI at the GPU matching `preference` ("high-performance",
// "low-power" or "default") before the first window is created.
//
// On Windows hybrid-graphics machines Qt's Direct3D backends otherwise use
// adapter 0, which is usually the integrated GPU. An explicit
// QT_D3D_ADAPTER_INDEX in the environment always wins. Returns a short
// description of the decision for logging.
QString applyGpuPreference(const QString &preference);

} // namespace qor::host
