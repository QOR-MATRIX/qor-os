// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "oklch.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Qt::StringLiterals;

namespace qtos {

Oklch::Oklch(QObject *parent) : QObject(parent) { }

QColor Oklch::color(double l, double c, double h, double alpha) const { return color::fromOklch(l, c, h, alpha); }

QColor Oklch::themed(const QVariantMap &token, double darkness, double accentHue) const
{
    const QVariantList light = token.value(u"light"_s).toList();
    const QVariantList dark = token.value(u"dark"_s).toList();
    if (light.size() < 4 || dark.size() < 4)
        return {};

    const double hueOffset = token.value(u"followAccent"_s).toBool() ? accentHue : 0.0;
    auto lab = [hueOffset](const QVariantList &v) {
        return color::oklchToOklab(v[0].toDouble(), v[1].toDouble(), v[2].toDouble() + hueOffset);
    };

    const double t = std::clamp(darkness, 0.0, 1.0);
    const color::Lab a = lab(light);
    const color::Lab b = lab(dark);
    const color::Lab m { a.l + (b.l - a.l) * t, a.a + (b.a - a.a) * t, a.b + (b.b - a.b) * t };
    const double alpha = light[3].toDouble() + (dark[3].toDouble() - light[3].toDouble()) * t;
    return color::oklabToSrgb(m, alpha);
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
