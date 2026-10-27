// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include "lumenpalette.h"

#include <QtWidgets/QProxyStyle>

namespace qor {

// Lumen for Qt Widgets: a proxy over Fusion that applies the Lumen palette,
// typography, radii and control shapes, so classic Widgets apps (Krita, OBS,
// Wireshark, VLC, KeePassXC…) sit naturally next to QML apps.
//
// Widgets cannot get Glass, but they get the same colors, type, focus rings,
// rounded geometry and accent behaviour. Select it with `-style Lumen`,
// QT_STYLE_OVERRIDE=Lumen, or the qor platform theme on device.
class LumenStyle : public QProxyStyle
{
    Q_OBJECT

public:
    explicit LumenStyle(const lumen::Appearance &appearance = lumen::appearanceFromEnvironment());

    void setAppearance(const lumen::Appearance &appearance);
    lumen::Appearance appearance() const { return m_appearance; }

    QPalette standardPalette() const override;
    void polish(QPalette &palette) override;
    void polish(QApplication *app) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                           const QWidget *widget) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl,
                         const QWidget *widget = nullptr) const override;

private:
    QColor role(const lumen::ColorRole &role) const { return lumen::resolve(role, m_appearance); }

    lumen::Appearance m_appearance;
};

} // namespace qor
