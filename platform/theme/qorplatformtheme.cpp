// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "qorplatformtheme.h"

#include <QtGui/qpa/qplatformthemeplugin.h>

using namespace Qt::StringLiterals;

namespace qor {

QorPlatformTheme::QorPlatformTheme()
    : m_appearance(lumen::appearanceFromEnvironment()), m_palette(lumen::palette(m_appearance)),
      m_uiFont(lumen::uiFont(lumen::kTypeBodySize)), m_smallFont(lumen::uiFont(lumen::kTypeFootnoteSize)),
      m_titleFont(lumen::uiFont(lumen::kTypeHeadlineSize)), m_monoFont(lumen::monoFont(lumen::kTypeBodySize))
{
    m_titleFont.setWeight(QFont::DemiBold);
}

const QPalette *QorPlatformTheme::palette(Palette type) const { return type == SystemPalette ? &m_palette : nullptr; }

const QFont *QorPlatformTheme::font(Font type) const
{
    switch (type) {
    case FixedFont:
    case EditorFont: return &m_monoFont;
    case SmallFont:
    case MiniFont:
    case TipLabelFont:
    case StatusBarFont: return &m_smallFont;
    case TitleBarFont:
    case MdiSubWindowTitleFont:
    case DockWidgetTitleFont: return &m_titleFont;
    default: return &m_uiFont;
    }
}

QVariant QorPlatformTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case StyleNames: return QStringList { u"Lumen"_s, u"Fusion"_s };
    case SystemIconThemeName: return u"lumen"_s;
    case SystemIconFallbackThemeName: return u"hicolor"_s;
    case IconThemeSearchPaths: return QStringList { u"/usr/share/icons"_s, u"/usr/local/share/icons"_s };
    case DialogButtonBoxLayout: return QVariant(int(1)); // QDialogButtonBox::KdeLayout ordering
    case KeyboardScheme: return QVariant(int(KdeKeyboardScheme));
    case ShowShortcutsInContextMenus: return true;
    case ShowIconsInMenus: return false;
    case UiEffects:
        return QVariant(int(AnimateMenuUiEffect | FadeMenuUiEffect | AnimateComboUiEffect | AnimateTooltipUiEffect
                            | FadeTooltipUiEffect | HoverEffect));
    case CursorFlashTime: return 1060;
    case MouseDoubleClickInterval: return 400;
    case WheelScrollLines: return 3;
    case MouseCursorTheme: return u"lumen"_s;
    case MouseCursorSize: return QSize(24, 24);
    case PreferFileIconFromTheme: return true;
    default: return QPlatformTheme::themeHint(hint);
    }
}

Qt::ColorScheme QorPlatformTheme::colorScheme() const
{
    return m_appearance.dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

} // namespace qor

class QorPlatformThemePlugin : public QPlatformThemePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformThemeFactoryInterface_iid FILE "qor.json")

public:
    QPlatformTheme *create(const QString &key, const QStringList &) override
    {
        return key.compare(u"qor"_s, Qt::CaseInsensitive) == 0 ? new qor::QorPlatformTheme() : nullptr;
    }
};

#include "qorplatformtheme.moc"
