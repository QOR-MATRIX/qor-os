// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "analyzer.h"
#include "dsp.h"
#include "engine.h"
#include "fft.h"
#include "filesource.h"
#include "library.h"
#include "renderer.h"
#include "session.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>
#include <numbers>
#include <vector>

using namespace qtos::audio;
using namespace Qt::StringLiterals;

namespace {

constexpr double kRate = 48000;
constexpr double kTau = 2.0 * std::numbers::pi;

std::vector<float> sine(double hz, double seconds, double amplitude = 1.0, int channels = 1, double rate = kRate)
{
    std::vector<float> out(std::size_t(seconds * rate) * std::size_t(channels));
    for (std::size_t i = 0; i < out.size() / std::size_t(channels); ++i)
        for (int c = 0; c < channels; ++c)
            out[i * std::size_t(channels) + std::size_t(c)] = float(amplitude * std::sin(kTau * hz * double(i) / rate));
    return out;
}

double rms(const float *samples, std::size_t count, std::size_t stride = 1)
{
    double sum = 0;
    std::size_t n = 0;
    for (std::size_t i = 0; i < count; i += stride, ++n)
        sum += double(samples[i]) * samples[i];
    return std::sqrt(sum / double(std::max<std::size_t>(n, 1)));
}

// Gain in dB the equaliser applies to a stereo sine at `hz`.
double eqGainDb(Equalizer &eq, double hz)
{
    eq.reset();
    std::vector<float> signal = sine(hz, 1.0, 0.25, 2);
    const double before = rms(signal.data() + signal.size() / 2, signal.size() / 2, 2);
    eq.process(signal.data(), int(signal.size() / 2));
    const double after = rms(signal.data() + signal.size() / 2, signal.size() / 2, 2);
    return gainToDb(after / before);
}

// Feeds `mono` through the analyser at 60 Hz, as the engine does.
std::vector<AnalysisFrame> analyse(Analyzer &analyzer, const std::vector<float> &mono)
{
    std::vector<AnalysisFrame> frames;
    const std::size_t hop = std::size_t(kRate / 60);
    for (std::size_t end = Analyzer::kWindow; end <= mono.size(); end += hop)
        frames.push_back(analyzer.analyze(mono.data() + end - Analyzer::kWindow, 1.0 / 60));
    return frames;
}

std::vector<float> renderSession(const SessionSpec &spec, double from, double seconds)
{
    SessionSource session(spec, kRate);
    session.seek(from);
    std::vector<float> out(std::size_t(seconds * kRate) * 2);
    const qint64 rendered = session.render(out.data(), qint64(seconds * kRate));
    out.resize(std::size_t(rendered) * 2);
    return out;
}

std::vector<float> toMono(const std::vector<float> &stereo)
{
    std::vector<float> mono(stereo.size() / 2);
    for (std::size_t i = 0; i < mono.size(); ++i)
        mono[i] = 0.5f * (stereo[i * 2] + stereo[i * 2 + 1]);
    return mono;
}

// A 16-bit PCM stereo WAV file with `mono` in both channels.
bool writeWav(const QString &path, const std::vector<float> &mono, int rate)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const quint32 dataBytes = quint32(mono.size() * 4);
    auto u32 = [&file](quint32 v) { file.write(reinterpret_cast<const char *>(&v), 4); };
    auto u16 = [&file](quint16 v) { file.write(reinterpret_cast<const char *>(&v), 2); };
    file.write("RIFF");
    u32(36 + dataBytes);
    file.write("WAVEfmt ");
    u32(16);
    u16(1); // PCM
    u16(2); // stereo
    u32(quint32(rate));
    u32(quint32(rate * 4));
    u16(4);
    u16(16);
    file.write("data");
    u32(dataBytes);
    for (float sample : mono) {
        const auto value = quint16(qint16(std::lround(std::clamp(sample, -1.0f, 1.0f) * 32767.0f)));
        u16(value);
        u16(value);
    }
    return true;
}

} // namespace

