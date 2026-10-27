// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QString>

namespace qor::sim {

// QTOS became QOR OS (QOR-0009), and the application names that settings and
// data are kept under changed with it. Call after the new names are set: the
// first time the new name has no settings, the old name's are copied over, and
// the old data folder (installed Store items among it) is moved to the new
// one. The old settings are left in place.
void carryOverFrom(const QString &oldApplicationName);

} // namespace qor::sim
