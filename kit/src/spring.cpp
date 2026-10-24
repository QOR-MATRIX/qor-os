// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "spring.h"

#include "kit.h"

#include <QtCore/QAbstractAnimation>

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Qt::StringLiterals;

namespace qtos {

// Bridges Qt's animation timer (which Qt Quick drives from vsync) to Spring.
class SpringDriver final : public QAbstractAnimation
{
public:
    explicit SpringDriver(Spring *spring) : m_spring(spring) { }
    int duration() const override { return -1; }

protected:
    void updateCurrentTime(int currentTime) override { m_spring->advanceTo(currentTime); }

private:
    Spring *m_spring;
};

namespace {

constexpr qreal kMaxStepSeconds = 0.1; // ignore pathological frame gaps (debugger, suspend)
constexpr qreal kMinResponse = 0.01;
constexpr qreal kCriticalEpsilon = 1e-4;

struct State
{
    qreal x; // displacement from target
    qreal v; // velocity
};

// Closed-form damped harmonic oscillator, unit mass.
State evolve(State s, qreal omega, qreal zeta, qreal t)
{
    if (std::abs(zeta - 1.0) < kCriticalEpsilon) {
        const qreal b = s.v + omega * s.x;
        const qreal e = std::exp(-omega * t);
        return { (s.x + b * t) * e, (b - omega * (s.x + b * t)) * e };
    }
    if (zeta < 1.0) {
        const qreal wd = omega * std::sqrt(1.0 - zeta * zeta);
        const qreal decay = std::exp(-zeta * omega * t);
        const qreal c = std::cos(wd * t);
        const qreal sn = std::sin(wd * t);
        const qreal b = (s.v + zeta * omega * s.x) / wd;
        const qreal x = decay * (s.x * c + b * sn);
        const qreal v = decay * ((b * wd - zeta * omega * s.x) * c - (s.x * wd + zeta * omega * b) * sn);
        return { x, v };
    }
    const qreal root = std::sqrt(zeta * zeta - 1.0);
    const qreal r1 = -omega * (zeta - root);
    const qreal r2 = -omega * (zeta + root);
    const qreal c2 = (s.v - r1 * s.x) / (r2 - r1);
    const qreal c1 = s.x - c2;
    const qreal e1 = std::exp(r1 * t);
    const qreal e2 = std::exp(r2 * t);
    return { c1 * e1 + c2 * e2, c1 * r1 * e1 + c2 * r2 * e2 };
}

} // namespace

Spring::Spring(QObject *parent) : QObject(parent), m_driver(std::make_unique<SpringDriver>(this))
{
    connect(Kit::instance(), &Kit::reducedMotionChanged, this, [this] {
        if (Kit::instance()->reducedMotion() && isRunning())
            complete();
    });
}

Spring::~Spring() = default;

bool Spring::isRunning() const { return m_driver->state() == QAbstractAnimation::Running; }

bool Spring::shouldAnimate() const { return m_enabled && !m_initializing && !Kit::instance()->reducedMotion(); }

void Spring::setTarget(qreal target)
{
    if (m_target == target)
        return;
    m_target = target;
    Q_EMIT targetChanged();

    if (m_initializing)
        return;
    if (!shouldAnimate()) {
        complete();
        return;
    }
    start();
}

void Spring::setValue(qreal value)
{
    if (m_initializing)
        m_valueSetDuringInit = true;
    if (m_value == value)
        return;
    m_value = value;
    Q_EMIT valueChanged();

    if (m_initializing)
        return;
    if (!shouldAnimate()) {
        complete();
        return;
    }
    if (m_value != m_target)
        start();
}

void Spring::setVelocity(qreal velocity)
{
    if (m_velocity == velocity)
        return;
    m_velocity = velocity;
    Q_EMIT velocityChanged();
    if (!m_initializing && shouldAnimate() && velocity != 0)
        start();
}

void Spring::setResponse(qreal response)
{
    response = std::max(response, kMinResponse);
    if (m_response == response)
        return;
    m_response = response;
    Q_EMIT responseChanged();
}

void Spring::setDamping(qreal damping)
{
    damping = std::max(damping, 0.0);
    if (m_damping == damping)
        return;
    m_damping = damping;
    Q_EMIT dampingChanged();
}

void Spring::setPreset(const QVariantMap &preset)
{
    if (m_preset == preset)
        return;
    m_preset = preset;
    if (const auto it = preset.constFind(u"response"_s); it != preset.cend())
        setResponse(it->toReal());
    if (const auto it = preset.constFind(u"damping"_s); it != preset.cend())
        setDamping(it->toReal());
    Q_EMIT presetChanged();
}

void Spring::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    Q_EMIT enabledChanged();
    if (!enabled)
        complete();
}

void Spring::snap(qreal value)
{
    const bool wasRunning = isRunning();
    m_driver->stop();
    const bool targetChangedNow = m_target != value;
    const bool valueChangedNow = m_value != value;
    const bool velocityChangedNow = m_velocity != 0;
    m_target = value;
    m_value = value;
    m_velocity = 0;
    if (targetChangedNow)
        Q_EMIT targetChanged();
    if (valueChangedNow)
        Q_EMIT valueChanged();
    if (velocityChangedNow)
        Q_EMIT velocityChanged();
    if (wasRunning)
        Q_EMIT runningChanged();
}

void Spring::complete()
{
    const bool moved = m_value != m_target;
    const bool hadVelocity = m_velocity != 0;
    m_value = m_target;
    m_velocity = 0;
    if (moved)
        Q_EMIT valueChanged();
    if (hadVelocity)
        Q_EMIT velocityChanged();
    stop(moved || hadVelocity);
}

void Spring::updateThreshold()
{
    // Settle relative to the distance travelled so pixel-scale and unit-scale
    // springs both come to rest imperceptibly.
    m_threshold = std::max(1e-4, std::abs(m_target - m_value) * 1e-4);
}

void Spring::start()
{
    updateThreshold();
    if (isRunning())
        return;
    m_lastTimeMs = 0;
    m_driver->start();
    Q_EMIT runningChanged();
}

void Spring::stop(bool emitSettled)
{
    const bool wasRunning = isRunning();
    m_driver->stop();
    if (wasRunning)
        Q_EMIT runningChanged();
    if (emitSettled)
        Q_EMIT settled();
}

void Spring::advanceTo(int animationTimeMs)
{
    const int deltaMs = animationTimeMs - m_lastTimeMs;
    m_lastTimeMs = animationTimeMs;
    if (deltaMs <= 0)
        return;
    step(std::min(deltaMs / 1000.0, kMaxStepSeconds));
}

void Spring::step(qreal seconds)
{
    const qreal omega = 2.0 * std::numbers::pi / m_response;
    const State next = evolve({ m_value - m_target, m_velocity }, omega, m_damping, seconds);

    if (std::abs(next.x) < m_threshold && std::abs(next.v) < m_threshold * 10.0) {
        complete();
        return;
    }

    m_value = m_target + next.x;
    m_velocity = next.v;
    Q_EMIT valueChanged();
    Q_EMIT velocityChanged();
}

void Spring::classBegin() { m_initializing = true; }

void Spring::componentComplete()
{
    m_initializing = false;
    if (!m_valueSetDuringInit) {
        // Declared without an explicit start value: begin at rest on the target.
        if (m_value != m_target) {
            m_value = m_target;
            Q_EMIT valueChanged();
        }
        return;
    }
    if (m_value != m_target || m_velocity != 0) {
        if (shouldAnimate())
            start();
        else
            complete();
    }
}

} // namespace qtos
