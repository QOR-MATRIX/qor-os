// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include "touchpad.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtQml/qqmlregistration.h>
#include <QtQuick/QQuickWindow>

QT_BEGIN_NAMESPACE
class QTouchEvent;
QT_END_NAMESPACE

namespace qtos::input {

// System gestures: swipes with three or more fingers on a touchpad or a
// touchscreen, recognised for the whole window before any item or Wayland
// client inside it sees them.
//
// Touchpad swipes arrive as Qt's native gestures (Wayland pointer gestures,
// macOS) or, on Windows, from TouchpadSource, which SystemGestures turns into
// native gestures for its window while it has focus. A touchscreen swipe
// starts once enough fingers are down and they move past a small slop; the
// items under them then get a touch cancel. Either way the gesture locks to
// the axis it starts along, and reports its translation and velocity in
// logical pixels (and pixels per second).
class SystemGestures : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(int minimumFingers READ minimumFingers WRITE setMinimumFingers NOTIFY minimumFingersChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(Qt::Orientation axis READ axis NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString source READ source NOTIFY activeChanged FINAL)
    Q_PROPERTY(int fingers READ fingers NOTIFY activeChanged FINAL)
    Q_PROPERTY(QPointF translation READ translation NOTIFY translationChanged FINAL)
    Q_PROPERTY(QPointF velocity READ velocity NOTIFY translationChanged FINAL)

public:
    explicit SystemGestures(QObject *parent = nullptr);
    ~SystemGestures() override;

    QQuickWindow *window() const { return m_window; }
    void setWindow(QQuickWindow *window);
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    int minimumFingers() const { return m_minimumFingers; }
    void setMinimumFingers(int fingers);

    bool isActive() const { return m_active; }
    Qt::Orientation axis() const { return m_axis; }
    QString source() const;
    int fingers() const { return m_fingers; }
    QPointF translation() const { return m_translation; }
    QPointF velocity() const { return m_velocity; }

    // Distance the fingers travel before a swipe picks its axis.
    static constexpr qreal kSlop = 10;

Q_SIGNALS:
    void windowChanged();
    void enabledChanged();
    void minimumFingersChanged();
    void activeChanged();
    void translationChanged();
    // The swipe has picked its axis; translation follows from here on.
    void started();
    // The fingers lifted; `velocity` is the release velocity.
    void finished();
    // The swipe stopped without a release (focus lost, disabled).
    void canceled();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class Source { None, Touchpad, Touchscreen };

    bool touchEvent(QTouchEvent *event);
    void touchpadSwipe(TouchpadSwipe::Phase phase, QPointF delta, int fingers);
    void begin(Source source, int fingers, quint64 timestamp);
    void move(QPointF translation, quint64 timestamp);
    void release(quint64 timestamp);
    void cancel();
    void reset();
    QPointF velocityAt(quint64 timestamp) const;

    QPointer<QQuickWindow> m_window;
    bool m_enabled = true;
    int m_minimumFingers = 3;

    Source m_source = Source::None;
    bool m_active = false;
    Qt::Orientation m_axis = Qt::Orientation(0);
    int m_fingers = 0;
    QPointF m_translation;
    QPointF m_velocity;
    struct Sample
    {
        quint64 timestamp;
        QPointF translation;
    };
    QList<Sample> m_samples;

    // A touchscreen swipe in the making, or under way: the fingers' last
    // positions, by point id.
    bool m_touchCandidate = false;
    QPointF m_touchStart;
    QHash<int, QPointF> m_touchPoints;
    bool m_sendingCancel = false;

    // Whether a touchpad swipe from TouchpadSource is being delivered.
    bool m_touchpadInjecting = false;
};

} // namespace qtos::input
