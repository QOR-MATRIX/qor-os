// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <complex>
#include <vector>

namespace qtos::audio {

// Iterative radix-2 FFT of a fixed power-of-two size, with precomputed
// twiddles and bit reversal. Real-to-magnitude convenience for analysis.
class Fft
{
public:
    explicit Fft(int size);

    int size() const { return m_size; }

    // In place, forward transform.
    void transform(std::vector<std::complex<float>> &data) const;

    // Windowed (Hann) magnitudes of `size` real samples, normalised so a
    // full-scale sine reads 1.0 at its bin. Writes size / 2 + 1 values.
    void magnitudes(const float *samples, float *out);

private:
    int m_size;
    std::vector<std::complex<float>> m_twiddles;
    std::vector<int> m_reversed;
    std::vector<float> m_window;
    std::vector<std::complex<float>> m_scratch;
};

} // namespace qtos::audio
