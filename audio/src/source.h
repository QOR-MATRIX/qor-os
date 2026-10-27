// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QString>

namespace qor::audio {

// Something the engine plays: renders interleaved stereo float frames at the
// engine's sample rate. Called on the audio thread.
class Source
{
public:
    virtual ~Source() = default;

    // Renders up to `frames` frames into `out`. Returns the frames rendered:
    // fewer than asked only at the end of the source. A source that is still
    // loading renders silence and reports `buffering()`.
    virtual qint64 render(float *out, qint64 frames) = 0;

    virtual void seek(double seconds) = 0;
    virtual double position() const = 0; // seconds
    virtual double duration() const = 0; // seconds; 0 while unknown
    virtual bool buffering() const { return false; }
    virtual bool failed() const { return false; }
    virtual QString errorString() const { return {}; }
};

} // namespace qor::audio
