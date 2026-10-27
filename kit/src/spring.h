// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlParserStatus>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace qor {

class SpringDriver;

// A critically-tunable spring that animates `value` toward `target`.
//
// Lumen uses springs instead of fixed-duration easing curves so every motion
// is interruptible: retargeting mid-flight keeps the current velocity, and
// gesture code can hand its release velocity straight to the spring.
//
// Parameters follow the response/damping model:
//   response  – seconds for one undamped oscillation (perceived duration)
//   damping   – damping ratio; 1 = critically damped, < 1 overshoots
//
// The integration is the closed-form solution of a damped harmonic
// oscillator, so it is exact and frame-rate independent. The spring is ticked
// by Qt's animation driver (vsync-locked in Qt Quick) and stops itself when it
// settles, so idle springs cost nothing.
class Spring : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    Q_PROPERTY(qreal target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal velocity READ velocity WRITE setVelocity NOTIFY velocityChanged FINAL)
    Q_PROPERTY(qreal response READ response WRITE setResponse NOTIFY responseChanged FINAL)
    Q_PROPERTY(qreal damping READ damping WRITE setDamping NOTIFY dampingChanged FINAL)
    Q_PROPERTY(QVariantMap preset READ preset WRITE setPreset NOTIFY presetChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged FINAL)

public:
    explicit Spring(QObject *parent = nullptr);
    ~Spring() override;

    qreal target() const { return m_target; }
    void setTarget(qreal target);

    qreal value() const { return m_value; }
    void setValue(qreal value);

    qreal velocity() const { return m_velocity; }
    void setVelocity(qreal velocity);

    qreal response() const { return m_response; }
    void setResponse(qreal response);

    qreal damping() const { return m_damping; }
    void setDamping(qreal damping);

    QVariantMap preset() const { return m_preset; }
    void setPreset(const QVariantMap &preset);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool isRunning() const;

    // Jumps to `value` (and makes it the target) with zero velocity.
    Q_INVOKABLE void snap(qreal value);
    // Finishes the current motion immediately.
    Q_INVOKABLE void complete();

    // Advances the simulation by `seconds`. Public for deterministic tests.
    void step(qreal seconds);

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void targetChanged();
    void valueChanged();
    void velocityChanged();
    void responseChanged();
    void dampingChanged();
    void presetChanged();
    void enabledChanged();
    void runningChanged();
    // Emitted once when the spring comes to rest at its target.
    void settled();

private:
    friend class SpringDriver;
    void advanceTo(int animationTimeMs);
    void start();
    void stop(bool emitSettled);
    bool shouldAnimate() const;
    void updateThreshold();

    qreal m_target = 0;
    qreal m_value = 0;
    qreal m_velocity = 0;
    qreal m_response = 0.42;
    qreal m_damping = 1.0;
    qreal m_threshold = 1e-4;
    QVariantMap m_preset;
    bool m_enabled = true;
    bool m_initializing = false;
    bool m_valueSetDuringInit = false;
    int m_lastTimeMs = 0;
    std::unique_ptr<SpringDriver> m_driver;
};

} // namespace qor
