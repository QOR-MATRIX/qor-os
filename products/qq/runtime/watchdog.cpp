// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "watchdog.h"

#include <QJSEngine>

#include <algorithm>
#include <chrono>

namespace qq {

namespace {

qint64 nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}  // namespace

LogicWatchdog::LogicWatchdog(QJSEngine *engine, int patienceMs, QObject *parent)
    : QObject(parent), m_engine(engine), m_patienceMs(patienceMs)
{
    m_lastBeat = nowMs();
    m_beat.setInterval(std::max(50, patienceMs / 20));
    connect(&m_beat, &QTimer::timeout, this, &LogicWatchdog::beat);
    m_beat.start();
    m_thread = std::make_unique<std::thread>([this] {
        while (!m_stop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::max(25, m_patienceMs / 40)));
            if (!m_interrupted && nowMs() - m_lastBeat > m_patienceMs) {
                m_interrupted = true;
                m_engine->setInterrupted(true);
            }
        }
    });
}

LogicWatchdog::~LogicWatchdog()
{
    m_stop = true;
    if (m_thread)
        m_thread->join();
    if (m_interrupted)
        m_engine->setInterrupted(false);
}

void LogicWatchdog::beat()
{
    m_lastBeat = nowMs();
    if (!m_interrupted)
        return;
    // The thread is back: JavaScript may run again.
    m_engine->setInterrupted(false);
    m_interrupted = false;
    ++m_interruptions;
    qWarning("QQ interrupted logic that held the thread for %d ms.", m_patienceMs);
    if (m_interruptions == 3)
        emit gaveUp();
}

}  // namespace qq
