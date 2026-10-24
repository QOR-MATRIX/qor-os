// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "shellbenchmark.h"

#include "framestats.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QScreen>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <functional>

using namespace Qt::StringLiterals;

namespace qtos {

namespace {

constexpr int kWarmupMs = 2500;
constexpr int kTourStepMs = 750;
constexpr int kLatencySamples = 40;
constexpr int kLatencyIntervalMs = 150;
// A frame is missed when its interval exceeds this many refresh periods.
constexpr double kMissedFrameFactor = 1.5;

double percentileOf(QList<double> values, double p)
{
    if (values.isEmpty())
        return 0;
    std::sort(values.begin(), values.end());
    const double rank = std::clamp(p, 0.0, 100.0) / 100.0 * double(values.size() - 1);
    return values.at(qsizetype(rank + 0.5));
}

QJsonObject distribution(const QList<double> &values)
{
    double sum = 0;
    for (double v : values)
        sum += v;
    const double average = values.isEmpty() ? 0 : sum / double(values.size());
    return {
        { u"averageMs"_s, average },
        { u"p50Ms"_s, percentileOf(values, 50) },
        { u"p95Ms"_s, percentileOf(values, 95) },
        { u"p99Ms"_s, percentileOf(values, 99) },
        { u"maxMs"_s, values.isEmpty() ? 0 : *std::max_element(values.cbegin(), values.cend()) },
    };
}

QString graphicsApiName(QQuickWindow *window)
{
    switch (window->rendererInterface()->graphicsApi()) {
    case QSGRendererInterface::Direct3D11: return u"Direct3D 11"_s;
    case QSGRendererInterface::Direct3D12: return u"Direct3D 12"_s;
    case QSGRendererInterface::Vulkan: return u"Vulkan"_s;
    case QSGRendererInterface::Metal: return u"Metal"_s;
    case QSGRendererInterface::OpenGL: return u"OpenGL"_s;
    default: return u"unknown"_s;
    }
}

QJsonObject outputReport(QQuickWindow *window, const FrameStats *stats)
{
    const QList<double> intervals = stats->intervals();
    const double refreshHz = window->screen() ? window->screen()->refreshRate() : 0;
    const double budgetMs = refreshHz > 0 ? kMissedFrameFactor * 1000.0 / refreshHz : 0;
    const qsizetype missed =
        budgetMs > 0 ? std::count_if(intervals.cbegin(), intervals.cend(), [=](double v) { return v > budgetMs; }) : 0;

    QJsonObject report = distribution(intervals);
    const double average = report.value(u"averageMs"_s).toDouble();
    report.insert(u"screen"_s, window->screen() ? window->screen()->name() : QString());
    report.insert(u"refreshHz"_s, refreshHz);
    report.insert(u"frames"_s, int(intervals.size()));
    report.insert(u"averageFps"_s, average > 0 ? 1000.0 / average : 0.0);
    report.insert(u"missedFrames"_s, int(missed));
    report.insert(u"missedPercent"_s, intervals.isEmpty() ? 0.0 : 100.0 * double(missed) / double(intervals.size()));
    report.insert(u"size"_s, QString(QString::number(window->width()) + u'x' + QString::number(window->height())));
    report.insert(u"devicePixelRatio"_s, window->devicePixelRatio());
    return report;
}

// Times pointer input to the frame that shows its effect.
//
// A sample is: post a mouse move over `target` (t0), note when the window
// handles it, mark the first frame whose synchronisation starts after that,
// and record t0 → that frame's swap. Samples whose move changed nothing (no
// frame followed) are dropped.
class InputLatencyProbe : public QObject
{
public:
    InputLatencyProbe(QQuickWindow *window, QQuickItem *target) : QObject(window), m_window(window), m_target(target)
    {
        m_clock.start();
        window->installEventFilter(this);
        connect(
            window, &QQuickWindow::beforeSynchronizing, this,
            [this] {
                if (m_handledNs.load() >= 0)
                    m_frameArmed.store(true);
            },
            Qt::DirectConnection);
        connect(
            window, &QQuickWindow::frameSwapped, this,
            [this] {
                if (!m_frameArmed.exchange(false))
                    return;
                const qint64 posted = m_postedNs.exchange(-1);
                m_handledNs.store(-1);
                if (posted >= 0) {
                    QMutexLocker lock(&m_mutex);
                    m_samples.append(double(m_clock.nsecsElapsed() - posted) / 1e6);
                }
            },
            Qt::DirectConnection);
    }

    void run(int samples, int intervalMs, std::function<void()> done)
    {
        auto *timer = new QTimer(this);
        timer->setInterval(intervalMs);
        connect(timer, &QTimer::timeout, this, [this, timer, samples, done] {
            if (m_posted >= samples) {
                timer->stop();
                done();
                return;
            }
            post();
        });
        timer->start();
    }

    QList<double> samples() const
    {
        QMutexLocker lock(&m_mutex);
        return m_samples;
    }

