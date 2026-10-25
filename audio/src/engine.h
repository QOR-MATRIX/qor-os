// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "analyzer.h"

#include <QtCore/QList>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QVariantMap>

namespace qtos::audio {

struct OutputDevice
{
    QString id; // empty: no device (the null output)
    QString name;
    bool isDefault = false;
};

class EngineWorker;

// The QTOS audio engine: plays generative sessions and music files through
// the system output, with a five-band equaliser, perceptual volume, a soft
// limiter, and live analysis of what is being heard for the audio-reactive
// interface.
//
// All work happens off the calling thread: control and decoding on an engine
// thread, mixing on the output's real-time thread (QAudioSink's callback
// interface). This object is the thread-safe front; its signals arrive on the
// thread that created it.
//
// QTOS_AUDIO_OUTPUT=null (or Output::Null) renders in real time to nowhere:
// playback, positions and analysis behave exactly as with a device, which is
// what tests and headless runs use.
class Engine : public QObject
{
    Q_OBJECT
public:
    enum class Output { Auto, Null };

    explicit Engine(Output output = Output::Auto, QObject *parent = nullptr);
    ~Engine() override;

    // A media queue entry: { "kind": "session" | "file", "path": string,
    // "session": { style, mode, bpm, root, seed }, "title": string,
    // "duration": seconds }. Starts paused or playing per setPlaying().
    void load(const QVariantMap &track, double position = 0);
    void unload();
    void setPlaying(bool playing);
    void seek(double seconds);

    void setVolume(double volume); // 0..1, perceptual
    void setMuted(bool muted);
    void setEqualizer(const QList<double> &gainsDb);
    void setReactiveRange(double lowHz, double highHz);

    // Empty id: follow the system default output.
    void selectOutput(const QString &id);

    bool isNullOutput() const { return m_null; }

Q_SIGNALS:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void finished();
    void loadFailed(const QString &message);
    void bufferingChanged(bool buffering);
    void analysisReady(const qtos::audio::AnalysisFrame &frame);
    void outputsChanged(const QList<qtos::audio::OutputDevice> &outputs, const QString &currentId);

private:
    QThread m_thread;
    EngineWorker *m_worker;
    bool m_null;
};

} // namespace qtos::audio

Q_DECLARE_METATYPE(qtos::audio::AnalysisFrame)
Q_DECLARE_METATYPE(qtos::audio::OutputDevice)
