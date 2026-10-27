// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "dsp.h"

#include <numbers>

namespace qor::audio {

void Biquad::set(Type type, double sampleRate, double frequency, double q, double gainDb)
{
    const double a = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * std::numbers::pi * frequency / sampleRate;
    const double cosw = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * q);
    double nb0 = 1, nb1 = 0, nb2 = 0, na0 = 1, na1 = 0, na2 = 0;
    switch (type) {
    case Type::Peaking:
        nb0 = 1 + alpha * a;
        nb1 = -2 * cosw;
        nb2 = 1 - alpha * a;
        na0 = 1 + alpha / a;
        na1 = -2 * cosw;
        na2 = 1 - alpha / a;
        break;
    case Type::LowShelf: {
        const double s = 2 * std::sqrt(a) * alpha;
        nb0 = a * ((a + 1) - (a - 1) * cosw + s);
        nb1 = 2 * a * ((a - 1) - (a + 1) * cosw);
        nb2 = a * ((a + 1) - (a - 1) * cosw - s);
        na0 = (a + 1) + (a - 1) * cosw + s;
        na1 = -2 * ((a - 1) + (a + 1) * cosw);
        na2 = (a + 1) + (a - 1) * cosw - s;
        break;
    }
    case Type::HighShelf: {
        const double s = 2 * std::sqrt(a) * alpha;
        nb0 = a * ((a + 1) + (a - 1) * cosw + s);
        nb1 = -2 * a * ((a - 1) + (a + 1) * cosw);
        nb2 = a * ((a + 1) + (a - 1) * cosw - s);
        na0 = (a + 1) - (a - 1) * cosw + s;
        na1 = 2 * ((a - 1) - (a + 1) * cosw);
        na2 = (a + 1) - (a - 1) * cosw - s;
        break;
    }
    }
    b0 = nb0 / na0;
    b1 = nb1 / na0;
    b2 = nb2 / na0;
    a1 = na1 / na0;
    a2 = na2 / na0;
}

void Equalizer::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    update();
}

void Equalizer::setGains(const std::array<double, kBands> &gainsDb)
{
    for (int i = 0; i < kBands; ++i)
        m_gains[i] = std::clamp(gainsDb[i], -kMaxGainDb, kMaxGainDb);
    update();
}

void Equalizer::update()
{
    m_flat = std::all_of(m_gains.cbegin(), m_gains.cend(), [](double g) { return std::abs(g) < 0.05; });
    for (int i = 0; i < kBands; ++i) {
        const Biquad::Type type = i == 0            ? Biquad::Type::LowShelf
                                  : i == kBands - 1 ? Biquad::Type::HighShelf
                                                    : Biquad::Type::Peaking;
        // Shelves use the RBJ "S = 1" slope (Q = 0.707); peaks are an octave
        // and a bit wide, so neighbouring bands blend smoothly.
        m_filters[i].set(type, m_sampleRate, kFrequencies[i], type == Biquad::Type::Peaking ? 1.1 : 0.707, m_gains[i]);
    }
}

void Equalizer::process(float *samples, int frames)
{
    if (m_flat)
        return;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < 2; ++c) {
            float x = samples[f * 2 + c];
            for (int b = 0; b < kBands; ++b)
                x = m_state[b][c].process(m_filters[b], x);
            samples[f * 2 + c] = x;
        }
    }
}

void Equalizer::reset() { m_state = {}; }

void GainRamp::process(float *samples, int frames)
{
    for (int f = 0; f < frames; ++f) {
        if (m_current < m_target)
            m_current = std::min(m_target, m_current + m_step);
        else if (m_current > m_target)
            m_current = std::max(m_target, m_current - m_step);
        samples[f * 2] *= float(m_current);
        samples[f * 2 + 1] *= float(m_current);
    }
}

} // namespace qor::audio
