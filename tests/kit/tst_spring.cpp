// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "kit.h"
#include "spring.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>

using namespace Qt::StringLiterals;
using qor::Spring;

class tst_Spring : public QObject
{
    Q_OBJECT

private:
    // Steps the simulation at a fixed 120 Hz until it settles or times out.
    static int settle(Spring &spring, double maxSeconds = 5.0)
    {
        int steps = 0;
        while (spring.isRunning() && steps < maxSeconds * 120) {
            spring.step(1.0 / 120.0);
            ++steps;
        }
        return steps;
    }

private Q_SLOTS:
    void init() { qor::Kit::instance()->setReducedMotion(false); }

    void startsAtRestOnTarget()
    {
        Spring spring;
        spring.classBegin();
        spring.setTarget(42);
        spring.componentComplete();
        QCOMPARE(spring.value(), 42.0);
        QVERIFY(!spring.isRunning());
    }

    void explicitStartValueAnimates()
    {
        Spring spring;
        spring.classBegin();
        spring.setValue(0);
        spring.setTarget(1);
        spring.componentComplete();
        QVERIFY(spring.isRunning());
        settle(spring);
        QCOMPARE(spring.value(), 1.0);
        QVERIFY(!spring.isRunning());
    }

    void criticallyDampedNeverOvershoots()
    {
        Spring spring;
        spring.setDamping(1.0);
        spring.setResponse(0.4);
        spring.snap(0);
        spring.setTarget(100);
        double peak = 0;
        while (spring.isRunning()) {
            spring.step(1.0 / 240.0);
            peak = std::max(peak, spring.value());
        }
        QVERIFY2(peak <= 100.0 + 1e-6, qPrintable(u"peak %1"_s.arg(peak)));
        QCOMPARE(spring.value(), 100.0);
    }

    void underdampedOvershoots()
    {
        Spring spring;
        spring.setDamping(0.5);
        spring.setResponse(0.4);
        spring.snap(0);
        spring.setTarget(100);
        double peak = 0;
        while (spring.isRunning()) {
            spring.step(1.0 / 240.0);
            peak = std::max(peak, spring.value());
        }
        QVERIFY(peak > 105.0);
    }

    void overdampedConverges()
    {
        Spring spring;
        spring.setDamping(2.0);
        spring.snap(10);
        spring.setTarget(-10);
        settle(spring, 10);
        QCOMPARE(spring.value(), -10.0);
    }

    void responseRoughlyMatchesDuration()
    {
        // A critically damped spring is within 2% of its target after one
        // response period: (1 + 2π)·e^(−2π) ≈ 1.36%.
        Spring spring;
        spring.setDamping(1.0);
        spring.setResponse(0.5);
        spring.snap(0);
        spring.setTarget(1);
        for (int i = 0; i < 60; ++i) // 0.5 s at 120 Hz
            spring.step(1.0 / 120.0);
        QVERIFY(std::abs(spring.value() - 1.0) < 0.02);
    }

    void retargetPreservesVelocity()
    {
        Spring spring;
        spring.setDamping(1.0);
        spring.snap(0);
        spring.setTarget(100);
        for (int i = 0; i < 6; ++i)
            spring.step(1.0 / 120.0);
        const double velocityBefore = spring.velocity();
        QVERIFY(velocityBefore > 0);
        spring.setTarget(-100);
        QCOMPARE(spring.velocity(), velocityBefore);
        spring.step(1.0 / 120.0);
        QVERIFY(spring.value() > 0); // momentum carries it forward before it turns
    }

    void velocityKick()
    {
        Spring spring;
        spring.snap(0);
        spring.setVelocity(-400);
        QVERIFY(spring.isRunning());
        spring.step(1.0 / 60.0);
        QVERIFY(spring.value() < 0);
        settle(spring);
        QCOMPARE(spring.value(), 0.0);
    }

    void snapStopsImmediately()
    {
        Spring spring;
        spring.snap(0);
        spring.setTarget(50);
        QVERIFY(spring.isRunning());
        spring.snap(20);
        QVERIFY(!spring.isRunning());
        QCOMPARE(spring.value(), 20.0);
        QCOMPARE(spring.target(), 20.0);
        QCOMPARE(spring.velocity(), 0.0);
    }

    void settledEmittedOnce()
    {
        Spring spring;
        spring.snap(0);
        QSignalSpy settled(&spring, &Spring::settled);
        spring.setTarget(1);
        settle(spring);
        QCOMPARE(settled.count(), 1);
    }

    void presetAppliesParameters()
    {
        Spring spring;
        spring.setPreset({ { u"response"_s, 0.28 }, { u"damping"_s, 0.9 } });
        QCOMPARE(spring.response(), 0.28);
        QCOMPARE(spring.damping(), 0.9);
    }

    void reducedMotionSnaps()
    {
        qor::Kit::instance()->setReducedMotion(true);
        Spring spring;
        spring.snap(0);
        spring.setTarget(10);
        QVERIFY(!spring.isRunning());
        QCOMPARE(spring.value(), 10.0);
    }

    void disabledFollowsTarget()
    {
        Spring spring;
        spring.setEnabled(false);
        spring.setTarget(7);
        QCOMPARE(spring.value(), 7.0);
        QVERIFY(!spring.isRunning());
    }

    void largeStepsAreStable()
    {
        // The closed-form integrator must not explode with coarse time steps.
        Spring spring;
        spring.setDamping(0.3);
        spring.snap(0);
        spring.setTarget(1000);
        for (int i = 0; i < 50 && spring.isRunning(); ++i) {
            spring.step(0.1);
            QVERIFY(std::isfinite(spring.value()));
            QVERIFY(std::abs(spring.value()) < 5000);
        }
    }
};

QTEST_MAIN(tst_Spring)
#include "tst_spring.moc"
