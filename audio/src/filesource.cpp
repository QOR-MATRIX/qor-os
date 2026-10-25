// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "filesource.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QUrl>
#include <QtMultimedia/QAudioBuffer>
#include <QtMultimedia/QAudioDecoder>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(lcFileSource, "qtos.audio.file")

namespace qtos::audio {

namespace {

float sampleAt(const QAudioBuffer &buffer, qsizetype index)
{
    switch (buffer.format().sampleFormat()) {
    case QAudioFormat::UInt8: return (float(buffer.constData<quint8>()[index]) - 128.0f) / 128.0f;
    case QAudioFormat::Int16: return float(buffer.constData<qint16>()[index]) / 32768.0f;
    case QAudioFormat::Int32: return float(double(buffer.constData<qint32>()[index]) / 2147483648.0);
    case QAudioFormat::Float: return buffer.constData<float>()[index];
    default: return 0.0f;
    }
}

} // namespace

FileSource::FileSource(const QString &path, double sampleRate, QObject *parent)
    : QObject(parent), m_path(path), m_sampleRate(sampleRate), m_decoder(new QAudioDecoder(this)), m_chunks(kMaxChunks)
{
    QAudioFormat format;
    format.setSampleRate(int(sampleRate));
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);
    m_decoder->setAudioFormat(format);
    m_decoder->setSource(QUrl::fromLocalFile(path));

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &FileSource::readBuffer);
    connect(m_decoder, &QAudioDecoder::finished, this, &FileSource::finish);
    connect(m_decoder, &QAudioDecoder::durationChanged, this, [this](qint64 milliseconds) {
        if (milliseconds <= 0)
            return;
        m_expected.store(qint64(double(milliseconds) / 1000.0 * m_sampleRate), std::memory_order_relaxed);
        Q_EMIT durationChanged(double(milliseconds) / 1000.0);
    });
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this,
            [this](QAudioDecoder::Error) { fail(m_decoder->errorString()); });

    if (!m_decoder->isSupported())
        QMetaObject::invokeMethod(this, [this] { fail(tr("Audio decoding is not available")); }, Qt::QueuedConnection);
    else
        m_decoder->start();
}

FileSource::~FileSource() { m_decoder->stop(); }

void FileSource::readBuffer()
{
    while (m_decoder->bufferAvailable()) {
        const QAudioBuffer buffer = m_decoder->read();
        if (!buffer.isValid())
            continue;
        const QAudioFormat format = buffer.format();
        const int channels = format.channelCount();
        const qsizetype frames = buffer.frameCount();
        if (channels <= 0 || frames <= 0)
            continue;

        // Fast path: the decoder honoured the requested format.
        if (format.sampleFormat() == QAudioFormat::Int16 && channels == 2
            && std::abs(format.sampleRate() - m_sampleRate) < 0.5) {
            m_scratch.resize(std::size_t(frames) * 2);
            const qint16 *data = buffer.constData<qint16>();
            for (qsizetype i = 0; i < frames * 2; ++i)
                m_scratch[std::size_t(i)] = float(data[i]) / 32768.0f;
            append(m_scratch.data(), frames);
            continue;
        }

        // General path: any sample type and channel layout, resampled linearly.
        const double step = format.sampleRate() / m_sampleRate;
        m_scratch.clear();
        m_scratch.reserve(std::size_t(double(frames) / step + 2) * 2);
        for (qsizetype frame = 0; frame < frames; ++frame) {
            float current[2];
            current[0] = sampleAt(buffer, frame * channels);
            current[1] = channels > 1 ? sampleAt(buffer, frame * channels + 1) : current[0];
            if (!m_havePrevious) {
                m_previous[0] = current[0];
                m_previous[1] = current[1];
                m_havePrevious = true;
            }
            while (m_resamplePhase < 1.0) {
                const float t = float(m_resamplePhase);
                m_scratch.push_back(m_previous[0] + (current[0] - m_previous[0]) * t);
                m_scratch.push_back(m_previous[1] + (current[1] - m_previous[1]) * t);
                m_resamplePhase += step;
            }
            m_resamplePhase -= 1.0;
            m_previous[0] = current[0];
            m_previous[1] = current[1];
        }
        append(m_scratch.data(), qint64(m_scratch.size() / 2));
    }
}

