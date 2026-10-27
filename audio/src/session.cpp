// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "session.h"

#include <QtCore/QHash>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace qor::audio {

namespace {

constexpr double kTau = 2.0 * std::numbers::pi;
// Leaves about a decibel of headroom on the loudest style before the engine's
// equaliser and limiter.
constexpr double kMaster = 0.74;

double midiToHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

// xorshift32: small, fast and deterministic across platforms.
struct Rng
{
    quint32 state;
    explicit Rng(quint32 seed) : state(seed ? seed : 0x9e3779b9u) { }
    quint32 next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    float uniform() { return float(next() >> 8) / float(1u << 24); } // [0, 1)
    float bipolar() { return uniform() * 2.0f - 1.0f; }
};

// Band-limited sawtooth (PolyBLEP).
struct Saw
{
    double phase = 0;
    float next(double increment)
    {
        double t = phase;
        double value = 2.0 * t - 1.0;
        if (t < increment) {
            t /= increment;
            value -= t + t - t * t - 1.0;
        } else if (t > 1.0 - increment) {
            t = (t - 1.0) / increment;
            value -= t * t + t + t + 1.0;
        }
        phase += increment;
        if (phase >= 1.0)
            phase -= 1.0;
        return float(value);
    }
};

// Topology-preserving state-variable filter (Zavalishin); coefficients are set
// at control rate.
struct Svf
{
    double ic1 = 0, ic2 = 0;
    double a1 = 1, a2 = 0, a3 = 0, k = 1.4;