class TestAudio : public QObject
{
    Q_OBJECT

private slots:
    // ── DSP ──────────────────────────────────────────────────────────────────
    void fftReadsFullScaleSineAsOne()
    {
        Fft fft(Analyzer::kWindow);
        // Exactly on bin 64.
        const double hz = 64.0 * kRate / Analyzer::kWindow;
        const std::vector<float> signal = sine(hz, 0.1);
        std::vector<float> magnitudes(Analyzer::kWindow / 2 + 1);
        fft.magnitudes(signal.data(), magnitudes.data());
        QCOMPARE_LT(std::abs(magnitudes[64] - 1.0f), 0.02f);
        QCOMPARE_LT(magnitudes[200], 0.001f);
    }

    void flatEqualizerIsTransparent()
    {
        Equalizer eq;
        eq.setSampleRate(kRate);
        QVERIFY(eq.isFlat());
        std::vector<float> signal = sine(440, 0.1, 0.5, 2);
        const std::vector<float> original = signal;
        eq.process(signal.data(), int(signal.size() / 2));
        QCOMPARE(signal, original);
    }

    void equalizerBandsHitTheirTargets()
    {
        Equalizer eq;
        eq.setSampleRate(kRate);
        eq.setGains({ 0, 0, 6, 0, 0 });
        QVERIFY(!eq.isFlat());
        QCOMPARE_LT(std::abs(eqGainDb(eq, 1000) - 6.0), 0.3);
        QCOMPARE_LT(std::abs(eqGainDb(eq, 60)), 0.5);
        QCOMPARE_LT(std::abs(eqGainDb(eq, 12000)), 0.5);

        eq.setGains({ 9, 0, 0, 0, -6 });
        QCOMPARE_LT(std::abs(eqGainDb(eq, 25) - 9.0), 0.8);    // low shelf
        QCOMPARE_LT(std::abs(eqGainDb(eq, 18000) + 6.0), 0.8); // high shelf
        QCOMPARE_LT(std::abs(eqGainDb(eq, 1000)), 1.0);

        eq.setGains({ 40, 0, 0, 0, 0 });
        QCOMPARE(eq.gains()[0], Equalizer::kMaxGainDb);
    }

    void limiterIsTransparentBelowTheKneeAndNeverClips()
    {
        QCOMPARE(softLimit(0.5f), 0.5f);
        QCOMPARE(softLimit(-0.85f), -0.85f);
        float previous = 0.85f;
        for (float x = 0.86f; x < 8.0f; x += 0.01f) {
            const float y = softLimit(x);
            // Strictly rising until tanh saturates in float precision.
            QVERIFY(x < 1.4f ? y > previous : y >= previous);
            QVERIFY(y <= 0.98f);
            QCOMPARE(softLimit(-x), -y);
            previous = y;
        }
    }

    void volumeTaperIsCubic()
    {
        QCOMPARE(volumeToGain(0), 0.0);
        QCOMPARE(volumeToGain(1), 1.0);
        QCOMPARE(volumeToGain(0.5), 0.125);
        QCOMPARE(volumeToGain(2), 1.0);
    }

    // ── Analysis ─────────────────────────────────────────────────────────────
    void silenceIsInactive()
    {
        Analyzer analyzer(kRate);
        const auto frames = analyse(analyzer, std::vector<float>(std::size_t(kRate), 0.0f));
        QVERIFY(!frames.back().active);
        QCOMPARE(frames.back().level, 0.0f);
        QCOMPARE(frames.back().reactive, 0.0f);
        QCOMPARE(frames.back().beat, 0);
    }

    void reactsOnlyToTheChosenRange()
    {
        const std::vector<float> bass = sine(80, 1.0, 0.5);

        Analyzer low(kRate);
        low.setRange(40, 160);
        const auto lowFrames = analyse(low, bass);
        QVERIFY(lowFrames.back().active);
        QCOMPARE_GT(lowFrames.back().reactive, 0.9f);

        Analyzer high(kRate);
        high.setRange(4000, 16000);
        const auto highFrames = analyse(high, bass);
        QVERIFY(highFrames.back().active);
        QCOMPARE_LT(highFrames.back().reactive, 0.05f);
    }

