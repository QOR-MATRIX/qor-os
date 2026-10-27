// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "source.h"

#include <QtCore/QVariantMap>

#include <memory>

namespace qor::audio {

// A generative "QOR Session": original music synthesised in real time, so
// QOR OS has sound out of the box without shipping recordings. Deterministic
// for a given spec.
struct SessionSpec
{
    QString style = QStringLiteral("drive"); // drive | pulse | chill | ambient
    QString mode = QStringLiteral("minor");  // minor | dorian | major | lydian
    double bpm = 112;
    int root = 57; // MIDI note of the tonic (57 = A3)
    quint32 seed = 1;
    double duration = 210; // seconds

    // From a queue entry's "session" object ({style, mode, bpm, root, seed});
    // anything missing is derived from the title.
    static SessionSpec fromVariant(const QVariantMap &session, double duration, const QString &title);
};

class SessionSource : public Source
{
public:
    SessionSource(const SessionSpec &spec, double sampleRate);
    ~SessionSource() override;

    qint64 render(float *out, qint64 frames) override;
    void seek(double seconds) override;
    double position() const override;
    double duration() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace qor::audio
