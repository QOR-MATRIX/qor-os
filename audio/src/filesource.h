// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "source.h"

#include <QtCore/QObject>
#include <QtCore/QString>

#include <atomic>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QAudioBuffer;
class QAudioDecoder;
QT_END_NAMESPACE

namespace qtos::audio {

// A music file decoded (QAudioDecoder, FFmpeg) into memory on the thread that
// owns this object, and played from the audio thread while decoding is still
// under way. Decoded audio is stored as 16-bit stereo in fixed chunks that are
// published lock-free, so render() never waits.
class FileSource : public QObject, public Source
{
    Q_OBJECT
public:
    FileSource(const QString &path, double sampleRate, QObject *parent = nullptr);
    ~FileSource() override;

    // Audio thread.
    qint64 render(float *out, qint64 frames) override;
    void seek(double seconds) override;
    double position() const override;
    double duration() const override;
    bool buffering() const override { return m_buffering.load(std::memory_order_relaxed); }
    bool failed() const override { return m_failed.load(std::memory_order_acquire); }

    // Owning thread.
    QString errorString() const override { return m_error; }
    QString path() const { return m_path; }

Q_SIGNALS:
    void durationChanged(double seconds);
    void decodeFailed(const QString &message);

private:
    void readBuffer();
    void append(const float *stereo, qint64 frames);
    void finish();
    void fail(const QString &message);

    static constexpr qint64 kChunkFrames = 1 << 16;
    static constexpr int kMaxChunks = 4096; // ~93 minutes at 48 kHz

    QString m_path;
    double m_sampleRate;
    QAudioDecoder *m_decoder = nullptr;
    QString m_error;

    // Sized once; slot i is written before m_decoded covers it.
    std::vector<std::unique_ptr<qint16[]>> m_chunks;
    std::atomic<qint64> m_decoded { 0 };  // frames available
    std::atomic<qint64> m_position { 0 }; // next frame to render
    std::atomic<qint64> m_expected { 0 }; // frames, from the container's duration
    std::atomic<bool> m_complete { false };
    std::atomic<bool> m_failed { false };
    std::atomic<bool> m_buffering { true };

    // Streaming linear resampler, for decoders that ignore the requested rate.
    double m_resamplePhase = 0;
    float m_previous[2] {};
    bool m_havePrevious = false;
    std::vector<float> m_scratch;
};

} // namespace qtos::audio
