// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "lumentokens.h"

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPalette>

// Lumen for classic Qt APIs (QPalette, QFont): shared by the Lumen QStyle and
// the qtos QPA platform theme so Qt Widgets apps match the QML shell.
namespace qtos::lumen {

struct Appearance
{
    bool dark = true;
    double accentHue = kAccentDefaultHue;
    bool highContrast = false;
};

// Reads QTOS_APPEARANCE (light|dark), QTOS_ACCENT_HUE and QTOS_CONTRAST
// (standard|high). On device the platform theme feeds this from
// qtos-settingsd instead.
Appearance appearanceFromEnvironment();

QColor resolve(const ColorRole &role, const Appearance &appearance);
QPalette palette(const Appearance &appearance);
QFont uiFont(int pixelSize = kTypeBodySize);
QFont monoFont(int pixelSize = kTypeBodySize);

} // namespace qtos::lumen