    void set(double cutoff, double sampleRate, double q)
    {
        const double g = std::tan(std::numbers::pi * std::clamp(cutoff, 20.0, sampleRate * 0.45) / sampleRate);
        k = 1.0 / q;
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    struct Out
    {
        float low, band, high;
    };
    Out process(float input)
    {
        const double v3 = input - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        return { float(v2), float(v1), float(input - k * v1 - v2) };
    }
};

// Freeverb-style reverb: parallel damped combs into series allpasses.
class Reverb
{
public:
    explicit Reverb(double sampleRate)
    {
        const double scale = sampleRate / 44100.0;
        const int combs[] = { 1116, 1188, 1277, 1356, 1422, 1491 };
        const int allpasses[] = { 556, 441 };
        for (int c = 0; c < 2; ++c) {
            for (int length : combs)
                m_combs[c].push_back({ std::vector<float>(std::size_t((length + c * 23) * scale), 0.0f) });
            for (int length : allpasses)
                m_allpasses[c].push_back({ std::vector<float>(std::size_t((length + c * 23) * scale), 0.0f) });
        }
    }
    void process(float input, float &left, float &right)
    {
        float *outs[2] = { &left, &right };
        for (int c = 0; c < 2; ++c) {
            float sum = 0;
            for (Comb &comb : m_combs[c]) {
                const float out = comb.buffer[comb.index];
                comb.filter = out * (1.0f - kDamp) + comb.filter * kDamp;
                comb.buffer[comb.index] = input * 0.015f + comb.filter * kRoom;
                comb.index = (comb.index + 1) % comb.buffer.size();
                sum += out;
            }
            for (Allpass &allpass : m_allpasses[c]) {
                const float buffered = allpass.buffer[allpass.index];
                const float out = buffered - sum;
                allpass.buffer[allpass.index] = sum + buffered * 0.5f;
                allpass.index = (allpass.index + 1) % allpass.buffer.size();
                sum = out;
            }
            *outs[c] = sum;
        }
    }
    void clear()
    {
        for (auto &channel : m_combs)
            for (Comb &comb : channel) {
                std::fill(comb.buffer.begin(), comb.buffer.end(), 0.0f);
                comb.filter = 0;
            }
        for (auto &channel : m_allpasses)
            for (Allpass &allpass : channel)
                std::fill(allpass.buffer.begin(), allpass.buffer.end(), 0.0f);
    }

private:
    static constexpr float kRoom = 0.86f;
    static constexpr float kDamp = 0.32f;
    struct Comb
    {
        std::vector<float> buffer;
        std::size_t index = 0;
        float filter = 0;
    };
    struct Allpass
    {
        std::vector<float> buffer;
        std::size_t index = 0;
    };
    std::array<std::vector<Comb>, 2> m_combs;
    std::array<std::vector<Allpass>, 2> m_allpasses;
};

// Ping-pong delay with a darkening feedback path.
class PingPong
{
public:
    explicit PingPong(double sampleRate) : m_left(std::size_t(sampleRate * 2), 0.0f), m_right(m_left.size(), 0.0f) { }
    void setDelay(std::size_t samples) { m_delay = std::clamp<std::size_t>(samples, 1, m_left.size() - 1); }
    void process(float input, float &left, float &right)
    {
        const std::size_t read = (m_index + m_left.size() - m_delay) % m_left.size();
        const float l = m_left[read];
        const float r = m_right[read];
        m_toneL += 0.35f * (r - m_toneL);
        m_toneR += 0.35f * (l - m_toneR);
        m_left[m_index] = input + m_toneL * kFeedback;
        m_right[m_index] = m_toneR * kFeedback;
        m_index = (m_index + 1) % m_left.size();
        left = l;
        right = r;
    }
    void clear()
    {
        std::fill(m_left.begin(), m_left.end(), 0.0f);
        std::fill(m_right.begin(), m_right.end(), 0.0f);
        m_toneL = m_toneR = 0;
    }

private:
    static constexpr float kFeedback = 0.42f;
    std::vector<float> m_left, m_right;
    std::size_t m_index = 0, m_delay = 1;
    float m_toneL = 0, m_toneR = 0;
};

enum class Style { Drive, Pulse, Chill, Ambient };

// How present each layer is in a section of the arrangement.
struct Mix
{
    float pad, arp, bass, kick, clap, hats, open, brightness;
};

Mix mixFor(double progress)
{
    if (progress < 0.12)
        return { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.35f }; // intro
    if (progress < 0.24)
        return { 1.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.55f }; // build
    if (progress < 0.50)
        return { 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.85f }; // main
    if (progress < 0.62)
        return { 1.0f, 0.7f, 0.5f, 0.0f, 0.0f, 0.3f, 0.0f, 0.5f }; // break
    if (progress < 0.90)
        return { 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }; // main 2
    return { 1.0f, 0.6f, 0.4f, 0.5f, 0.0f, 0.4f, 0.0f, 0.45f };    // outro
}

const QHash<QString, std::array<int, 7>> &modes()
{
    static const QHash<QString, std::array<int, 7>> table {
        { QStringLiteral("minor"), { 0, 2, 3, 5, 7, 8, 10 } },
        { QStringLiteral("dorian"), { 0, 2, 3, 5, 7, 9, 10 } },
        { QStringLiteral("major"), { 0, 2, 4, 5, 7, 9, 11 } },
        { QStringLiteral("lydian"), { 0, 2, 4, 6, 7, 9, 11 } },
    };
    return table;
}

std::vector<std::vector<int>> progressionsFor(const QString &mode)
{
    if (mode == QLatin1String("dorian"))
        return { { 0, 3, 0, 6 }, { 0, 1, 3, 0 }, { 0, 6, 3, 4 } };
    if (mode == QLatin1String("major"))
        return { { 0, 4, 5, 3 }, { 0, 5, 3, 4 }, { 3, 4, 0, 5 } };
    if (mode == QLatin1String("lydian"))
        return { { 0, 1, 4, 0 }, { 0, 1, 5, 1 }, { 0, 5, 1, 4 } };
    return { { 0, 5, 2, 6 }, { 0, 3, 5, 4 }, { 0, 6, 5, 6 } };
}

} // namespace

SessionSpec SessionSpec::fromVariant(const QVariantMap &session, double duration, const QString &title)
{
    SessionSpec spec;
    const auto hash = quint32(qHash(title, 0x51u));
    static const char *styles[] = { "drive", "pulse", "chill", "ambient" };
    static const char *modeNames[] = { "minor", "dorian", "major", "lydian" };
    spec.style = session.value(QStringLiteral("style"), QString::fromLatin1(styles[hash % 4])).toString();
    spec.mode = session.value(QStringLiteral("mode"), QString::fromLatin1(modeNames[(hash >> 3) % 4])).toString();
    spec.bpm = session.value(QStringLiteral("bpm"), 84 + (hash >> 6) % 40).toDouble();
    spec.root = session.value(QStringLiteral("root"), 52 + int((hash >> 11) % 9)).toInt();
    spec.seed = session.value(QStringLiteral("seed"), hash).toUInt();
    spec.duration = duration > 0 ? duration : 200;
    return spec;
}

struct SessionSource::Impl
{
    SessionSpec spec;
    double rate;
    Style style;
    std::array<int, 7> scale;
    std::vector<int> progression;
    qint64 total;
    qint64 position = 0;
    double samplesPerStep;
    qint64 step = -1;
    double nextStepAt = 0;
    Rng rng;