    void spectrumPeaksAtTheTone()
    {
        Analyzer analyzer(kRate);
        const auto frames = analyse(analyzer, sine(1000, 0.5, 0.5));
        const auto &bands = frames.back().bands;
        const auto peak = std::max_element(bands.cbegin(), bands.cend()) - bands.cbegin();
        const auto edges = Analyzer::bandEdges();
        QVERIFY2(
            edges[std::size_t(peak)] <= 1000 * 1.05 && edges[std::size_t(peak) + 1] >= 1000 / 1.05,
            qPrintable(
                u"peak band %1: %2-%3 Hz"_s.arg(peak).arg(edges[std::size_t(peak)]).arg(edges[std::size_t(peak) + 1])));
    }

    void detectsBeats()
    {
        // 60 Hz thumps, twice a second, over a quiet hiss.
        std::vector<float> signal(std::size_t(kRate * 6));
        quint32 noise = 1;
        for (std::size_t i = 0; i < signal.size(); ++i) {
            noise = noise * 1664525u + 1013904223u;
            const double t = double(i % std::size_t(kRate / 2)) / kRate;
            signal[i] = float(0.7 * std::sin(kTau * 60 * t) * std::exp(-t / 0.08)
                              + 0.01 * (double(noise >> 8) / double(1u << 24) - 0.5));
        }
        Analyzer analyzer(kRate);
        analyzer.setRange(40, 160);
        const auto frames = analyse(analyzer, signal);
        // The first second trains the threshold.
        const int beats = frames.back().beat;
        QVERIFY2(beats >= 9 && beats <= 12, qPrintable(u"%1 beats"_s.arg(beats)));
    }

    // ── QTOS Sessions ────────────────────────────────────────────────────────
    void sessionsAreDeterministicAndHaveHeadroom_data()
    {
        QTest::addColumn<QString>("style");
        QTest::addColumn<QString>("mode");
        QTest::addColumn<double>("bpm");
        QTest::newRow("drive") << u"drive"_s << u"minor"_s << 118.0;
        QTest::newRow("pulse") << u"pulse"_s << u"minor"_s << 124.0;
        QTest::newRow("chill") << u"chill"_s << u"dorian"_s << 92.0;
        QTest::newRow("ambient") << u"ambient"_s << u"lydian"_s << 78.0;
    }

    void sessionsAreDeterministicAndHaveHeadroom()
    {
        QFETCH(QString, style);
        QFETCH(QString, mode);
        QFETCH(double, bpm);
        SessionSpec spec;
        spec.style = style;
        spec.mode = mode;
        spec.bpm = bpm;
        spec.seed = 7;
        spec.duration = 200;

        QElapsedTimer clock;
        clock.start();
        const std::vector<float> main = renderSession(spec, 60, 8);
        const double realtime = 8.0 / std::max(0.001, double(clock.nsecsElapsed()) * 1e-9);
        qInfo().nospace() << style << ": " << qRound(realtime) << "x real time";
        QCOMPARE_GT(realtime, 4.0);

        float peak = 0;
        for (float sample : main) {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, std::abs(sample));
        }
        const double level = gainToDb(rms(main.data(), main.size()));
        qInfo().nospace() << style << ": peak " << gainToDb(peak) << " dBFS, RMS " << level << " dBFS";
        QCOMPARE_LT(peak, 1.0f);
        QCOMPARE_GT(level, -30.0);

