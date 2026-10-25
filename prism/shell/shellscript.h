// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QString>

class QQuickWindow;

namespace qtos {

// Runs a JSON UI script against the shell in `window` (`--script` in
// qtos-shell and prism), then quits: exit code 0 when every step succeeded,
// 4 when one failed (the reason goes to stderr).
//
// Input is synthesised as real Qt mouse and key events on the window, so it
// takes the same path as a person's: shell handlers, and under Prism on to
// Wayland clients. The format is documented in docs/DEVELOPMENT.md
// ("Shell scripts"); tools/dev/prism-e2e.py drives Prism with it.
void runShellScript(QQuickWindow *window, const QString &path);

} // namespace qtos