void FileSource::append(const float *stereo, qint64 frames)
{
    qint64 decoded = m_decoded.load(std::memory_order_relaxed);
    const qint64 capacity = kChunkFrames * kMaxChunks;
    frames = std::min(frames, capacity - decoded);
    while (frames > 0) {
        const qint64 chunk = decoded / kChunkFrames;
        const qint64 offset = decoded % kChunkFrames;
        auto &storage = m_chunks[std::size_t(chunk)];
        if (!storage)
            storage = std::make_unique<qint16[]>(std::size_t(kChunkFrames * 2));
        const qint64 count = std::min(frames, kChunkFrames - offset);
        qint16 *target = storage.get() + offset * 2;
        for (qint64 i = 0; i < count * 2; ++i)
            target[i] = qint16(std::lround(std::clamp(stereo[i], -1.0f, 1.0f) * 32767.0f));
        stereo += count * 2;
        frames -= count;
        decoded += count;
        // Publishes the samples above to render().
        m_decoded.store(decoded, std::memory_order_release);
    }
    if (decoded >= capacity) {
        qCWarning(lcFileSource) << m_path << "is longer than the engine stores; playing the first"
                                << int(double(capacity) / m_sampleRate / 60.0) << "minutes";
        m_decoder->stop();
        finish();
    }
}

void FileSource::finish()
{
    if (m_complete.exchange(true, std::memory_order_acq_rel))
        return;
    const qint64 decoded = m_decoded.load(std::memory_order_relaxed);
    if (decoded == 0) {
        fail(tr("The file contains no audio"));
        return;
    }
    m_expected.store(decoded, std::memory_order_relaxed);
    Q_EMIT durationChanged(double(decoded) / m_sampleRate);
}

void FileSource::fail(const QString &message)
{
    if (m_failed.load(std::memory_order_relaxed))
        return;
    m_error = message;
    qCWarning(lcFileSource) << "Cannot play" << m_path << ":" << message;
    m_complete.store(true, std::memory_order_relaxed);
    m_failed.store(true, std::memory_order_release);
    Q_EMIT decodeFailed(message);
}

qint64 FileSource::render(float *out, qint64 frames)
{
    if (failed())
        return 0;
    const bool complete = m_complete.load(std::memory_order_acquire);
    const qint64 decoded = m_decoded.load(std::memory_order_acquire);
    qint64 position = m_position.load(std::memory_order_relaxed);
    const qint64 available = std::max<qint64>(0, decoded - position);
    const qint64 count = std::min(frames, available);

    qint64 written = 0;
    while (written < count) {
        const qint64 chunk = position / kChunkFrames;
        const qint64 offset = position % kChunkFrames;
        const qint64 run = std::min(count - written, kChunkFrames - offset);
        const qint16 *source = m_chunks[std::size_t(chunk)].get() + offset * 2;
        for (qint64 i = 0; i < run * 2; ++i)
            out[written * 2 + i] = float(source[i]) * (1.0f / 32768.0f);
        written += run;
        position += run;
    }
    m_position.store(position, std::memory_order_relaxed);

    if (complete) {
        m_buffering.store(false, std::memory_order_relaxed);
        return written; // fewer than asked: the end
    }
    // Still decoding: pad with silence and keep going.
    std::fill(out + written * 2, out + frames * 2, 0.0f);
    m_buffering.store(written < frames, std::memory_order_relaxed);
    return frames;
}

void FileSource::seek(double seconds)
{
    // Before the length is known a seek is kept as asked: render() buffers
    // until decoding reaches it.
    qint64 frame = std::max<qint64>(0, qint64(seconds * m_sampleRate));
    const qint64 limit =
        std::max(m_expected.load(std::memory_order_relaxed), m_decoded.load(std::memory_order_acquire));
    if (limit > 0)
        frame = std::min(frame, limit);
    m_position.store(frame, std::memory_order_relaxed);
}

double FileSource::position() const { return double(m_position.load(std::memory_order_relaxed)) / m_sampleRate; }

double FileSource::duration() const { return double(m_expected.load(std::memory_order_relaxed)) / m_sampleRate; }

} // namespace qtos::audio
