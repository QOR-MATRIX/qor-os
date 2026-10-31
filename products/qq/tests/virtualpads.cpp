// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// Built into every suite of QQ and ARQADE, before main(): a suite's controllers are its own, never one on the desk. QQ's
// Input hears every controller QOR.Input reports, so a controller on the machine a suite ran on walked and turned the
// player out of its cases (CI, 30 October 2026). The programs a suite starts inherit it.

#include <QtCore/qbytearrayview.h>
#include <QtCore/qtenvironmentvariables.h>

[[maybe_unused]] static const bool padsAreVirtual = qputenv("QOR_GAMEPADS", "virtual");