    // Arrangement and harmony.
    Mix mix {};
    Mix target {};
    std::array<int, 4> chord {};
    std::array<double, 4> padNote {};
    int bassRoot = 0;

    // Pad: four voice-led slots, two detuned saws each, one shared filter.
    std::array<std::array<Saw, 2>, 4> padOsc {};
    std::array<double, 4> padHz {};
    Svf padFilterL, padFilterR;

    // Bass: mono sine + saw through a low-pass.
    double bassPhase = 0;
    Saw bassSaw;
    double bassHz = 55;
    double bassEnv = 0;
    double bassGate = 0;
    Svf bassFilter;

    // Plucks: round-robin voices.
    struct Pluck
    {
        Saw osc;
        Svf filter;
        double hz = 440, age = 1e9, velocity = 0, pan = 0;
    };
    std::array<Pluck, 6> plucks {};
    int nextPluck = 0;
    int arpIndex = 0;

    // Drums.
    double kickAge = 1e9, kickPhase = 0, kickVelocity = 1;
    double clapAge = 1e9, clapPhase = 0;
    double hatAge = 1e9, hatVelocity = 0, openAge = 1e9;
    Svf clapFilter, hatFilter, openFilter;

    Reverb reverb;
    PingPong delay;
    int controlCounter = 0;

    Impl(const SessionSpec &s, double sampleRate)
        : spec(s), rate(sampleRate), total(qint64(s.duration * sampleRate)),
          samplesPerStep(sampleRate * 60.0 / s.bpm / 4.0), rng(s.seed), reverb(sampleRate), delay(sampleRate)
    {
        style = s.style == QLatin1String("pulse")     ? Style::Pulse
                : s.style == QLatin1String("chill")   ? Style::Chill
                : s.style == QLatin1String("ambient") ? Style::Ambient
                                                      : Style::Drive;
        scale = modes().value(s.mode, modes().value(QStringLiteral("minor")));
        const auto progressions = progressionsFor(s.mode);
        progression = progressions[s.seed % progressions.size()];
        delay.setDelay(std::size_t(samplesPerStep * 3)); // dotted eighth
        for (std::size_t i = 0; i < padNote.size(); ++i)
            padNote[i] = s.root + 7.0 * double(i);
        reset();
    }

    int degreeToSemitone(int degree) const
    {
        const int octave = degree >= 0 ? degree / 7 : (degree - 6) / 7;
        return octave * 12 + scale[std::size_t(degree - octave * 7)];
    }

    void reset()
    {
        padFilterL = padFilterR = Svf {};
        bassFilter = clapFilter = hatFilter = openFilter = Svf {};
        bassFilter.set(380, rate, 0.8);
        clapFilter.set(1700, rate, 0.9);
        hatFilter.set(7500, rate, 0.7);
        openFilter.set(6500, rate, 0.7);
        for (Pluck &p : plucks)
            p = Pluck {};
        kickAge = clapAge = hatAge = openAge = 1e9;
        bassEnv = bassGate = 0;
        reverb.clear();
        delay.clear();
        step = qint64(std::floor(position / samplesPerStep)) - 1;
        nextStepAt = double(step + 1) * samplesPerStep;
        const double progress = double(position) / double(std::max<qint64>(total, 1));
        mix = target = mixFor(progress);
        applyChord(std::max<qint64>(0, step + 1) / 32, true);
    }

