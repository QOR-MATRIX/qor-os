// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "fft.h"

#include <cmath>
#include <numbers>

namespace qor::audio {

Fft::Fft(int size) : m_size(size), m_twiddles(size / 2), m_reversed(size), m_window(size), m_scratch(size)
{
    int bits = 0;
    while ((1 << bits) < size)
        ++bits;
    for (int i = 0; i < size; ++i) {
        int r = 0;
        for (int b = 0; b < bits; ++b)
            r |= ((i >> b) & 1) << (bits - 1 - b);
        m_reversed[i] = r;
    }
    for (int k = 0; k < size / 2; ++k)
        m_twiddles[k] = std::polar(1.0f, float(-2.0 * std::numbers::pi * k / size));
    for (int n = 0; n < size; ++n)
        m_window[n] = float(0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * n / (size - 1)));
}

void Fft::transform(std::vector<std::complex<float>> &data) const
{
    for (int i = 0; i < m_size; ++i)
        if (i < m_reversed[i])
            std::swap(data[i], data[m_reversed[i]]);
    for (int length = 2; length <= m_size; length <<= 1) {
        const int half = length / 2;
        const int stride = m_size / length;
        for (int start = 0; start < m_size; start += length) {
            for (int k = 0; k < half; ++k) {
                const std::complex<float> t = m_twiddles[k * stride] * data[start + k + half];
                data[start + k + half] = data[start + k] - t;
                data[start + k] += t;
            }
        }
    }
}

void Fft::magnitudes(const float *samples, float *out)
{
    for (int n = 0; n < m_size; ++n)
        m_scratch[n] = { samples[n] * m_window[n], 0.0f };
    transform(m_scratch);
    // A Hann window has a coherent gain of 0.5; a real sine splits its energy
    // between the positive and negative bins, hence 2 / (N * 0.5).
    const float scale = 4.0f / float(m_size);
    for (int k = 0; k <= m_size / 2; ++k)
        out[k] = std::abs(m_scratch[k]) * scale;
}

} // namespace qor::audio
