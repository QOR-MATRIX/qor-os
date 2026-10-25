// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "engine.h"

#include "backend.h"
#include "filesource.h"
#include "renderer.h"
#include "session.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioSink>
#include <QtMultimedia/QMediaDevices>

#include <algorithm>
#include <memory>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcAudio, "qtos.audio")

namespace qtos::audio {

namespace {

constexpr double kPreferredRate = 48000;
constexpr int kTickMs = 16;      // analysis at the display rate
constexpr int kIdleTickMs = 250; // housekeeping while nothing plays
constexpr int kNullClockMs = 5;
constexpr int kScratchFrames = 4096;
constexpr double kSuspendAfter = 3.0; // seconds of silence before releasing the device

} // namespace

class EngineWorker : public QObject
{
    Q_OBJECT
public:
    explicit EngineWorker(bool null)
        : m_null(null), m_tick(this), m_nullClock(this), m_window(Analyzer::kWindow), m_scratch(kScratchFrames * 8)
    {
        m_tick.setTimerType(Qt::PreciseTimer);
        connect(&m_tick, &QTimer::timeout, this, &EngineWorker::tick);
        m_nullClock.setTimerType(Qt::PreciseTimer);
        m_nullClock.setInterval(kNullClockMs);
        connect(&m_nullClock, &QTimer::timeout, this, &EngineWorker::renderNull);
    }

    void start()
    {
        if (!m_null) {
            m_devices = new QMediaDevices(this);
            connect(m_devices, &QMediaDevices::audioOutputsChanged, this, &EngineWorker::refreshOutputs);
        }
        openOutput();
        m_tickClock.start();
        m_tick.start(kIdleTickMs);
    }

    void shutdown()
    {
        m_tick.stop();
        closeOutput();
        m_renderer.clear();
        m_file = nullptr;
    }

    void load(const QVariantMap &track, double position)
    {
        m_track = track;
        m_hasTrack = true;
        Source *source = createSource(track, position);
        m_renderer.setSource(source, ++m_generation);
        Q_EMIT positionChanged(position);
        m_lastPosition = position;
        wake();
    }

    void unload()
    {
        m_hasTrack = false;
        m_file = nullptr;
        m_renderer.setSource(nullptr, ++m_generation);
        wake();
    }

    void setPlaying(bool playing)
    {
        m_playing = playing;
        m_renderer.setPlaying(playing);
        if (playing)
            wake();
    }

    void seek(double seconds)
    {
        m_renderer.seek(seconds);
        m_lastPosition = seconds;
        Q_EMIT positionChanged(seconds);
        wake();
    }

    void setVolume(double volume) { m_renderer.setVolume(volume); }
    void setMuted(bool muted) { m_renderer.setMuted(muted); }

    void setEqualizer(const QList<double> &gains)
    {
        std::array<double, Equalizer::kBands> array {};
        for (int i = 0; i < Equalizer::kBands && i < gains.size(); ++i)
            array[std::size_t(i)] = gains.at(i);
        m_renderer.setEqualizer(array);
    }

    void setReactiveRange(double low, double high) { m_analyzer.setRange(low, high); }

    void selectOutput(const QString &id)
    {
        m_requestedId = id;
        if (!m_null)
            openOutput();
    }

Q_SIGNALS:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void finished();
    void loadFailed(const QString &message);
    void bufferingChanged(bool buffering);
    void analysisReady(const qtos::audio::AnalysisFrame &frame);
    void outputsChanged(const QList<qtos::audio::OutputDevice> &outputs, const QString &currentId);

private:
    Source *createSource(const QVariantMap &track, double position)
    {
        const double rate = m_renderer.sampleRate();
        if (track.value(u"kind"_s).toString() == u"file"_s) {
            auto *file = new FileSource(track.value(u"path"_s).toString(), rate);
            connect(file, &FileSource::durationChanged, this, [this, file](double seconds) {
                if (file == m_file)
                    Q_EMIT durationChanged(seconds);
            });
            connect(file, &FileSource::decodeFailed, this, [this, file](const QString &message) {
                if (file == m_file)
                    Q_EMIT loadFailed(message);
            });
            if (position > 0)
                file->seek(position);
            m_file = file;
            if (const double known = track.value(u"duration"_s).toDouble(); known > 0)
                Q_EMIT durationChanged(known);
            return file;
        }
        m_file = nullptr;
        const SessionSpec spec =
            SessionSpec::fromVariant(track.value(u"session"_s).toMap(), track.value(u"duration"_s).toDouble(),
                                     track.value(u"title"_s).toString());
        auto *session = new SessionSource(spec, rate);
        if (position > 0)
            session->seek(position);
        Q_EMIT durationChanged(spec.duration);
        return session;
    }