    // A new chord every two bars, voice-led from the previous one.
    void applyChord(qint64 chordIndex, bool snap)
    {
        const int degree = progression[std::size_t(chordIndex % qint64(progression.size()))];
        for (int i = 0; i < 4; ++i)
            chord[std::size_t(i)] = degreeToSemitone(degree + i * 2);
        for (std::size_t i = 0; i < 4; ++i) {
            double note = spec.root + chord[i];
            while (note < padNote[i] - 6)
                note += 12;
            while (note > padNote[i] + 6)
                note -= 12;
            padNote[i] = std::clamp(note, double(spec.root - 5), double(spec.root + 19));
            if (snap)
                padHz[i] = midiToHz(padNote[i]);
        }
        bassRoot = spec.root - 24 + chord[0];
        while (bassRoot < 31)
            bassRoot += 12;
    }

    void triggerPluck(double note, float velocity)
    {
        Pluck &p = plucks[std::size_t(nextPluck)];
        nextPluck = (nextPluck + 1) % int(plucks.size());
        p.hz = midiToHz(note);
        p.age = 0;
        p.velocity = velocity;
        p.pan = (nextPluck % 2 ? 0.28 : -0.28);
    }

    void onStep()
    {
        ++step;
        const int inBar = int(step % 16);
        const qint64 bar = step / 16;
        const double progress = double(position) / double(std::max<qint64>(total, 1));
        target = mixFor(progress);
        if (step % 32 == 0)
            applyChord(bar / 2, false);

        // Drums.
        bool kick = false, clap = false, hat = false, open = false;
        switch (style) {
        case Style::Drive:
            kick = inBar % 4 == 0;
            clap = inBar == 4 || inBar == 12;
            hat = inBar % 2 == 0 || (progress > 0.62 && rng.uniform() < 0.35f);
            open = inBar % 4 == 2;
            break;
        case Style::Pulse:
            kick = inBar % 4 == 0;
            clap = inBar == 4 || inBar == 12;
            hat = true;
            open = inBar % 4 == 2;
            break;
        case Style::Chill:
            kick = inBar == 0 || inBar == 10 || (inBar == 7 && rng.uniform() < 0.4f);
            clap = inBar == 8;
            hat = inBar % 2 == 0;
            open = inBar == 14 && rng.uniform() < 0.5f;
            break;
        case Style::Ambient:
            kick = inBar == 0;
            hat = inBar % 3 == 0 && rng.uniform() < 0.6f;
            break;
        }
        if (kick && mix.kick > 0.05f) {
            kickAge = 0;
            kickPhase = 0;
            kickVelocity = style == Style::Ambient ? 0.55 : 1.0;
        }
        if (clap && mix.clap > 0.05f)
            clapAge = 0;
        if (hat && mix.hats > 0.05f) {
            hatAge = 0;
            hatVelocity = (inBar % 4 == 0 ? 1.0 : 0.6) * (0.85 + 0.15 * rng.uniform());
            openAge = 1e9; // a closed hat chokes the open one
        }
        if (open && mix.open > 0.05f)
            openAge = 0;

        // Bass.
        double bassLength = 0;
        int bassNote = bassRoot;
        switch (style) {
        case Style::Drive:
            if (inBar % 4 == 2 || (progress > 0.62 && inBar == 0))
                bassLength = 1.5;
            break;
        case Style::Pulse:
            if (inBar % 4 != 0)
                bassLength = 0.8;
            break;
        case Style::Chill:
            if (inBar == 0)
                bassLength = 6;
            else if (inBar == 10) {
                bassLength = 4;
                bassNote = bassRoot + 7;
            }
            break;
        case Style::Ambient:
            if (inBar == 0)
                bassLength = 15;
            break;
        }
        if (bassLength > 0 && mix.bass > 0.05f) {
            bassHz = midiToHz(bassNote);
            bassGate = bassLength * samplesPerStep;
            bassEnv = std::max(bassEnv, 0.0);
        }

        // Arpeggio over the chord tones and the octave.
        bool play = false;
        switch (style) {
        case Style::Drive: play = rng.uniform() < 0.9f; break;
        case Style::Pulse: play = (0b1011011010110110 >> inBar) & 1; break;
        case Style::Chill: play = inBar % 2 == 0 && rng.uniform() < 0.8f; break;
        case Style::Ambient: play = inBar % 3 == 0 && rng.uniform() < 0.7f; break;
        }
        if (play && mix.arp > 0.05f) {
            static constexpr int pattern[] = { 0, 1, 2, 3, 4, 3, 2, 1 };
            const int tone = pattern[arpIndex % 8];
            arpIndex += (rng.uniform() < 0.15f) ? 2 : 1;
            const double note = spec.root + 12 + (tone == 4 ? chord[0] + 12 : chord[std::size_t(tone)]);
            triggerPluck(note, inBar % 4 == 0 ? 1.0f : 0.72f);
        }
    }

