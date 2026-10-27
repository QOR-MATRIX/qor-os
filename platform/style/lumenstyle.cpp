// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "lumenstyle.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QAbstractSlider>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QStyleOption>

using namespace Qt::StringLiterals;

namespace qor {

namespace {

constexpr qreal kControlRadius = lumen::kRadiusM - 2; // 7 px on a 30 px control
constexpr qreal kFieldRadius = lumen::kRadiusM - 2;
constexpr int kIndicator = 18;

QPainterPath rounded(const QRectF &rect, qreal radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

void fillRounded(QPainter *p, const QRectF &rect, qreal radius, const QColor &fill,
                 const QColor &stroke = Qt::transparent, qreal strokeWidth = 1.0)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(stroke.alpha() > 0 ? QPen(stroke, strokeWidth) : Qt::NoPen);
    p->setBrush(fill);
    const qreal inset = stroke.alpha() > 0 ? strokeWidth / 2 : 0;
    p->drawPath(rounded(rect.adjusted(inset, inset, -inset, -inset), radius));
    p->restore();
}

void focusRing(QPainter *p, const QRectF &rect, qreal radius, const QColor &color)
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(QPen(color, 2.0));
    p->setBrush(Qt::NoBrush);
    p->drawPath(rounded(rect.adjusted(1, 1, -1, -1), radius));
    p->restore();
}

QColor withAlpha(QColor c, qreal alpha)
{
    c.setAlphaF(float(alpha));
    return c;
}

} // namespace

LumenStyle::LumenStyle(const lumen::Appearance &appearance)
    : QProxyStyle(QStyleFactory::create(u"Fusion"_s)), m_appearance(appearance)
{
    setObjectName(u"Lumen"_s);
}

void LumenStyle::setAppearance(const lumen::Appearance &appearance)
{
    m_appearance = appearance;
    if (qApp)
        QApplication::setPalette(standardPalette());
}

QPalette LumenStyle::standardPalette() const { return lumen::palette(m_appearance); }

void LumenStyle::polish(QPalette &palette) { palette = standardPalette(); }

void LumenStyle::polish(QApplication *app)
{
    QProxyStyle::polish(app);
    app->setFont(lumen::uiFont());
}

void LumenStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (qobject_cast<QAbstractButton *>(widget) || qobject_cast<QComboBox *>(widget)
        || qobject_cast<QAbstractSlider *>(widget) || qobject_cast<QLineEdit *>(widget))
        widget->setAttribute(Qt::WA_Hover, true);
}

void LumenStyle::unpolish(QWidget *widget) { QProxyStyle::unpolish(widget); }

int LumenStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonMargin: return 14;
    case PM_DefaultFrameWidth: return 1;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight: return kIndicator;
    case PM_CheckBoxLabelSpacing:
    case PM_RadioButtonLabelSpacing: return 8;
    case PM_SliderThickness: return 22;
    case PM_SliderLength: return 18;
    case PM_SliderControlThickness: return 18;
    case PM_ScrollBarExtent: return 12;
    case PM_ScrollBarSliderMin: return 36;
    case PM_ToolBarItemSpacing: return 4;
    case PM_FocusFrameHMargin:
    case PM_FocusFrameVMargin: return 2;
    default: return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

int LumenStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                          QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_UnderlineShortcut: return 0;
    case SH_DialogButtonBox_ButtonsHaveIcons: return 0;
    case SH_ScrollBar_ContextMenu: return 0;
    case SH_ComboBox_Popup: return 0;
    case SH_ItemView_ShowDecorationSelected: return 1;
    default: return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

QSize LumenStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size,
                                   const QWidget *widget) const
{
    QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
    switch (type) {
    case CT_PushButton:
        s.setHeight(std::max(s.height(), 30));
        s.setWidth(std::max(s.width(), 80));
        break;
    case CT_LineEdit:
    case CT_ComboBox:
    case CT_SpinBox: s.setHeight(std::max(s.height(), 30)); break;
    default: break;
    }
    return s;
}

void LumenStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *p,
                               const QWidget *widget) const
{
    const bool enabled = option->state & State_Enabled;
    const bool hovered = enabled && (option->state & State_MouseOver);
    const bool sunken = option->state & (State_Sunken | State_On);
    const QColor surface = role(lumen::kSurface);

    switch (element) {
    case PE_PanelButtonCommand: {
        const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
        const bool primary = button && (button->features & QStyleOptionButton::DefaultButton);
        const bool flat = button && (button->features & QStyleOptionButton::Flat);
        QColor fill = primary   ? (sunken    ? role(lumen::kAccent)
                                   : hovered ? role(lumen::kAccentHover)
                                             : role(lumen::kAccent))
                      : sunken  ? role(lumen::kFillControlPressed)
                      : hovered ? role(lumen::kFillControlHover)
                      : flat    ? QColor(Qt::transparent)
                                : role(lumen::kFillControl);
        if (!enabled)
            fill = withAlpha(fill, fill.alphaF() * 0.5);
        fillRounded(p, QRectF(option->rect).adjusted(1, 1, -1, -1), kControlRadius, fill,
                    primary ? QColor(255, 255, 255, 46) : QColor(Qt::transparent));
        return;
    }
    case PE_PanelButtonTool: {
        if (!(option->state & State_AutoRaise) || hovered || sunken) {
            const QColor fill = sunken ? role(lumen::kFillControlPressed) : role(lumen::kFillControlHover);
            fillRounded(p, QRectF(option->rect).adjusted(1, 1, -1, -1), kControlRadius, fill);
        }
        return;
    }
    case PE_FrameFocusRect:
        focusRing(p, QRectF(option->rect).adjusted(-2, -2, 2, 2), kControlRadius + 2, role(lumen::kFocusRing));
        return;
    case PE_PanelLineEdit: {
        const auto *frame = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if (frame && frame->lineWidth <= 0 && !(widget && widget->inherits("QAbstractSpinBox")))
            return;
        const bool focused = option->state & State_HasFocus;
        fillRounded(p, QRectF(option->rect), kFieldRadius,
                    focused ? role(lumen::kSurfaceRaised) : role(lumen::kFillControl),
                    focused ? role(lumen::kFocusRing) : role(lumen::kWindowStroke), focused ? 2.0 : 1.0);
        return;
    }
    case PE_FrameLineEdit: return; // drawn as part of PE_PanelLineEdit
    case PE_IndicatorCheckBox: {
        const bool on = option->state & (State_On | State_NoChange);
        const QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
        fillRounded(p, r, 5,
                    on ? role(lumen::kAccent) : (hovered ? role(lumen::kFillControlHover) : role(lumen::kFillControl)),
                    on ? QColor(Qt::transparent) : role(lumen::kWindowStroke));
        if (on) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing);
            p->setPen(QPen(role(lumen::kOnAccent), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            QPainterPath mark;
            if (option->state & State_NoChange) {
                mark.moveTo(r.left() + r.width() * 0.28, r.center().y());
                mark.lineTo(r.right() - r.width() * 0.28, r.center().y());
            } else {
                mark.moveTo(r.left() + r.width() * 0.26, r.top() + r.height() * 0.53);
                mark.lineTo(r.left() + r.width() * 0.43, r.top() + r.height() * 0.70);
                mark.lineTo(r.left() + r.width() * 0.75, r.top() + r.height() * 0.32);
            }
            p->drawPath(mark);
            p->restore();
        }
        return;
    }
    case PE_IndicatorRadioButton: {
        const bool on = option->state & State_On;
        const QRectF r = QRectF(option->rect).adjusted(1, 1, -1, -1);
        fillRounded(p, r, r.width() / 2,
                    on ? role(lumen::kAccent) : (hovered ? role(lumen::kFillControlHover) : role(lumen::kFillControl)),
                    on ? QColor(Qt::transparent) : role(lumen::kWindowStroke));
        if (on) {
            const qreal d = r.width() * 0.4;
            fillRounded(p, QRectF(r.center().x() - d / 2, r.center().y() - d / 2, d, d), d / 2, role(lumen::kOnAccent));
        }
        return;
    }
    case PE_PanelItemViewItem: {
        if (option->state & State_Selected)
            fillRounded(p, QRectF(option->rect).adjusted(2, 1, -2, -1), lumen::kRadiusS, role(lumen::kFillSelected));
        else if (hovered)
            fillRounded(p, QRectF(option->rect).adjusted(2, 1, -2, -1), lumen::kRadiusS, role(lumen::kFillControl));
        return;
    }
    case PE_FrameGroupBox:
    case PE_Frame: {
        fillRounded(p, QRectF(option->rect), lumen::kRadiusL, Qt::transparent, role(lumen::kWindowStroke));
        return;
    }
    case PE_PanelTipLabel:
        fillRounded(p, QRectF(option->rect), lumen::kRadiusS, role(lumen::kSurfaceRaised), role(lumen::kWindowStroke));
        return;
    case PE_PanelMenu:
    case PE_FrameMenu:
        fillRounded(p, QRectF(option->rect), lumen::kRadiusM, role(lumen::kSurfaceRaised), role(lumen::kWindowStroke));
        return;
    default: Q_UNUSED(surface); QProxyStyle::drawPrimitive(element, option, p, widget);
    }
}

void LumenStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *p,
                             const QWidget *widget) const
{
    switch (element) {
    case CE_PushButtonLabel: {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            QStyleOptionButton copy(*button);
            if (button->features & QStyleOptionButton::DefaultButton)
                copy.palette.setColor(QPalette::ButtonText, role(lumen::kOnAccent));
            QFont font = p->font();
            font.setWeight(QFont::DemiBold);
            p->save();
            p->setFont(font);
            QProxyStyle::drawControl(element, &copy, p, widget);
            p->restore();
            return;
        }
        break;
    }
    case CE_ProgressBarGroove:
        fillRounded(
            p, QRectF(option->rect).adjusted(0, option->rect.height() / 2.0 - 2, 0, -(option->rect.height() / 2.0 - 2)),
            2, role(lumen::kFillControlPressed));
        return;
    case CE_ProgressBarContents:
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const qreal span =
                bar->maximum > bar->minimum ? qreal(bar->progress - bar->minimum) / (bar->maximum - bar->minimum) : 0;
            QRectF r = QRectF(option->rect)
                           .adjusted(0, option->rect.height() / 2.0 - 2, 0, -(option->rect.height() / 2.0 - 2));
            r.setWidth(r.width() * std::clamp(span, 0.0, 1.0));
            fillRounded(p, r, 2, role(lumen::kAccent));
            return;
        }
        break;
    case CE_ProgressBarLabel: return;
    case CE_MenuItem:
        if (const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (item->menuItemType == QStyleOptionMenuItem::Separator) {
                p->fillRect(QRectF(option->rect.left() + 8, option->rect.center().y(), option->rect.width() - 16, 1),
                            role(lumen::kSeparator));
                return;
            }
            if (item->state & State_Selected) {
                fillRounded(p, QRectF(option->rect).adjusted(4, 1, -4, -1), lumen::kRadiusS, role(lumen::kAccent));
                QStyleOptionMenuItem copy(*item);
                copy.palette.setColor(QPalette::HighlightedText, role(lumen::kOnAccent));
                copy.state &= ~State_Selected;
                copy.palette.setColor(QPalette::ButtonText, role(lumen::kOnAccent));
                copy.palette.setColor(QPalette::WindowText, role(lumen::kOnAccent));
                copy.palette.setColor(QPalette::Text, role(lumen::kOnAccent));
                QProxyStyle::drawControl(element, &copy, p, widget);
                return;
            }
        }
        break;
    default: break;
    }
    QProxyStyle::drawControl(element, option, p, widget);
}