    QList<OutputDevice> outputs() const
    {
        QList<OutputDevice> list;
        if (!m_devices)
            return list;
        for (const QAudioDevice &device : QMediaDevices::audioOutputs())
            list.append({ QString::fromUtf8(device.id()), device.description(), device.isDefault() });
        return list;
    }

    QAudioDevice chooseDevice() const
    {
        if (!m_requestedId.isEmpty()) {
            for (const QAudioDevice &device : QMediaDevices::audioOutputs())
                if (QString::fromUtf8(device.id()) == m_requestedId)
                    return device;
        }
        return QMediaDevices::defaultAudioOutput();
    }

    void refreshOutputs()
    {
        const QAudioDevice wanted = chooseDevice();
        if (QString::fromUtf8(wanted.id()) != m_currentId || (!m_sink && !wanted.isNull()))
            openOutput();
        else
            Q_EMIT outputsChanged(outputs(), m_currentId);
    }

    // The sample rate everything renders at. Changing it rebuilds the track.
    void applyRate(double rate)
    {
        if (std::abs(rate - m_renderer.sampleRate()) < 0.5)
            return;
        const double position = m_renderer.position();
        // No audio thread runs here: the output is closed.
        m_renderer.clear();
        m_file = nullptr;
        m_renderer.setSampleRate(rate);
        m_analyzer.setSampleRate(rate);
        if (m_hasTrack)
            m_renderer.setSource(createSource(m_track, position), ++m_generation);
    }

    void openOutput()
    {
        closeOutput();
        const QAudioDevice device = m_null ? QAudioDevice() : chooseDevice();
        if (device.isNull()) {
            if (!m_null)
                qCInfo(lcAudio) << "No audio output; rendering silently";
            applyRate(kPreferredRate);
            startNull();
            m_currentId.clear();
            Q_EMIT outputsChanged(outputs(), m_currentId);
            return;
        }

        QAudioFormat format;
        format.setSampleRate(int(kPreferredRate));
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Float);
        if (!device.isFormatSupported(format)) {
            format = device.preferredFormat();
            if (format.channelCount() <= 0)
                format.setChannelCount(2);
            format.setSampleFormat(QAudioFormat::Float);
            if (!device.isFormatSupported(format))
                format.setSampleFormat(QAudioFormat::Int16);
        }
        applyRate(format.sampleRate());

        m_sink = std::make_unique<QAudioSink>(device, format);
        connect(m_sink.get(), &QAudioSink::stateChanged, this, [this](QAudio::State state) {
            if (state == QAudio::StoppedState && m_sink && m_sink->error() != QAudio::NoError) {
                qCWarning(lcAudio) << "Audio output stopped:" << m_sink->error() << "- reopening";
                QTimer::singleShot(250, this, &EngineWorker::refreshOutputs);
            }
        });
        const int channels = format.channelCount();
        if (format.sampleFormat() == QAudioFormat::Float) {
            m_sink->start([this, channels](QSpan<float> span) { renderTo(span.data(), span.size(), channels); });
        } else {
            m_sink->start([this, channels](QSpan<qint16> span) { renderTo(span.data(), span.size(), channels); });
        }
        if (m_sink->error() != QAudio::NoError) {
            qCWarning(lcAudio) << "Cannot open" << device.description() << m_sink->error() << "- rendering silently";
            m_sink.reset();
            startNull();
            m_currentId.clear();
        } else {
            m_currentId = QString::fromUtf8(device.id());
            m_suspended = false;
            qCInfo(lcAudio).nospace() << "Output: " << device.description() << ", " << format.sampleRate() << " Hz, "
                                      << channels << " ch, " << format.sampleFormat();
        }
        Q_EMIT outputsChanged(outputs(), m_currentId);
    }

    void closeOutput()
    {
        m_nullClock.stop();
        if (m_sink) {
            m_sink->disconnect(this);
            m_sink->reset(); // returns once the callback no longer runs
            m_sink.reset();
        }
        m_callbackFrames.store(0, std::memory_order_relaxed);
    }