    void control()
    {
        // Mix follows the arrangement over about a second.
        const float k = float(1.0 - std::exp(-32.0 / rate / 0.8));
        mix.pad += (target.pad - mix.pad) * k;
        mix.arp += (target.arp - mix.arp) * k;
        mix.bass += (target.bass - mix.bass) * k;
        mix.kick += (target.kick - mix.kick) * k;
        mix.clap += (target.clap - mix.clap) * k;
        mix.hats += (target.hats - mix.hats) * k;
        mix.open += (target.open - mix.open) * k;
        mix.brightness += (target.brightness - mix.brightness) * k;

        const double t = double(position) / rate;
        const double lfo = std::sin(kTau * 0.045 * t);
        const double cutoff = (500.0 + 2600.0 * mix.brightness) * std::pow(2.0, 0.6 * lfo);
        padFilterL.set(cutoff, rate, 0.9);
        padFilterR.set(cutoff * 1.04, rate, 0.9);
        for (std::size_t i = 0; i < padHz.size(); ++i) {
            const double targetHz = midiToHz(padNote[i]);
            padHz[i] += (targetHz - padHz[i]) * 0.02; // portamento
        }
        for (Pluck &p : plucks)
            if (p.age < 0.6 * rate)
                p.filter.set(260.0 + 3800.0 * std::exp(-p.age / (0.11 * rate)) * mix.brightness + 300.0, rate, 1.1);
    }

