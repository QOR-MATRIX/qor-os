// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

namespace qtos {

// Passive frame-pacing probe for a QQuickWindow.
//
// Records the interval between presented frames (QQuickWindow::frameSwapped)
// without ever requesting a frame itself, so measuring never changes what is
// measured and an idle shell stays idle. Exposes a smoothed frame time for
// UI (Monitor) and raw percentiles for benchmarks.
class FrameStats : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(double frameMs READ frameMs NOTIFY updated FINAL)
    Q_PROPERTY(double fps READ fps NOTIFY updated FINAL)
    Q_PROPERTY(bool idle READ isIdle NOTIFY updated FINAL)
    Q_PROPERTY(int framesRecorded READ framesRecorded NOTIFY updated FINAL)

public:
    explicit FrameStats(QObject *parent = nullptr);
    ~FrameStats() override;

    // Attach to any item in the window to observe (QML), or a window (C++).
    QQuickItem *item() const { return m_item; }
    void setItem(QQuickItem *item);
    void setWindow(QQuickWindow *window);

    double frameMs() const { return m_frameMs; }
    double fps() const { return m_frameMs > 0 ? 1000.0 / m_frameMs : 0; }
    bool isIdle() const { return m_idle; }
    int framesRecorded() const;

    // Recorded frame intervals in milliseconds, oldest first.
    QList<double> intervals() const;
    // Percentile (0-100) of recorded frame intervals in milliseconds.
    double percentile(double p) const;
    double average() const;
    double maximum() const;
    void reset();

Q_SIGNALS:
    void itemChanged();
    void updated();

private:
    void onFrameSwapped(); // render thread
    void publish();        // GUI thread

    QPointer<QQuickItem> m_item;
    QPointer<QQuickWindow> m_window;
    QMetaObject::Connection m_connection;
    QTimer m_publishTimer;

    mutable QMutex m_mutex;
    QElapsedTimer m_clock;
    qint64 m_lastFrameNs = -1;
    QList<double> m_intervals; // ms, guarded by m_mutex

    double m_frameMs = 0;
    bool m_idle = true;
};

} // namespace qtos
