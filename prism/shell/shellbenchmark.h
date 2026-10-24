// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QList>
#include <QtCore/QString>

class QQuickWindow;

namespace qtos {

struct ShellBenchmarkOptions
{
    QString tool;       // reported as "tool": qtos-shell or prism
    int seconds = 8;    // length of the measured tour
    QString outputPath; // also write the JSON report here when set
};

// The shell benchmark shared by qtos-shell and prism (`--benchmark`).
//
// 1. After a warm-up, drives a scripted tour on the Desktop (objectName
//    "desktop") in the first window: Overview, Beam, Control Center and a
//    Space switch every 750 ms, while qtos::FrameStats records the frame
//    intervals of every window (one per output under prism).
// 2. Measures input-to-swap latency: synthetic pointer moves over the Shelf
//    (objectName "shelf"), timed from posting the event to the swap of the
//    first frame synchronised after the event was handled.
// 3. Prints a JSON report and quits. The top-level fields describe the first
//    window, as in earlier versions; "outputs" has one entry per window, with
//    missed frames judged against that output's own refresh rate.
void runShellBenchmark(const QList<QQuickWindow *> &windows, const ShellBenchmarkOptions &options);

} // namespace qtos