    void renderFrame(float &left, float &right)
    {
        if (position >= qint64(nextStepAt)) {
            onStep();
            nextStepAt = double(step + 1) * samplesPerStep;
        }
        if (controlCounter-- <= 0) {
            control();
            controlCounter = 32;
        }

        const double dt = 1.0 / rate;
        float l = 0, r = 0, send = 0, delaySend = 0;

        // Kick (also ducks the pad and bass: the "pump").
        float kickOut = 0, duck = 1;
        if (kickAge < 0.5 * rate) {
            const double t = kickAge * dt;
            const double hz = 46.0 + 84.0 * std::exp(-t / 0.032);
            kickPhase += hz * dt;
            const double env = std::exp(-t / 0.27);
            kickOut = float(std::sin(kTau * kickPhase) * env * kickVelocity * 0.78 * mix.kick);
            if (kickAge < 0.002 * rate)
                kickOut += rng.bipolar() * 0.12f * mix.kick;
            duck = float(1.0 - 0.45 * std::exp(-t / 0.16) * mix.kick);
            kickAge += 1;
        }
        l += kickOut;
        r += kickOut;

        // Pad.
        float padL = 0, padR = 0;
        for (std::size_t i = 0; i < padOsc.size(); ++i) {
            const double hz = padHz[i];
            padL += padOsc[i][0].next(hz * 0.996 * dt);
            padR += padOsc[i][1].next(hz * 1.004 * dt);
        }
        const float padGain = 0.052f * mix.pad * duck;
        padL = padFilterL.process(padL * padGain).low;
        padR = padFilterR.process(padR * padGain).low;
        l += padL;
        r += padR;
        send += (padL + padR) * 0.55f;

        // Bass.
        if (bassGate > 0) {
            bassEnv += (1.0 - bassEnv) * 0.012;
            bassGate -= 1;
        } else {
            bassEnv *= 0.9985;
        }
        bassPhase += bassHz * dt;
        if (bassPhase >= 1.0)
            bassPhase -= 1.0;
        const float bassRaw = float(std::sin(kTau * bassPhase) * 0.8) + bassSaw.next(bassHz * dt) * 0.28f;
        const float bassOut = bassFilter.process(bassRaw).low * float(bassEnv) * 0.34f * mix.bass * duck;
        l += bassOut;
        r += bassOut;

        // Plucks.
        float pluckSum = 0;
        for (Pluck &p : plucks) {
            if (p.age > 0.7 * rate)
                continue;
            const double t = p.age * dt;
            const double env = std::min(1.0, t / 0.003) * std::exp(-t / 0.22) * p.velocity;
            const float voice = p.filter.process(p.osc.next(p.hz * dt)).low * float(env) * 0.15f * mix.arp;
            l += voice * float(1.0 - p.pan);
            r += voice * float(1.0 + p.pan);
            pluckSum += voice;
            p.age += 1;
        }
        delaySend += pluckSum;
        send += pluckSum * 0.35f;

        // Clap: three quick bursts of filtered noise and a low tone.
        if (clapAge < 0.35 * rate) {
            const double t = clapAge * dt;
            const double bursts = t < 0.024 ? std::exp(-std::fmod(t, 0.009) / 0.003) : std::exp(-(t - 0.024) / 0.12);
            const float noise = clapFilter.process(rng.bipolar()).band;
            clapPhase += 185.0 * dt;
            const float tone = float(std::sin(kTau * clapPhase) * std::exp(-t / 0.05) * 0.25);
            const float clapOut = (noise * float(bursts) * 1.4f + tone) * 0.24f * mix.clap;
            l += clapOut;
            r += clapOut;
            send += clapOut * 0.5f;
            clapAge += 1;
        }

        // Hats.
        if (hatAge < 0.12 * rate) {
            const double t = hatAge * dt;
            const float hat =
                hatFilter.process(rng.bipolar()).high * float(std::exp(-t / 0.022) * hatVelocity) * 0.075f * mix.hats;
            l += hat * 0.85f;
            r += hat * 1.15f;
            hatAge += 1;
        }
        if (openAge < 0.5 * rate) {
            const double t = openAge * dt;
            const float open = openFilter.process(rng.bipolar()).high * float(std::exp(-t / 0.17)) * 0.06f * mix.open;
            l += open * 1.1f;
            r += open * 0.9f;
            openAge += 1;
        }

        float delayL = 0, delayR = 0;
        delay.process(delaySend, delayL, delayR);
        l += delayL * 0.32f;
        r += delayR * 0.32f;
        float verbL = 0, verbR = 0;
        reverb.process(send, verbL, verbR);
        l += verbL * 0.55f;
        r += verbR * 0.55f;

        // Fade in over the first second and out over the last four.
        const double seconds = double(position) * dt;
        const double remaining = double(total - position) * dt;
        const double fade = std::min({ 1.0, seconds / 1.0, remaining / 4.0 });
        left = float(l * fade * kMaster);
        right = float(r * fade * kMaster);
    }
};

SessionSource::SessionSource(const SessionSpec &spec, double sampleRate) : d(std::make_unique<Impl>(spec, sampleRate))
{ }

SessionSource::~SessionSource() = default;

qint64 SessionSource::render(float *out, qint64 frames)
{
    qint64 rendered = 0;
    while (rendered < frames && d->position < d->total) {
        d->renderFrame(out[rendered * 2], out[rendered * 2 + 1]);
        ++d->position;
        ++rendered;
    }
    return rendered;
}

void SessionSource::seek(double seconds)
{
    d->position = std::clamp(qint64(seconds * d->rate), qint64(0), d->total);
    d->reset();
}

double SessionSource::position() const { return double(d->position) / d->rate; }

double SessionSource::duration() const { return d->spec.duration; }

} // namespace qor::audio
