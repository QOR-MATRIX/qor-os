// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "fft.h"

#include <array>
#include <vector>

namespace qtos::audio {

// What the interface reacts to, for one moment of the audio being heard.
struct AnalysisFrame
{
    static constexpr int kBands = 48;

    // Spectrum for display: 48 log-spaced bands from 30 Hz to 16 kHz, 0..1,
    // tilted +3 dB/octave so typical music reads level across the range.
    std::array<float, kBands> bands {};
    // Overall loudness, 0..1 (-60 dBFS .. 0 dBFS RMS).
    float level = 0;
    // Energy in the chosen frequency range (setRange), adaptively normalised
    // against its recent peak and smoothed: 0..1. This drives the interface.
    float reactive = 0;
    // Counts detected beats (onsets in the chosen range); it increases on each.
    int beat = 0;
    // How strongly the last beat stood out, 0..1.
    float beatStrength = 0;
    // True while there is audible signal.
    bool active = false;
};

// Spectrum, loudness, range energy and beat detection over a sliding window.
class Analyzer
{
public:
    static constexpr int kWindow = 2048;

    explicit Analyzer(double sampleRate = 48000);

    void setSampleRate(double sampleRate);
    // The frequency range the interface reacts to, in Hz.
    void setRange(double lowHz, double highHz);
    double rangeLow() const { return m_low; }
    double rangeHigh() const { return m_high; }

    // Analyses `kWindow` mono samples (oldest first); `dt` is the time since
    // the previous call, which sets the smoothing.
    AnalysisFrame analyze(const float *samples, double dt);
    // Lets everything decay to silence (no signal this tick).
    AnalysisFrame silence(double dt);
    void reset();

    // Band edges used for the display spectrum (kBands + 1 values, Hz).
    static std::array<double, AnalysisFrame::kBands + 1> bandEdges();

private:
    double binHz() const { return m_sampleRate / kWindow; }

    double m_sampleRate;
    double m_low = 40;
    double m_high = 160;
    Fft m_fft { kWindow };
    std::vector<float> m_magnitudes;
    std::vector<float> m_previousLog;
    AnalysisFrame m_frame;
    double m_peakDb = -60;
    std::vector<float> m_fluxHistory;
    double m_sinceBeat = 1;
};

} // namespace qtos::audio