        QCOMPARE(renderSession(spec, 60, 1),
                 std::vector<float>(main.begin(), main.begin() + std::ptrdiff_t(kRate * 2)));
        spec.seed = 8;
        QVERIFY(renderSession(spec, 60, 1)
                != std::vector<float>(main.begin(), main.begin() + std::ptrdiff_t(kRate * 2)));
    }

    void sessionBeatsFollowTheTempo_data()
    {
        QTest::addColumn<QString>("style");
        QTest::addColumn<double>("bpm");
        QTest::addColumn<double>("highHz");
        QTest::addColumn<int>("fewest");
        QTest::addColumn<int>("most");
        // Drive: a kick on every beat and a bass note on every offbeat, so
        // between 20 (kicks) and 40 (every onset) in 10 s at 120 BPM.
        QTest::newRow("drive, bass") << u"drive"_s << 120.0 << 160.0 << 16 << 44;
        QTest::newRow("drive, sub") << u"drive"_s << 120.0 << 90.0 << 16 << 44;
        // Chill: kicks on 1 and the "and" of 3 (sometimes 2), ~2.4 a bar.
        QTest::newRow("chill, bass") << u"chill"_s << 92.0 << 160.0 << 6 << 16;
    }

    void sessionBeatsFollowTheTempo()
    {
        QFETCH(QString, style);
        QFETCH(double, bpm);
        QFETCH(double, highHz);
        QFETCH(int, fewest);
        QFETCH(int, most);
        SessionSpec spec;
        spec.style = style;
        spec.bpm = bpm;
        spec.duration = 200;
        Analyzer analyzer(kRate);
        analyzer.setRange(40, highHz);
        const auto frames = analyse(analyzer, toMono(renderSession(spec, 60, 12)));
        // The first two seconds train the detector.
        const int beats = frames.back().beat - frames[120].beat;
        QVERIFY2(beats >= fewest && beats <= most, qPrintable(u"%1 beats in 10 s"_s.arg(beats)));
    }

    void sessionEndsAndSeeks()
    {
        SessionSpec spec;
        spec.duration = 2;
        SessionSource session(spec, kRate);
        QCOMPARE(session.duration(), 2.0);
        std::vector<float> buffer(std::size_t(kRate) * 2 * 3);
        QCOMPARE(session.render(buffer.data(), qint64(kRate * 3)), qint64(kRate * 2));
        QCOMPARE(session.position(), 2.0);
        session.seek(0.5);
        QCOMPARE(session.position(), 0.5);
    }

    void specDerivesMissingFieldsFromTheTitle()
    {
        const SessionSpec a = SessionSpec::fromVariant({}, 180, u"Glass Horizon"_s);
        const SessionSpec b = SessionSpec::fromVariant({}, 180, u"Glass Horizon"_s);
        QCOMPARE(a.style, b.style);
        QCOMPARE(a.seed, b.seed);
        QCOMPARE(a.duration, 180.0);
        const SessionSpec given = SessionSpec::fromVariant({ { u"style"_s, u"chill"_s }, { u"bpm"_s, 92 } }, 0, u"x"_s);
        QCOMPARE(given.style, u"chill"_s);
        QCOMPARE(given.bpm, 92.0);
        QCOMPARE_GT(given.duration, 0.0);
    }

    // ── Renderer ─────────────────────────────────────────────────────────────
    void rendererFadesPausesAndReportsTheEnd()
    {
        Renderer renderer(kRate);
        SessionSpec spec;
        spec.duration = 1.5;
        renderer.setSource(new SessionSource(spec, kRate), 1);
        renderer.setVolume(1.0);
        std::vector<float> buffer(4800 * 2);

        renderer.process(buffer.data(), 4800); // stopped: silence
        QCOMPARE(rms(buffer.data(), buffer.size()), 0.0);

        renderer.setPlaying(true);
        for (int i = 0; i < 5; ++i)
            renderer.process(buffer.data(), 4800);
        QCOMPARE_GT(rms(buffer.data(), buffer.size()), 0.001);
        QVERIFY(renderer.position() > 0.4);

        renderer.setPlaying(false);
        renderer.process(buffer.data(), 4800); // 10 ms fade, then silence
        renderer.process(buffer.data(), 4800);
        QCOMPARE(rms(buffer.data(), buffer.size()), 0.0);
        const double paused = renderer.position();
        renderer.process(buffer.data(), 4800);
        QCOMPARE(renderer.position(), paused);

        renderer.setPlaying(true);
        renderer.setMuted(true);
        for (int i = 0; i < 3; ++i)
            renderer.process(buffer.data(), 4800);
        QCOMPARE(rms(buffer.data() + 4800, 4800), 0.0);
        renderer.setMuted(false);

        QCOMPARE(renderer.finishedGeneration(), quint64(0));
        for (int i = 0; i < 20; ++i)
            renderer.process(buffer.data(), 4800);
        QCOMPARE(renderer.finishedGeneration(), quint64(1));

        // A replaced source comes back for deletion off the audio thread.
        renderer.setSource(new SessionSource(spec, kRate), 2);
        renderer.process(buffer.data(), 256);
        Source *retired = renderer.takeRetired();
        QVERIFY(retired);
        delete retired;

        float monitor[1024];
        QVERIFY(renderer.copyMonitor(monitor, 1024, 0));
    }

    // ── Files ────────────────────────────────────────────────────────────────
    void decodesFilesWhilePlaying()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"tone.wav"_s);
        QVERIFY(writeWav(path, sine(440, 2.0, 0.5, 1, 44100), 44100)); // resampled to 48 kHz

        FileSource file(path, kRate);
        QSignalSpy duration(&file, &FileSource::durationChanged);
        QVERIFY(QTest::qWaitFor([&] { return std::abs(file.duration() - 2.0) < 0.05 || file.failed(); }, 10000));
        QVERIFY2(!file.failed(), qPrintable(file.errorString()));

        std::vector<float> buffer(std::size_t(kRate) * 2);
        QVERIFY(QTest::qWaitFor(
            [&] {
                file.seek(0);
                return file.render(buffer.data(), qint64(kRate)) == qint64(kRate) && !file.buffering();
            },
            10000));
        // 0.5 amplitude sine: RMS 0.354.
        QCOMPARE_LT(std::abs(rms(buffer.data(), buffer.size()) - 0.3536), 0.01);
        QCOMPARE(file.position(), 1.0);

        // Once decoding completes, the end is the end.
        qint64 tail = 0;
        QVERIFY(QTest::qWaitFor(
            [&] {
                file.seek(1.9);
                tail = file.render(buffer.data(), qint64(kRate));
                return tail < qint64(kRate);
            },
            10000));
        QVERIFY2(tail > 4000 && tail < 5600, qPrintable(QString::number(tail)));
    }

    void libraryReadsNamesFromTheLayout()
    {
        QTemporaryDir dir;
        QDir root(dir.path());
        QVERIFY(root.mkpath(u"Nova Arc/Night Shapes"_s));
        auto touch = [&](const QString &relative) {
            QFile file(root.filePath(relative));
            return file.open(QIODevice::WriteOnly);
        };
        QVERIFY(touch(u"Nova Arc/Night Shapes/03 Lanterns.flac"_s));
        QVERIFY(touch(u"Mira Vale - Tidelines.mp3"_s));
        QVERIFY(touch(u"notes.txt"_s));

        const QVariantList tracks = scanLibrary({ dir.path() });
        QCOMPARE(tracks.size(), 2);
        const QVariantMap first = tracks.at(0).toMap();
        QCOMPARE(first.value(u"title"_s).toString(), u"Tidelines"_s);
        QCOMPARE(first.value(u"artist"_s).toString(), u"Mira Vale"_s);
        QCOMPARE(first.value(u"kind"_s).toString(), u"file"_s);
        const QVariantMap second = tracks.at(1).toMap();
        QCOMPARE(second.value(u"title"_s).toString(), u"Lanterns"_s);
        QCOMPARE(second.value(u"artist"_s).toString(), u"Nova Arc"_s);
        QCOMPARE(second.value(u"album"_s).toString(), u"Night Shapes"_s);
    }

    // ── Engine ───────────────────────────────────────────────────────────────
    void enginePlaysInRealTimeWithoutADevice()
    {
        Engine engine(Engine::Output::Null);
        QVERIFY(engine.isNullOutput());
        QSignalSpy position(&engine, &Engine::positionChanged);
        QSignalSpy analysis(&engine, &Engine::analysisReady);
        QSignalSpy finished(&engine, &Engine::finished);
        QSignalSpy duration(&engine, &Engine::durationChanged);

        engine.setVolume(0.8);
        engine.load({ { u"kind"_s, u"session"_s },
                      { u"title"_s, u"Test"_s },
                      { u"duration"_s, 1.6 },
                      { u"session"_s, QVariantMap { { u"style"_s, u"drive"_s } } } });
        engine.setPlaying(true);

        QVERIFY(duration.wait(2000));
        QCOMPARE(duration.last().at(0).toDouble(), 1.6);
        QVERIFY(QTest::qWaitFor([&] { return !position.isEmpty() && position.last().at(0).toDouble() > 0.5; }, 5000));
        QVERIFY(QTest::qWaitFor(
            [&] {
                for (const auto &arguments : analysis)
                    if (arguments.at(0).value<AnalysisFrame>().active)
                        return true;
                return false;
            },
            5000));
        QVERIFY(finished.wait(5000));
    }
};

QTEST_GUILESS_MAIN(TestAudio)
#include "tst_audio.moc"
