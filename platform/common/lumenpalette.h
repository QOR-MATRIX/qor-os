// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "lumentokens.h"

#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPalette>

// Lumen for classic Qt APIs (QPalette, QFont): shared by the Lumen QStyle and
// the qor QPA platform theme so Qt Widgets apps match the QML shell.
namespace qor::lumen {

struct Appearance
{
    bool dark = true;
    double accentHue = kAccentDefaultHue;
    bool highContrast = false;
};

// Reads QOR_APPEARANCE (light|dark), QOR_ACCENT_HUE and QOR_CONTRAST
// (standard|high). On device the platform theme feeds this from
// qor-settingsd instead.
Appearance appearanceFromEnvironment();

QColor resolve(const ColorRole &role, const Appearance &appearance);
QPalette palette(const Appearance &appearance);
QFont uiFont(int pixelSize = kTypeBodySize);
QFont monoFont(int pixelSize = kTypeBodySize);

} // namespace qor::lumen
