// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "oklch.h"

#include <QtTest/QTest>

#include <cmath>

using namespace Qt::StringLiterals;

class tst_Oklch : public QObject
{
    Q_OBJECT

private:
    static bool near(double a, double b, double eps) { return std::abs(a - b) <= eps; }

private Q_SLOTS:
    void whiteAndBlack()
    {
        const QColor white = qtos::color::fromOklch(1.0, 0.0, 0.0);
        const QColor black = qtos::color::fromOklch(0.0, 0.0, 0.0);
        QCOMPARE(white.red(), 255);
        QCOMPARE(white.green(), 255);
        QCOMPARE(white.blue(), 255);
        QCOMPARE(black.red(), 0);
        QCOMPARE(black.green(), 0);
        QCOMPARE(black.blue(), 0);
    }

    void roundTrip_data()
    {
        QTest::addColumn<QColor>("color");
        QTest::newRow("red") << QColor(255, 0, 0);
        QTest::newRow("green") << QColor(0, 200, 80);
        QTest::newRow("blue") << QColor(40, 90, 255);
        QTest::newRow("gray") << QColor(128, 128, 128);
        QTest::newRow("lumen-indigo") << QColor(0x5b, 0x69, 0xf0);
    }

    void roundTrip()
    {
        QFETCH(QColor, color);
        qtos::Oklch oklch;
        const QVariantList lch = oklch.toOklch(color);
        const QColor back = oklch.color(lch[0].toDouble(), lch[1].toDouble(), lch[2].toDouble(), 1.0);
        QVERIFY2(std::abs(back.red() - color.red()) <= 1, qPrintable(back.name()));
        QVERIFY2(std::abs(back.green() - color.green()) <= 1, qPrintable(back.name()));
        QVERIFY2(std::abs(back.blue() - color.blue()) <= 1, qPrintable(back.name()));
    }

    void knownReference()
    {
        // sRGB red is approximately OKLCH(0.628, 0.258, 29.2°).
        qtos::Oklch oklch;
        const QVariantList lch = oklch.toOklch(QColor(255, 0, 0));
        QVERIFY(near(lch[0].toDouble(), 0.628, 0.002));
        QVERIFY(near(lch[1].toDouble(), 0.258, 0.002));
        QVERIFY(near(lch[2].toDouble(), 29.23, 0.2));
    }

    void outOfGamutIsMappedNotClipped()
    {
        // Very high chroma must stay in gamut while preserving hue.
        const QColor c = qtos::color::fromOklch(0.7, 0.4, 150.0);
        QVERIFY(c.isValid());
        qtos::Oklch oklch;
        const QVariantList lch = oklch.toOklch(c);
        QVERIFY2(near(lch[2].toDouble(), 150.0, 3.0), qPrintable(QString::number(lch[2].toDouble())));
        QVERIFY(near(lch[0].toDouble(), 0.7, 0.02));
    }

    void themedResolvesLightAndDark()
    {
        qtos::Oklch oklch;
        const QVariantMap token { { u"light"_s, QVariantList { 0.98, 0.0, 0.0, 1.0 } },
                                  { u"dark"_s, QVariantList { 0.2, 0.0, 0.0, 0.5 } },
                                  { u"followAccent"_s, false } };
        const QColor light = oklch.themed(token, 0.0, 274);
        const QColor dark = oklch.themed(token, 1.0, 274);
        QVERIFY(light.lightnessF() > 0.9);
        QVERIFY(dark.lightnessF() < 0.2);
        QVERIFY(near(dark.alphaF(), 0.5, 0.01));
    }

    void themedFollowsAccent()
    {
        qtos::Oklch oklch;
        const QVariantMap token { { u"light"_s, QVariantList { 0.6, 0.18, 0.0, 1.0 } },
                                  { u"dark"_s, QVariantList { 0.6, 0.18, 0.0, 1.0 } },
                                  { u"followAccent"_s, true } };
        const QVariantList hueA = oklch.toOklch(oklch.themed(token, 1.0, 150));
        const QVariantList hueB = oklch.toOklch(oklch.themed(token, 1.0, 25));
        QVERIFY(near(hueA[2].toDouble(), 150, 3));
        QVERIFY(near(hueB[2].toDouble(), 25, 3));
    }

    void mixIsPerceptual()
    {
        // OKLab midpoint of black and white has lightness ~0.5 (sRGB ~#636363),
        // unlike the naive sRGB midpoint (#808080).
        qtos::Oklch oklch;
        const QColor mid = oklch.mix(Qt::black, Qt::white, 0.5);
        QVERIFY2(std::abs(mid.red() - 99) <= 2, qPrintable(mid.name()));
    }
};

QTEST_MAIN(tst_Oklch)
#include "tst_oklch.moc"