    int posted() const { return m_posted; }

protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::MouseMove && m_pending > 0) {
            --m_pending;
            m_handledNs.store(m_clock.nsecsElapsed());
        }
        return false;
    }

private:
    void post()
    {
        if (!m_target)
            return;
        // Alternate between two points so the Shelf's magnification changes.
        const qreal fraction = (m_posted % 2 == 0) ? 0.3 : 0.7;
        const QPointF local = m_target->mapToScene(QPointF(m_target->width() * fraction, m_target->height() / 2));
        m_postedNs.store(m_clock.nsecsElapsed());
        m_handledNs.store(-1);
        m_frameArmed.store(false);
        ++m_pending;
        ++m_posted;
        QCoreApplication::postEvent(m_window, new QMouseEvent(QEvent::MouseMove, local, m_window->mapToGlobal(local),
                                                              Qt::NoButton, Qt::NoButton, Qt::NoModifier));
    }

    QPointer<QQuickWindow> m_window;
    QPointer<QQuickItem> m_target;
    QElapsedTimer m_clock;
    std::atomic<qint64> m_postedNs { -1 };
    std::atomic<qint64> m_handledNs { -1 };
    std::atomic<bool> m_frameArmed { false };
    int m_pending = 0;
    int m_posted = 0;
    mutable QMutex m_mutex;
    QList<double> m_samples;
};

} // namespace

void runShellBenchmark(const QList<QQuickWindow *> &windows, const ShellBenchmarkOptions &options)
{
    if (windows.isEmpty()) {
        std::fputs("benchmark: no window to measure\n", stderr);
        QCoreApplication::exit(2);
        return;
    }
    QQuickWindow *primary = windows.constFirst();
    QObject *desktop = primary->findChild<QObject *>(u"desktop"_s);
    auto *shelf = primary->findChild<QQuickItem *>(u"shelf"_s);

    QList<FrameStats *> stats;
    for (QQuickWindow *window : windows) {
        auto *probe = new FrameStats(window);
        probe->setWindow(window);
        stats.append(probe);
    }

    struct Step
    {
        const char *method;
        QVariant argument;
    };
    static const QList<Step> script = {
        { "setSurface", u"overview"_s },
        { "setSurface", QString() },
        { "setSurface", u"beam"_s },
        { "setSurface", QString() },
        { "setSurface", u"control-center"_s },
        { "setSurface", QString() },
        { "switchSpace", 1 },
        { "switchSpace", 0 },
    };

    auto *tour = new QTimer(primary);
    tour->setInterval(kTourStepMs);
    QObject::connect(tour, &QTimer::timeout, primary, [desktop, tour] {
        if (!desktop)
            return;
        const int index = tour->property("step").toInt();
        const Step &step = script.at(index % script.size());
        if (step.argument.typeId() == QMetaType::Int)
            QMetaObject::invokeMethod(desktop, step.method, Q_ARG(int, step.argument.toInt()));
        else
            QMetaObject::invokeMethod(desktop, step.method, Q_ARG(QString, step.argument.toString()));
        tour->setProperty("step", index + 1);
    });

    const auto finish = [options, primary](const QJsonArray &outputs, const QList<double> &latency, int latencyPosted) {
        // Top-level fields describe the first output, as before "outputs" existed.
        QJsonObject report = outputs.first().toObject();
        report.insert(u"tool"_s, options.tool);
        report.insert(u"graphicsApi"_s, graphicsApiName(primary));
        report.insert(u"seconds"_s, options.seconds);
        report.insert(u"outputs"_s, outputs);
        QJsonObject input = distribution(latency);
        input.insert(u"samples"_s, int(latency.size()));
        input.insert(u"posted"_s, latencyPosted);
        report.insert(u"inputToSwap"_s, input);

        const QByteArray json = QJsonDocument(report).toJson(QJsonDocument::Indented);
        if (!options.outputPath.isEmpty()) {
            QFile file(options.outputPath);
            if (file.open(QIODevice::WriteOnly))
                file.write(json);
        }
        std::fputs(json.constData(), stdout);
        std::fflush(stdout);
        QCoreApplication::exit(0);
    };

    QTimer::singleShot(kWarmupMs, primary, [stats, tour] {
        for (FrameStats *probe : stats)
            probe->reset();
        tour->start();
    });
    QTimer::singleShot(kWarmupMs + options.seconds * 1000, primary, [windows, primary, shelf, tour, stats, finish] {
        tour->stop();
        // Snapshot the tour's frame statistics; the latency phase is separate.
        QJsonArray outputs;
        for (qsizetype i = 0; i < windows.size(); ++i)
            outputs.append(outputReport(windows.at(i), stats.at(i)));
        if (!shelf) {
            finish(outputs, {}, 0);
            return;
        }
        auto *probe = new InputLatencyProbe(primary, shelf);
        probe->run(kLatencySamples, kLatencyIntervalMs,
                   [probe, outputs, finish] { finish(outputs, probe->samples(), probe->posted()); });
    });
}

} // namespace qtos
