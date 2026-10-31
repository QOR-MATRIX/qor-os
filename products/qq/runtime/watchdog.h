// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// A game's logic, or a project's, runs on the thread everything else does: `while (true) {}` would stop the window for
// ever, with unsaved work behind it in the Studio. This watches from another thread for the event loop not coming round,
// and interrupts the JavaScript that is holding it. Logic that keeps doing it is given up on (gaveUp).
//
// Qt's setInterrupted is the one call here made from the other thread; Qt says it may be.

#pragma once

#include <QObject>
#include <QTimer>

#include <atomic>
#include <memory>
#include <thread>

class QJSEngine;

namespace qq {

class LogicWatchdog : public QObject
{
    Q_OBJECT

public:
    /// Watches `engine` from now on: JavaScript holding the thread for `patienceMs` is interrupted.
    explicit LogicWatchdog(QJSEngine *engine, int patienceMs = 15000, QObject *parent = nullptr);
    ~LogicWatchdog() override;

    int interruptions() const { return m_interruptions; }

signals:
    /// Logic was interrupted three times over: it will not stop on its own.
    void gaveUp();

private:
    void beat();

    QJSEngine *m_engine;
    const int m_patienceMs;
    QTimer m_beat;
    std::atomic<qint64> m_lastBeat{0};
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_interrupted{false};
    int m_interruptions = 0;
    std::unique_ptr<std::thread> m_thread;
};

}  // namespace qq
