// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "lumenpalette.h"

#include <QtGui/QFont>
#include <QtGui/QPalette>
#include <QtGui/qpa/qplatformtheme.h>

namespace qor {

// The `qor` QPA platform theme. Loaded by every Qt 5/6 application on a
// QOR OS device (QT_QPA_PLATFORMTHEME=qor), it supplies the Lumen palette,
// Inter / JetBrains Mono fonts, the Lumen widget style, colour scheme and
// UI hints — so third-party Qt apps look native without any changes.
//
// Native dialogs are provided through xdg-desktop-portal (qor-portald), so
// this theme deliberately reports no native dialog support of its own.
class QorPlatformTheme : public QPlatformTheme
{
public:
    QorPlatformTheme();

    const QPalette *palette(Palette type = SystemPalette) const override;
    const QFont *font(Font type = SystemFont) const override;
    QVariant themeHint(ThemeHint hint) const override;
    Qt::ColorScheme colorScheme() const override;

private:
    lumen::Appearance m_appearance;
    QPalette m_palette;
    QFont m_uiFont;
    QFont m_smallFont;
    QFont m_titleFont;
    QFont m_monoFont;
};

} // namespace qor
