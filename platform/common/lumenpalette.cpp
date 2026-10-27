// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "lumenpalette.h"

#include "oklchmath.h"

#include <QtCore/QtEnvironmentVariables>
#include <QtGui/QFontDatabase>

using namespace Qt::StringLiterals;

namespace qor::lumen {

Appearance appearanceFromEnvironment()
{
    Appearance appearance;
    const QString mode = qEnvironmentVariable("QOR_APPEARANCE");
    if (mode == u"light"_s)
        appearance.dark = false;
    bool ok = false;
    const double hue = qEnvironmentVariable("QOR_ACCENT_HUE").toDouble(&ok);
    if (ok)
        appearance.accentHue = hue;
    appearance.highContrast = qEnvironmentVariable("QOR_CONTRAST") == u"high"_s;
    return appearance;
}

QColor resolve(const ColorRole &role, const Appearance &appearance)
{
    const Oklch &v = appearance.highContrast ? (appearance.dark ? role.highContrastDark : role.highContrastLight)
                                             : (appearance.dark ? role.dark : role.light);
    const double hue = v.h + (role.followAccent ? appearance.accentHue : 0.0);
    const color::Lab lab = color::capLuminance(color::oklchToOklab(v.l, v.c, hue), role.maxLuminance);
    return color::oklabToSrgb(lab, v.a);
}

namespace {

// Composites a translucent token over an opaque base so classic widgets,
// which expect opaque palette brushes, get the exact on-screen color.
QColor over(const QColor &top, const QColor &base)
{
    const double a = top.alphaF();
    return QColor::fromRgbF(float(top.redF() * a + base.redF() * (1 - a)),
                            float(top.greenF() * a + base.greenF() * (1 - a)),
                            float(top.blueF() * a + base.blueF() * (1 - a)), 1.0f);
}

} // namespace

QPalette palette(const Appearance &a)
{
    const QColor background = resolve(kBackground, a);
    const QColor surface = resolve(kSurface, a);
    const QColor raised = resolve(kSurfaceRaised, a);
    const QColor sunken = resolve(kSurfaceSunken, a);
    const QColor text = resolve(kTextPrimary, a);
    const QColor secondary = resolve(kTextSecondary, a);
    const QColor tertiary = resolve(kTextTertiary, a);
    const QColor disabled = resolve(kTextDisabled, a);
    const QColor accent = resolve(kAccent, a);
    const QColor onAccent = resolve(kOnAccent, a);
    const QColor accentText = resolve(kAccentText, a);
    const QColor control = over(resolve(kFillControl, a), surface);
    const QColor separator = over(resolve(kSeparator, a), surface);
    const QColor stroke = over(resolve(kWindowStroke, a), surface);

    QPalette p;
    p.setColor(QPalette::Window, surface);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, a.dark ? sunken : raised);
    p.setColor(QPalette::AlternateBase, control);
    p.setColor(QPalette::ToolTipBase, raised);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, tertiary);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, control);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, onAccent);
    p.setColor(QPalette::Light, raised);
    p.setColor(QPalette::Midlight, control);
    p.setColor(QPalette::Mid, stroke);
    p.setColor(QPalette::Dark, separator);
    p.setColor(QPalette::Shadow, background);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, onAccent);
    p.setColor(QPalette::Accent, accent);
    p.setColor(QPalette::Link, accentText);
    p.setColor(QPalette::LinkVisited, accentText);

    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, disabled);
    p.setColor(QPalette::Inactive, QPalette::WindowText, secondary);
    return p;
}

QFont uiFont(int pixelSize)
{
    const QStringList preferred { u"Inter Variable Text"_s, u"Inter Variable"_s, u"Inter"_s };
    const QStringList available = QFontDatabase::families();
    QFont font;
    for (const QString &family : preferred) {
        if (available.contains(family)) {
            font.setFamily(family);
            break;
        }
    }
    font.setPixelSize(pixelSize);
    font.setHintingPreference(QFont::PreferNoHinting);
    return font;
}

QFont monoFont(int pixelSize)
{
    QFont font(u"JetBrains Mono"_s);
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(pixelSize);
    return font;
}

} // namespace qor::lumen
