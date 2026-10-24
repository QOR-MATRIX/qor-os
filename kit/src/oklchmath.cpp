// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "oklchmath.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace qtos {
namespace color {

namespace {

struct Rgb
{
    double r, g, b;
};

constexpr double kGamutEpsilon = 1e-5;

Rgb oklabToLinear(const Lab &lab)
{
    const double l_ = lab.l + 0.3963377774 * lab.a + 0.2158037573 * lab.b;
    const double m_ = lab.l - 0.1055613458 * lab.a - 0.0638541728 * lab.b;
    const double s_ = lab.l - 0.0894841775 * lab.a - 1.2914855480 * lab.b;
    const double l3 = l_ * l_ * l_;
    const double m3 = m_ * m_ * m_;
    const double s3 = s_ * s_ * s_;
    return {
        +4.0767416621 * l3 - 3.3077115913 * m3 + 0.2309699292 * s3,
        -1.2684380046 * l3 + 2.6097574011 * m3 - 0.3413193965 * s3,
        -0.0041960863 * l3 - 0.7034186147 * m3 + 1.7076147010 * s3,
    };
}

bool inGamut(const Rgb &c)
{
    auto ok = [](double v) { return v >= -kGamutEpsilon && v <= 1.0 + kGamutEpsilon; };
    return ok(c.r) && ok(c.g) && ok(c.b);
}

double encode(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return x <= 0.0031308 ? 12.92 * x : 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
}

double decode(double x) { return x <= 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4); }

// Linear sRGB for an OKLab color, reducing chroma (hue and lightness
// preserved) until it fits the sRGB gamut.
Rgb gamutMappedLinear(const Lab &lab)
{
    const Rgb linear = oklabToLinear(lab);
    if (inGamut(linear) || (lab.a == 0 && lab.b == 0))
        return linear;
    // Binary-search the largest in-gamut chroma at this lightness and hue.
    double lo = 0.0;
    double hi = 1.0;
    for (int i = 0; i < 20; ++i) {
        const double mid = (lo + hi) / 2.0;
        if (inGamut(oklabToLinear({ lab.l, lab.a * mid, lab.b * mid })))
            lo = mid;
        else
            hi = mid;
    }
    return oklabToLinear({ lab.l, lab.a * lo, lab.b * lo });
}

} // namespace

Lab oklchToOklab(double l, double c, double hueDegrees)
{
    const double h = hueDegrees * std::numbers::pi / 180.0;
    return { l, c * std::cos(h), c * std::sin(h) };
}

Lab srgbToOklab(const QColor &color)
{
    const QColor rgb = color.toRgb();
    const double r = decode(rgb.redF());
    const double g = decode(rgb.greenF());
    const double b = decode(rgb.blueF());
    const double l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const double m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const double s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);
    return {
        0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
        1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
        0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s,
    };
}

QColor oklabToSrgb(const Lab &lab, double alpha)
{
    const Rgb linear = gamutMappedLinear(lab);
    return QColor::fromRgbF(float(encode(linear.r)), float(encode(linear.g)), float(encode(linear.b)),
                            float(std::clamp(alpha, 0.0, 1.0)));
}

QColor fromOklch(double l, double c, double hueDegrees, double alpha)
{
    return oklabToSrgb(oklchToOklab(l, c, hueDegrees), alpha);
}

QColor mixOklab(const QColor &from, const QColor &to, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    const Lab a = srgbToOklab(from);
    const Lab b = srgbToOklab(to);
    const Lab m { a.l + (b.l - a.l) * t, a.a + (b.a - a.a) * t, a.b + (b.b - a.b) * t };
    return oklabToSrgb(m, from.alphaF() + (to.alphaF() - from.alphaF()) * t);
}

double relativeLuminance(const Lab &lab)
{
    const Rgb linear = gamutMappedLinear(lab);
    auto channel = [](double x) { return std::clamp(x, 0.0, 1.0); };
    return 0.2126 * channel(linear.r) + 0.7152 * channel(linear.g) + 0.0722 * channel(linear.b);
}

Lab capLuminance(const Lab &lab, double maxLuminance)
{
    if (relativeLuminance(lab) <= maxLuminance)
        return lab;
    // Luminance rises monotonically with lightness at a fixed hue and chroma.
    double lo = 0.0;
    double hi = lab.l;
    for (int i = 0; i < 24; ++i) {
        const double mid = (lo + hi) / 2.0;
        if (relativeLuminance({ mid, lab.a, lab.b }) <= maxLuminance)
            lo = mid;
        else
            hi = mid;
    }
    return { lo, lab.a, lab.b };
}

} // namespace color

} // namespace qtos
