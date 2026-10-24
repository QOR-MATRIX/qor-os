// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "framestats.h"

#include <algorithm>

namespace qtos {

namespace {
constexpr int kPublishIntervalMs = 250;
constexpr double kIdleGapMs = 200.0; // a gap this long means the scene went idle
constexpr qsizetype kMaxSamples = 20000;
} // namespace

FrameStats::FrameStats(QObject *parent) : QObject(parent)
{
    m_clock.start();
    m_publishTimer.setInterval(kPublishIntervalMs);
    connect(&m_publishTimer, &QTimer::timeout, this, &FrameStats::publish);
}

FrameStats::~FrameStats() { QObject::disconnect(m_connection); }

void FrameStats::setItem(QQuickItem *item)
{
    if (m_item == item)
        return;
    if (m_item)
        disconnect(m_item, &QQuickItem::windowChanged, this, &FrameStats::setWindow);
    m_item = item;
    if (m_item) {
        connect(m_item, &QQuickItem::windowChanged, this, &FrameStats::setWindow);
        setWindow(m_item->window());
    }
    Q_EMIT itemChanged();
}

void FrameStats::setWindow(QQuickWindow *window)
{
    if (m_window == window)
        return;
    QObject::disconnect(m_connection);
    m_window = window;
    reset();
    if (m_window) {
        // frameSwapped is emitted on the render thread; record it there.
        m_connection =
            connect(m_window, &QQuickWindow::frameSwapped, this, &FrameStats::onFrameSwapped, Qt::DirectConnection);
        m_publishTimer.start();
    } else {
        m_publishTimer.stop();
    }
}

void FrameStats::onFrameSwapped()
{
    const qint64 now = m_clock.nsecsElapsed();
    QMutexLocker lock(&m_mutex);
    if (m_lastFrameNs >= 0) {
        const double interval = double(now - m_lastFrameNs) / 1e6;
        if (interval < kIdleGapMs && m_intervals.size() < kMaxSamples)
            m_intervals.append(interval);
    }
    m_lastFrameNs = now;
}

void FrameStats::publish()
{
    double recent = 0;
    bool idle = true;
    {
        QMutexLocker lock(&m_mutex);
        const double sinceLast = m_lastFrameNs < 0 ? 1e9 : double(m_clock.nsecsElapsed() - m_lastFrameNs) / 1e6;
        idle = sinceLast > kIdleGapMs;
        const qsizetype n = std::min<qsizetype>(m_intervals.size(), 30);
        if (n > 0) {
            for (qsizetype i = m_intervals.size() - n; i < m_intervals.size(); ++i)
                recent += m_intervals.at(i);
            recent /= double(n);
        }
    }
    const bool changed = idle != m_idle || (recent > 0 && std::abs(recent - m_frameMs) > 0.05);
    m_idle = idle;
    if (recent > 0)
        m_frameMs = recent;
    if (changed)
        Q_EMIT updated();
}

int FrameStats::framesRecorded() const
{
    QMutexLocker lock(&m_mutex);
    return int(m_intervals.size());
}

QList<double> FrameStats::intervals() const
{
    QMutexLocker lock(&m_mutex);
    return m_intervals;
}

double FrameStats::percentile(double p) const
{
    QList<double> sorted;
    {
        QMutexLocker lock(&m_mutex);
        sorted = m_intervals;
    }
    if (sorted.isEmpty())
        return 0;
    std::sort(sorted.begin(), sorted.end());
    const double rank = std::clamp(p, 0.0, 100.0) / 100.0 * double(sorted.size() - 1);
    return sorted.at(qsizetype(rank + 0.5));
}

double FrameStats::average() const
{
    QMutexLocker lock(&m_mutex);
    if (m_intervals.isEmpty())
        return 0;
    double sum = 0;
    for (double v : m_intervals)
        sum += v;
    return sum / double(m_intervals.size());
}

double FrameStats::maximum() const
{
    QMutexLocker lock(&m_mutex);
    return m_intervals.isEmpty() ? 0 : *std::max_element(m_intervals.cbegin(), m_intervals.cend());
}

void FrameStats::reset()
{
    QMutexLocker lock(&m_mutex);
    m_intervals.clear();
    m_lastFrameNs = -1;
}

} // namespace qtos
