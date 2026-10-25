// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "renderer.h"

#include <QtCore/QThread>

#include <algorithm>

namespace qtos::audio {

Renderer::Renderer(double sampleRate) : m_sampleRate(sampleRate), m_monitor(new std::atomic<float>[kMonitorSize])
{
    for (int i = 0; i < kMonitorSize; ++i)
        m_monitor[i].store(0.0f, std::memory_order_relaxed);
    setSampleRate(sampleRate);
}

Renderer::~Renderer() { clear(); }

void Renderer::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    m_eq.setSampleRate(sampleRate);
    m_volumeRamp.setSampleRate(sampleRate);
    m_transportStep = 1.0 / (0.01 * sampleRate); // 10 ms fades
}

void Renderer::pushCommand(const Command &command)
{
    // The audio thread drains the queue every few milliseconds; it is only
    // ever full if the output has stalled.
    while (!m_commands.push(command))
        QThread::yieldCurrentThread();
}

void Renderer::setSource(Source *source, quint64 generation)
{
    pushCommand({ Command::Type::SetSource, source, generation, 0 });
}

void Renderer::seek(double seconds) { pushCommand({ Command::Type::Seek, nullptr, 0, seconds }); }

void Renderer::setEqualizer(const std::array<double, Equalizer::kBands> &gainsDb)
{
    for (int i = 0; i < Equalizer::kBands; ++i)
        m_eqGains[std::size_t(i)].store(gainsDb[std::size_t(i)], std::memory_order_relaxed);
    m_eqVersion.fetch_add(1, std::memory_order_release);
}

Source *Renderer::takeRetired() { return m_retired.take().value_or(nullptr); }

bool Renderer::copyMonitor(float *out, int count, qint64 delay) const
{
    const qint64 written = m_monitorWritten.load(std::memory_order_acquire);
    delay = std::clamp<qint64>(delay, 0, kMonitorSize - count);
    const qint64 start = written - delay - count;
    if (start < 0)
        return false;
    for (int i = 0; i < count; ++i)
        out[i] = m_monitor[(start + i) & (kMonitorSize - 1)].load(std::memory_order_relaxed);
    return true;
}

void Renderer::clear()
{
    while (auto command = m_commands.take())
        delete command->source;
    delete m_source;
    m_source = nullptr;
    while (Source *source = takeRetired())
        delete source;
}

void Renderer::apply(const Command &command)
{
    switch (command.type) {
    case Command::Type::SetSource:
        if (m_source && !m_retired.push(m_source))
            delete m_source; // unreachable in practice: the control thread drains every frame
        m_source = command.source;
        m_generation = command.generation;
        m_ended = false;
        m_position.store(m_source ? m_source->position() : 0.0, std::memory_order_relaxed);
        break;
    case Command::Type::Seek:
        if (m_source) {
            m_source->seek(command.seconds);
            m_ended = false;
            m_position.store(m_source->position(), std::memory_order_relaxed);
        }
        break;
    case Command::Type::None: break;
    }
}

void Renderer::process(float *out, qint64 frames)
{
    while (frames > 0) {
        const int count = int(std::min<qint64>(frames, kBlock));
        processBlock(out, count);
        out += count * 2;
        frames -= count;
    }
}

void Renderer::processBlock(float *out, int frames)
{
    const quint32 eqVersion = m_eqVersion.load(std::memory_order_acquire);
    if (eqVersion != m_appliedEqVersion) {
        std::array<double, Equalizer::kBands> gains {};
        for (int i = 0; i < Equalizer::kBands; ++i)
            gains[std::size_t(i)] = m_eqGains[std::size_t(i)].load(std::memory_order_relaxed);
        m_eq.setGains(gains);
        m_appliedEqVersion = eqVersion;
    }
    m_volumeRamp.setTarget(
        m_muted.load(std::memory_order_relaxed) ? 0.0 : volumeToGain(m_volume.load(std::memory_order_relaxed)));

    // Changes to what plays wait until the output has faded to silence, so
    // seeks and track changes never click.
    bool holding = false;
    while (const Command *command = m_commands.peek()) {
        if (m_transport > 0.0) {
            holding = true;
            break;
        }
        apply(*command);
        m_commands.pop();
    }

    const bool audible = m_playing.load(std::memory_order_relaxed) && !holding && m_source && !m_ended;
    if (m_source && !m_ended && (audible || m_transport > 0.0)) {
        const qint64 rendered = m_source->render(out, frames);
        if (rendered < frames) {
            std::fill(out + rendered * 2, out + qint64(frames) * 2, 0.0f);
            if (!m_source->buffering()) {
                m_ended = true;
                m_finished.store(m_generation, std::memory_order_relaxed);
                m_ends.fetch_add(1, std::memory_order_release);
            }
        }
        m_position.store(m_source->position(), std::memory_order_relaxed);
        m_buffering.store(m_source->buffering(), std::memory_order_relaxed);
    } else {
        std::fill(out, out + qint64(frames) * 2, 0.0f);
    }

    const double target = audible ? 1.0 : 0.0;
    for (int i = 0; i < frames; ++i) {
        if (m_transport < target)
            m_transport = std::min(target, m_transport + m_transportStep);
        else if (m_transport > target)
            m_transport = std::max(target, m_transport - m_transportStep);
        out[i * 2] *= float(m_transport);
        out[i * 2 + 1] *= float(m_transport);
    }

    m_eq.process(out, frames);

    const qint64 written = m_monitorWritten.load(std::memory_order_relaxed);
    for (int i = 0; i < frames; ++i)
        m_monitor[(written + i) & (kMonitorSize - 1)].store(0.5f * (out[i * 2] + out[i * 2 + 1]),
                                                            std::memory_order_relaxed);
    m_monitorWritten.store(written + frames, std::memory_order_release);

    m_volumeRamp.process(out, frames);
    for (int i = 0; i < frames * 2; ++i)
        out[i] = softLimit(out[i]);
}

} // namespace qtos::audio
