// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtGui/QColor>
#include <QtQml/qqmlregistration.h>

#include "oklchmath.h"

namespace qtos {

// QML singleton `Oklch`.
class Oklch : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Oklch(QObject *parent = nullptr);

    // Builds a color from OKLCH components (hue in degrees).
    Q_INVOKABLE QColor color(double l, double c, double h, double alpha = 1.0) const;

    // Resolves a Lumen color-role token for the given darkness (0 = light,
    // 1 = dark) and contrast (0 = standard, 1 = Increase Contrast). Token keys:
    //   light, dark     [L, C, H, A]
    //   highContrast    optional {light, dark} blended in by `contrast`
    //   followAccent    the token hue is an offset from accentHue
    //   maxLuminance    optional cap on WCAG relative luminance (lowers L)
    Q_INVOKABLE QColor themed(const QVariantMap &token, double darkness, double accentHue, double contrast = 0.0) const;

    // Interpolates two colors in OKLab space.
    Q_INVOKABLE QColor mix(const QColor &from, const QColor &to, double t) const;

    // Returns [L, C, H, A] for a color.
    Q_INVOKABLE QVariantList toOklch(const QColor &color) const;

    // Returns `color` with its alpha replaced.
    Q_INVOKABLE QColor alpha(const QColor &color, double alpha) const;
};

} // namespace qtos
