// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtGui/QColor>

// Perceptual color math shared by QOR.Kit (QML), the Lumen QStyle and the
// QPA platform theme. Depends on Qt Gui only.
//
// Lumen defines every color in OKLCH (lightness, chroma, hue). Converting at
// runtime lets the accent hue and light/dark appearance animate smoothly:
// interpolation happens in OKLab, which keeps perceived lightness linear and
// avoids the muddy midpoints of sRGB blending.
namespace qor::color {

struct Lab
{
    double l = 0;
    double a = 0;
    double b = 0;
};

Lab oklchToOklab(double l, double c, double hueDegrees);
Lab srgbToOklab(const QColor &color);
// Converts OKLab to sRGB, reducing chroma (hue and lightness preserved) when
// the color falls outside the sRGB gamut.
QColor oklabToSrgb(const Lab &lab, double alpha = 1.0);
QColor fromOklch(double l, double c, double hueDegrees, double alpha = 1.0);
QColor mixOklab(const QColor &from, const QColor &to, double t);

// WCAG 2 relative luminance (0–1) of the color as displayed (gamut-mapped sRGB).
double relativeLuminance(const Lab &lab);
// Lowers lightness, keeping hue and chroma, until the relative luminance is at
// most `maxLuminance`. Lumen uses this to guarantee text contrast on accent
// fills for any accent hue: OKLCH lightness alone does not fix luminance.
Lab capLuminance(const Lab &lab, double maxLuminance);

} // namespace qor::color
