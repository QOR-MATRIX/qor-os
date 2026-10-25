// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "dsp.h"
#include "source.h"
#include "spsc.h"

#include <QtCore/QtTypes>

#include <array>
#include <atomic>
#include <memory>

namespace qtos::audio {

// The real-time half of the engine. One control thread talks to it; one audio
// thread (the QAudioSink callback, or the null output's clock) calls
// process(). Nothing on the audio side locks, allocates or frees: sources are
// handed over through a queue and handed back for deletion through another.
class Renderer
{
public:
    static constexpr int kMonitorSize = 1 << 15; // mono history, ~0.68 s at 48 kHz

    explicit Renderer(double sampleRate = 48000);
    ~Renderer();

    // ── Control thread ──────────────────────────────────────────────────────
    // Only while no audio thread is running.
    void setSampleRate(double sampleRate);
    double sampleRate() const { return m_sampleRate; }

    // Takes ownership. The previous source comes back through takeRetired().
    // `generation` identifies the source in finishedGeneration().
    void setSource(Source *source, quint64 generation);
    void seek(double seconds);
    void setPlaying(bool playing) { m_playing.store(playing, std::memory_order_relaxed); }
    void setVolume(double volume) { m_volume.store(volume, std::memory_order_relaxed); }
    void setMuted(bool muted) { m_muted.store(muted, std::memory_order_relaxed); }
    void setEqualizer(const std::array<double, Equalizer::kBands> &gainsDb);

    // A source the audio thread no longer uses, to delete on the control thread.
    Source *takeRetired();

    // What the audio thread last did.
    double position() const { return m_position.load(std::memory_order_relaxed); }
    quint64 finishedGeneration() const { return m_finished.load(std::memory_order_acquire); }
    bool buffering() const { return m_buffering.load(std::memory_order_relaxed); }
    qint64 framesRendered() const { return m_monitorWritten.load(std::memory_order_acquire); }

    // Copies `count` mono samples (post-EQ, pre-volume) ending `delay` frames
    // before the newest. Returns false when that much history is not there yet.
    bool copyMonitor(float *out, int count, qint64 delay) const;

    // Deletes every source still owned here, including queued ones. Only while
    // no audio thread is running.
    void clear();

    // ── Audio thread ────────────────────────────────────────────────────────
    // Renders interleaved stereo.
    void process(float *out, qint64 frames);

private:
    struct Command
    {
        enum class Type { None, SetSource, Seek } type = Type::None;
        Source *source = nullptr;
        quint64 generation = 0;
        double seconds = 0;
    };

    void pushCommand(const Command &command);
    void apply(const Command &command);
    void processBlock(float *out, int frames);

    static constexpr int kBlock = 256;

    double m_sampleRate;
    SpscQueue<Command, 64> m_commands;
    SpscQueue<Source *, 64> m_retired;

    std::atomic<bool> m_playing { false };
    std::atomic<double> m_volume { 0.6 };
    std::atomic<bool> m_muted { false };
    std::array<std::atomic<double>, Equalizer::kBands> m_eqGains {};
    std::atomic<quint32> m_eqVersion { 0 };

    std::atomic<double> m_position { 0 };
    std::atomic<quint64> m_finished { 0 };
    std::atomic<bool> m_buffering { false };

    std::unique_ptr<std::atomic<float>[]> m_monitor;
    std::atomic<qint64> m_monitorWritten { 0 };

    // Audio thread only.
    Source *m_source = nullptr;
    quint64 m_generation = 0;
    bool m_ended = false;
    Equalizer m_eq;
    quint32 m_appliedEqVersion = 0;
    GainRamp m_volumeRamp;
    double m_transport = 0; // fade for play/pause, seeks and source changes
    double m_transportStep = 1.0 / 480;
};

} // namespace qtos::audio
