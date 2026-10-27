// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "analyzer.h"

#include "dsp.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace qor::audio {

namespace {

constexpr double kLowestBandHz = 30;
constexpr double kHighestBandHz = 16000;
// Display range: -70 dB reads 0, -8 dB reads 1 (after the tilt).
constexpr double kDisplayFloorDb = -70;
constexpr double kDisplayRangeDb = 62;
// Range energy is normalised against its recent peak over this many dB.
constexpr double kReactiveSpanDb = 24;
constexpr double kPeakDecayDbPerSecond = 4;
constexpr int kFluxHistory = 64;     // about a second at 60 Hz
constexpr double kMinBeatGap = 0.12; // seconds (500 BPM)

double smoothingFactor(double dt, double seconds) { return 1.0 - std::exp(-dt / seconds); }

} // namespace

Analyzer::Analyzer(double sampleRate)
    : m_sampleRate(sampleRate), m_magnitudes(kWindow / 2 + 1), m_previousLog(kWindow / 2 + 1)
{ }

void Analyzer::setSampleRate(double sampleRate)
{
    m_sampleRate = sampleRate;
    reset();
}

void Analyzer::setRange(double lowHz, double highHz)
{
    m_low = std::clamp(std::min(lowHz, highHz), 20.0, 20000.0);
    m_high = std::clamp(std::max(lowHz, highHz), m_low + 1.0, 20000.0);
    m_fluxHistory.clear();
}

std::array<double, AnalysisFrame::kBands + 1> Analyzer::bandEdges()
{
    std::array<double, AnalysisFrame::kBands + 1> edges {};
    for (int i = 0; i <= AnalysisFrame::kBands; ++i)
        edges[i] = kLowestBandHz * std::pow(kHighestBandHz / kLowestBandHz, double(i) / AnalysisFrame::kBands);
    return edges;
}

void Analyzer::reset()
{
    m_frame = {};
    m_peakDb = -60;
    m_fluxHistory.clear();
    std::fill(m_previousLog.begin(), m_previousLog.end(), 0.0f);
    m_sinceBeat = 1;
}

AnalysisFrame Analyzer::analyze(const float *samples, double dt)
{
    // Loudness.
    double sum = 0;
    for (int n = 0; n < kWindow; ++n)
        sum += double(samples[n]) * samples[n];
    const double levelDb = gainToDb(std::sqrt(sum / kWindow));
    m_frame.level = float(std::clamp((levelDb + 60.0) / 60.0, 0.0, 1.0));
    m_frame.active = levelDb > -62.0;

    m_fft.magnitudes(samples, m_magnitudes.data());
    const int lastBin = kWindow / 2;

    // Display spectrum: the peak magnitude in each band (interpolated at the
    // band centre when the band is narrower than a bin), with a +3 dB/octave
    // tilt around 1 kHz.
    static const auto edges = bandEdges();
    const double rise = smoothingFactor(dt, 0.012);
    const double fall = smoothingFactor(dt, 0.18);
    for (int b = 0; b < AnalysisFrame::kBands; ++b) {
        const double lo = edges[b] / binHz();
        const double hi = edges[b + 1] / binHz();
        float magnitude = 0;
        const int first = int(std::ceil(lo));
        const int last = std::min(lastBin, int(std::floor(hi)));
        if (last >= first) {
            for (int k = first; k <= last; ++k)
                magnitude = std::max(magnitude, m_magnitudes[k]);
        } else {
            const double centre = std::sqrt(lo * hi);
            const int k = std::min(lastBin - 1, int(centre));
            const double fraction = centre - k;
            magnitude = float(m_magnitudes[k] * (1 - fraction) + m_magnitudes[k + 1] * fraction);
        }
        const double centreHz = std::sqrt(edges[b] * edges[b + 1]);
        const double db = gainToDb(magnitude) + 3.0 * std::log2(centreHz / 1000.0);
        const float target = float(std::clamp((db - kDisplayFloorDb) / kDisplayRangeDb, 0.0, 1.0));
        float &value = m_frame.bands[b];
        value += float((target - value) * (target > value ? rise : fall));
    }

    // Energy in the chosen range, normalised against its recent peak so quiet
    // and loud material both move the interface.
    const int kLo = std::clamp(int(std::floor(m_low / binHz())), 1, lastBin);
    const int kHi = std::clamp(int(std::ceil(m_high / binHz())), kLo, lastBin);
    double energy = 0;
    for (int k = kLo; k <= kHi; ++k)
        energy += double(m_magnitudes[k]) * m_magnitudes[k];
    const double rangeDb = 10.0 * std::log10(energy + 1e-12);
    m_peakDb = std::max({ rangeDb, m_peakDb - kPeakDecayDbPerSecond * dt, -60.0 });
    double normalised = std::clamp((rangeDb - (m_peakDb - kReactiveSpanDb)) / kReactiveSpanDb, 0.0, 1.0);
    if (rangeDb < -75.0)
        normalised = 0;
    if (normalised > m_frame.reactive)
        m_frame.reactive = float(normalised);
    else
        m_frame.reactive += float((normalised - m_frame.reactive) * smoothingFactor(dt, 0.14));

    // Beats: positive spectral flux of log magnitudes in the range, against an
    // adaptive threshold from the last second.
    double flux = 0;
    for (int k = kLo; k <= kHi; ++k) {
        const float logMagnitude = std::log1p(100.0f * m_magnitudes[k]);
        flux += std::max(0.0f, logMagnitude - m_previousLog[k]);
    }
    for (int k = 0; k <= lastBin; ++k)
        m_previousLog[k] = std::log1p(100.0f * m_magnitudes[k]);
    flux /= double(kHi - kLo + 1);

    m_sinceBeat += dt;
    m_frame.beatStrength *= float(std::exp(-dt / 0.25));
    if (m_fluxHistory.size() >= 10 && rangeDb > -60.0) {
        const double mean =
            std::accumulate(m_fluxHistory.cbegin(), m_fluxHistory.cend(), 0.0) / double(m_fluxHistory.size());
        double variance = 0;
        for (float f : m_fluxHistory)
            variance += (f - mean) * (f - mean);
        const double deviation = std::sqrt(variance / double(m_fluxHistory.size()));
        const double threshold = mean + std::max(1.5 * deviation, 0.3 * mean);
        if (flux > threshold && m_sinceBeat > kMinBeatGap) {
            ++m_frame.beat;
            m_frame.beatStrength = float(std::clamp((flux - mean) / (3.0 * mean + 1e-6), 0.2, 1.0));
            m_sinceBeat = 0;
        }
    }
    m_fluxHistory.push_back(float(flux));
    if (m_fluxHistory.size() > kFluxHistory)
        m_fluxHistory.erase(m_fluxHistory.begin());
    return m_frame;
}

AnalysisFrame Analyzer::silence(double dt)
{
    const double fall = smoothingFactor(dt, 0.18);
    for (float &value : m_frame.bands)
        value -= float(value * fall);
    m_frame.reactive -= float(m_frame.reactive * smoothingFactor(dt, 0.14));
    m_frame.level = 0;
    m_frame.beatStrength *= float(std::exp(-dt / 0.25));
    m_frame.active = false;
    m_fluxHistory.clear();
    return m_frame;
}

} // namespace qor::audio
