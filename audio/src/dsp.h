// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

// Signal-processing building blocks for the QTOS audio engine. Everything here
// is allocation-free and safe to run on the audio thread.
namespace qtos::audio {

inline double dbToGain(double db) { return std::pow(10.0, db / 20.0); }
inline double gainToDb(double gain) { return 20.0 * std::log10(std::max(gain, 1e-12)); }

// RBJ "Audio EQ Cookbook" biquad coefficients.
struct Biquad
{
    enum class Type { LowShelf, Peaking, HighShelf };

    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

    void set(Type type, double sampleRate, double frequency, double q, double gainDb);
};

// Per-channel state for a Biquad (transposed direct form II).
struct BiquadState
{
    double z1 = 0;
    double z2 = 0;

    float process(const Biquad &c, float input)
    {
        const double x = input;
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return float(y);
    }
};

// Five-band stereo equaliser: low shelf, three peaks, high shelf.
class Equalizer
{
public:
    static constexpr int kBands = 5;
    static constexpr std::array<double, kBands> kFrequencies { 60, 250, 1000, 4000, 12000 };
    static constexpr double kMaxGainDb = 12;

    void setSampleRate(double sampleRate);
    // Gains in dB, clamped to ±12 dB.
    void setGains(const std::array<double, kBands> &gainsDb);
    const std::array<double, kBands> &gains() const { return m_gains; }
    bool isFlat() const { return m_flat; }

    // In place, interleaved stereo.
    void process(float *samples, int frames);
    void reset();

private:
    void update();

    double m_sampleRate = 48000;
    std::array<double, kBands> m_gains {};
    std::array<Biquad, kBands> m_filters {};
    std::array<std::array<BiquadState, 2>, kBands> m_state {};
    bool m_flat = true;
};

// A smoothly ramped gain (volume, mute) without zipper noise.
class GainRamp
{
public:
    void setTarget(double gain) { m_target = gain; }
    void setSampleRate(double sampleRate) { m_step = 1.0 / (0.02 * sampleRate); } // 20 ms full-scale ramp
    void jump() { m_current = m_target; }

    // In place, interleaved stereo.
    void process(float *samples, int frames);

private:
    double m_current = 1;
    double m_target = 1;
    double m_step = 1.0 / 960;
};

// Soft-knee clipper: transparent below the knee, then saturates smoothly
// towards the ceiling, so EQ boosts and loud masters never clip hard.
inline float softLimit(float x)
{
    constexpr float knee = 0.85f;
    constexpr float range = 0.98f - knee;
    const float magnitude = std::abs(x);
    if (magnitude <= knee)
        return x;
    const float limited = knee + range * std::tanh((magnitude - knee) / range);
    return std::copysign(limited, x);
}

// Perceptual volume curve, the cubic mapping PulseAudio uses: equal steps of
// the control sound roughly like equal steps of loudness.
inline double volumeToGain(double volume)
{
    volume = std::clamp(volume, 0.0, 1.0);
    return volume * volume * volume;
}

} // namespace qtos::audio