    void startNull()
    {
        m_nullElapsed.start();
        m_nullRendered = 0;
        m_nullClock.start();
        m_suspended = false;
    }

    // Audio thread (the sink's callback).
    template<typename Sample>
    void renderTo(Sample *out, qsizetype samples, int channels)
    {
        qint64 frames = samples / channels;
        m_callbackFrames.store(frames, std::memory_order_relaxed);
        if constexpr (std::is_same_v<Sample, float>) {
            if (channels == 2) {
                m_renderer.process(out, frames);
                return;
            }
        }
        while (frames > 0) {
            const qint64 count = std::min<qint64>(frames, kScratchFrames);
            float *stereo = m_scratch.data();
            m_renderer.process(stereo, count);
            for (qint64 i = 0; i < count; ++i) {
                const float left = stereo[i * 2];
                const float right = stereo[i * 2 + 1];
                for (int c = 0; c < channels; ++c) {
                    const float value = channels == 1 ? 0.5f * (left + right) : c == 0 ? left : c == 1 ? right : 0.0f;
                    if constexpr (std::is_same_v<Sample, float>)
                        out[c] = value;
                    else
                        out[c] = Sample(std::lround(std::clamp(value, -1.0f, 1.0f) * 32767.0f));
                }
                out += channels;
            }
            frames -= count;
        }
    }

    // The null output: renders in real time, to nowhere.
    void renderNull()
    {
        const double rate = m_renderer.sampleRate();
        const qint64 target = qint64(double(m_nullElapsed.nsecsElapsed()) * 1e-9 * rate);
        qint64 due = target - m_nullRendered;
        const qint64 maximum = qint64(rate / 4);
        if (due > maximum) { // after a stall, do not race to catch up
            m_nullRendered += due - maximum;
            due = maximum;
        }
        while (due > 0) {
            const qint64 count = std::min<qint64>(due, kScratchFrames);
            m_renderer.process(m_scratch.data(), count);
            due -= count;
            m_nullRendered += count;
        }
    }

    // Resumes the output if it was released while idle.
    void wake()
    {
        m_silentFor = 0;
        if (m_suspended) {
            if (m_sink)
                m_sink->resume();
            else if (!m_nullClock.isActive())
                startNull();
            m_suspended = false;
        }
        if (m_tick.interval() != kTickMs)
            m_tick.start(kTickMs);
    }

    void tick()
    {
        const double dt = std::clamp(double(m_tickClock.restart()) / 1000.0, 0.001, 0.25);

        while (Source *source = m_renderer.takeRetired())
            delete source;

        if (m_hasTrack) {
            const quint64 finishedGeneration = m_renderer.finishedGeneration();
            if (finishedGeneration == m_generation && finishedGeneration != m_reportedFinished) {
                m_reportedFinished = finishedGeneration;
                Q_EMIT finished();
            }
        }

        const qint64 delay = m_sink ? 2 * m_callbackFrames.load(std::memory_order_relaxed) : 0;
        const double rate = m_renderer.sampleRate();
        if (m_playing && m_hasTrack) {
            const double position = std::max(0.0, m_renderer.position() - double(delay) / rate);
            if (m_positionClock.isValid() == false || m_positionClock.elapsed() >= 100) {
                if (std::abs(position - m_lastPosition) > 0.005) {
                    m_lastPosition = position;
                    Q_EMIT positionChanged(position);
                }
                m_positionClock.start();
            }
        }
        const bool buffering = m_renderer.buffering();
        if (buffering != m_buffering) {
            m_buffering = buffering;
            Q_EMIT bufferingChanged(buffering);
        }

        // Analysis of what is being heard right now.
        const qint64 written = m_renderer.framesRendered();
        const bool fresh = written != m_lastWritten;
        m_lastWritten = written;
        AnalysisFrame frame;
        if (fresh && m_renderer.copyMonitor(m_window.data(), Analyzer::kWindow, delay))
            frame = m_analyzer.analyze(m_window.data(), dt);
        else
            frame = m_analyzer.silence(dt);

        if (frame.active) {
            m_idle = false;
            m_silentFor = 0;
            Q_EMIT analysisReady(frame);
        } else if (!m_idle) {
            const float loudest = *std::max_element(frame.bands.cbegin(), frame.bands.cend());
            if (loudest < 0.004f && frame.reactive < 0.004f && frame.beatStrength < 0.01f) {
                m_analyzer.reset();
                frame = {};
                m_idle = true;
            }
            Q_EMIT analysisReady(frame);
        }

        // Release the device after a few seconds with nothing to play.
        if (!frame.active && !m_playing) {
            m_silentFor += dt;
            if (m_silentFor > kSuspendAfter && !m_suspended && m_idle) {
                if (m_sink)
                    m_sink->suspend();
                else
                    m_nullClock.stop();
                m_suspended = true;
            }
        }
        const int interval = (m_idle && !m_playing) ? kIdleTickMs : kTickMs;
        if (m_tick.interval() != interval)
            m_tick.start(interval);
    }