QRect LumenStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl sub,
                                 const QWidget *widget) const
{
    if (control == CC_ScrollBar) {
        if (sub == SC_ScrollBarAddLine || sub == SC_ScrollBarSubLine)
            return {};
        if (const auto *bar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QRect groove = option->rect;
            if (sub == SC_ScrollBarGroove)
                return groove;
            if (sub == SC_ScrollBarSlider || sub == SC_ScrollBarAddPage || sub == SC_ScrollBarSubPage) {
                const bool horizontal = bar->orientation == Qt::Horizontal;
                const int length = horizontal ? groove.width() : groove.height();
                const int range = bar->maximum - bar->minimum;
                const int sliderLength = range > 0
                                             ? std::max(pixelMetric(PM_ScrollBarSliderMin, option, widget),
                                                        int(qint64(length) * bar->pageStep / (range + bar->pageStep)))
                                             : length;
                const int start = sliderPositionFromValue(bar->minimum, bar->maximum, bar->sliderPosition,
                                                          length - sliderLength, bar->upsideDown);
                QRect slider = horizontal ? QRect(groove.left() + start, groove.top(), sliderLength, groove.height())
                                          : QRect(groove.left(), groove.top() + start, groove.width(), sliderLength);
                if (sub == SC_ScrollBarSlider)
                    return slider;
                if (sub == SC_ScrollBarSubPage)
                    return horizontal ? QRect(groove.left(), groove.top(), start, groove.height())
                                      : QRect(groove.left(), groove.top(), groove.width(), start);
                return horizontal
                           ? QRect(slider.right() + 1, groove.top(), groove.right() - slider.right(), groove.height())
                           : QRect(groove.left(), slider.bottom() + 1, groove.width(),
                                   groove.bottom() - slider.bottom());
            }
        }
    }
    return QProxyStyle::subControlRect(control, option, sub, widget);
}

void LumenStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *p,
                                    const QWidget *widget) const
{
    switch (control) {
    case CC_Slider:
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QRect groove = subControlRect(CC_Slider, option, SC_SliderGroove, widget);
            const QRect handle = subControlRect(CC_Slider, option, SC_SliderHandle, widget);
            const bool horizontal = slider->orientation == Qt::Horizontal;
            const QRectF track = horizontal ? QRectF(groove.left(), groove.center().y() - 2, groove.width(), 4)
                                            : QRectF(groove.center().x() - 2, groove.top(), 4, groove.height());
            fillRounded(p, track, 2, role(lumen::kFillControlPressed));
            QRectF filled = track;
            if (horizontal)
                filled.setRight(handle.center().x());
            else
                filled.setTop(handle.center().y());
            fillRounded(p, filled, 2,
                        (option->state & State_Enabled) ? role(lumen::kAccent) : role(lumen::kTextDisabled));

            const qreal d = 18;
            const QRectF knob(handle.center().x() - d / 2 + 0.5, handle.center().y() - d / 2 + 0.5, d, d);
            fillRounded(p, knob.translated(0, 1), d / 2, QColor(0, 0, 0, 40));
            fillRounded(p, knob, d / 2, Qt::white, QColor(0, 0, 0, 22));
            if (option->state & State_HasFocus)
                focusRing(p, knob.adjusted(-3, -3, 3, 3), d / 2 + 3, role(lumen::kFocusRing));
            return;
        }
        break;
    case CC_ScrollBar:
        if (qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QRect slider = subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget);
            const bool active = (option->state & State_MouseOver) || (option->activeSubControls & SC_ScrollBarSlider);
            const QColor thumb = withAlpha(role(lumen::kTextPrimary), active ? 0.45 : 0.25);
            const QRectF r = QRectF(slider).adjusted(3, 3, -3, -3);
            fillRounded(p, r, std::min(r.width(), r.height()) / 2, thumb);
            return;
        }
        break;
    default: break;
    }
    QProxyStyle::drawComplexControl(control, option, p, widget);
}

} // namespace qor
