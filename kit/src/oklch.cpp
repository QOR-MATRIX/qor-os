// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "oklch.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Qt::StringLiterals;

namespace qtos {

namespace {

// How a theme retints a neutral token: a replacement hue (when not negative)
// and a chroma multiplier.
struct Tint
{
    double hue = -1.0;
    double chroma = 1.0;
};

// Interpolates a {light, dark} pair of [L, C, H, A] values in OKLab at
// darkness `t`. Returns false when the pair is missing or malformed.
bool blendAppearance(const QVariantMap &pair, double t, double hueOffset, Tint tint, color::Lab &lab, double &alpha)
{
    const QVariantList light = pair.value(u"light"_s).toList();
    const QVariantList dark = pair.value(u"dark"_s).toList();
    if (light.size() < 4 || dark.size() < 4)
        return false;

    auto toLab = [hueOffset, tint](const QVariantList &v) {
        const double hue = tint.hue >= 0.0 ? tint.hue : v[2].toDouble() + hueOffset;
        return color::oklchToOklab(v[0].toDouble(), v[1].toDouble() * tint.chroma, hue);
    };
    const color::Lab a = toLab(light);
    const color::Lab b = toLab(dark);
    lab = { a.l + (b.l - a.l) * t, a.a + (b.a - a.a) * t, a.b + (b.b - a.b) * t };
    alpha = light[3].toDouble() + (dark[3].toDouble() - light[3].toDouble()) * t;
    return true;
}

} // namespace

Oklch::Oklch(QObject *parent) : QObject(parent) { }

QColor Oklch::color(double l, double c, double h, double alpha) const { return color::fromOklch(l, c, h, alpha); }

QColor Oklch::themed(const QVariantMap &token, double darkness, double accentHue, double contrast, double neutralHue,
                     double neutralChroma) const
{
    const bool followAccent = token.value(u"followAccent"_s).toBool();
    const double hueOffset = followAccent ? accentHue : 0.0;
    const double t = std::clamp(darkness, 0.0, 1.0);
    // Accent roles never take a neutral tint.
    const Tint tint = followAccent ? Tint {} : Tint { neutralHue, std::clamp(neutralChroma, 0.0, 3.0) };

    color::Lab lab;
    double alpha = 1.0;
    if (!blendAppearance(token, t, hueOffset, tint, lab, alpha))
        return {};

    const double k = std::clamp(contrast, 0.0, 1.0);
    color::Lab high;
    double highAlpha = 1.0;
    if (k > 0.0 && blendAppearance(token.value(u"highContrast"_s).toMap(), t, hueOffset, tint, high, highAlpha)) {
        lab = { lab.l + (high.l - lab.l) * k, lab.a + (high.a - lab.a) * k, lab.b + (high.b - lab.b) * k };
        alpha += (highAlpha - alpha) * k;
    }

    const QVariant maxLuminance = token.value(u"maxLuminance"_s);
    if (maxLuminance.isValid())
        lab = color::capLuminance(lab, maxLuminance.toDouble());
    return color::oklabToSrgb(lab, alpha);
}

QColor Oklch::mix(const QColor &from, const QColor &to, double t) const { return color::mixOklab(from, to, t); }

QVariantList Oklch::toOklch(const QColor &c) const
{
    const color::Lab lab = color::srgbToOklab(c);
    const double chroma = std::hypot(lab.a, lab.b);
    double hue = std::atan2(lab.b, lab.a) * 180.0 / std::numbers::pi;
    if (hue < 0)
        hue += 360.0;
    return { lab.l, chroma, hue, c.alphaF() };
}

QColor Oklch::alpha(const QColor &c, double alpha) const
{
    QColor out = c;
    out.setAlphaF(float(std::clamp(alpha, 0.0, 1.0)));
    return out;
}

} // namespace qtos