    Renderer m_renderer { kPreferredRate };
    Analyzer m_analyzer { kPreferredRate };
    const bool m_null;
    QMediaDevices *m_devices = nullptr;
    std::unique_ptr<QAudioSink> m_sink;
    QString m_requestedId;
    QString m_currentId;
    std::atomic<qint64> m_callbackFrames { 0 };
    bool m_suspended = false;
    double m_silentFor = 0;

    // Children, so they move to the engine thread with this object.
    QTimer m_tick;
    QTimer m_nullClock;
    QElapsedTimer m_tickClock;
    QElapsedTimer m_positionClock;
    QElapsedTimer m_nullElapsed;
    qint64 m_nullRendered = 0;

    QVariantMap m_track;
    bool m_hasTrack = false;
    quint64 m_generation = 0;
    quint64 m_reportedFinished = 0;
    FileSource *m_file = nullptr; // the current track when it is a file; owned by the renderer
    bool m_playing = false;
    bool m_buffering = false;
    double m_lastPosition = 0;
    qint64 m_lastWritten = 0;
    bool m_idle = true;

    std::vector<float> m_window;
    std::vector<float> m_scratch; // audio thread
};

// ── Engine ───────────────────────────────────────────────────────────────────

Engine::Engine(Output output, QObject *parent)
    : QObject(parent), m_null(output == Output::Null || qEnvironmentVariable("QTOS_AUDIO_OUTPUT") == u"null"_s)
{
    configureBackend();
    qRegisterMetaType<qtos::audio::AnalysisFrame>();
    qRegisterMetaType<QList<qtos::audio::OutputDevice>>();

    m_worker = new EngineWorker(m_null);
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &EngineWorker::positionChanged, this, &Engine::positionChanged);
    connect(m_worker, &EngineWorker::durationChanged, this, &Engine::durationChanged);
    connect(m_worker, &EngineWorker::finished, this, &Engine::finished);
    connect(m_worker, &EngineWorker::loadFailed, this, &Engine::loadFailed);
    connect(m_worker, &EngineWorker::bufferingChanged, this, &Engine::bufferingChanged);
    connect(m_worker, &EngineWorker::analysisReady, this, &Engine::analysisReady);
    connect(m_worker, &EngineWorker::outputsChanged, this, &Engine::outputsChanged);

    m_thread.setObjectName(u"qtos-audio"_s);
    m_thread.start(QThread::HighPriority);
    QMetaObject::invokeMethod(m_worker, &EngineWorker::start);
}

Engine::~Engine()
{
    QMetaObject::invokeMethod(m_worker, &EngineWorker::shutdown, Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
}

void Engine::load(const QVariantMap &track, double position)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, track, position] { worker->load(track, position); });
}

void Engine::unload() { QMetaObject::invokeMethod(m_worker, &EngineWorker::unload); }

void Engine::setPlaying(bool playing)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, playing] { worker->setPlaying(playing); });
}

void Engine::seek(double seconds)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, seconds] { worker->seek(seconds); });
}

void Engine::setVolume(double volume)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, volume] { worker->setVolume(volume); });
}

void Engine::setMuted(bool muted)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, muted] { worker->setMuted(muted); });
}

void Engine::setEqualizer(const QList<double> &gainsDb)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, gainsDb] { worker->setEqualizer(gainsDb); });
}

void Engine::setReactiveRange(double lowHz, double highHz)
{
    QMetaObject::invokeMethod(m_worker,
                              [worker = m_worker, lowHz, highHz] { worker->setReactiveRange(lowHz, highHz); });
}

void Engine::selectOutput(const QString &id)
{
    QMetaObject::invokeMethod(m_worker, [worker = m_worker, id] { worker->selectOutput(id); });
}

} // namespace qtos::audio

#include "engine.moc"
