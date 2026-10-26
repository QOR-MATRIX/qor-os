// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "systemgestures.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>
#include <QtGui/QNativeGestureEvent>
#include <QtGui/QPointingDevice>
#include <QtGui/QTouchEvent>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <algorithm>
#include <cmath>

namespace qtos::input {

namespace {

// The window of motion the release velocity is measured over.
constexpr quint64 kVelocityWindowMs = 100;

ulong touchpadTimestamp()
{
    static QElapsedTimer clock;
    if (!clock.isValid())
        clock.start();
    return ulong(clock.elapsed());
}

} // namespace

SystemGestures::SystemGestures(QObject *parent) : QObject(parent)
{
    if (TouchpadSource *source = TouchpadSource::instance())
        connect(source, &TouchpadSource::swipe, this, &SystemGestures::touchpadSwipe);
}

SystemGestures::~SystemGestures()
{
    if (m_window)
        m_window->removeEventFilter(this);
}

void SystemGestures::setWindow(QQuickWindow *window)
{
    if (m_window == window)
        return;
    cancel();
    if (m_window)
        m_window->removeEventFilter(this);
    m_window = window;
    if (m_window)
        m_window->installEventFilter(this);
    Q_EMIT windowChanged();
}

void SystemGestures::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!enabled)
        cancel();
    Q_EMIT enabledChanged();
}

void SystemGestures::setMinimumFingers(int fingers)
{
    fingers = std::max(2, fingers);
    if (m_minimumFingers == fingers)
        return;
    m_minimumFingers = fingers;
    Q_EMIT minimumFingersChanged();
}

QString SystemGestures::source() const
{
    switch (m_source) {
    case Source::Touchpad: return QStringLiteral("touchpad");
    case Source::Touchscreen: return QStringLiteral("touchscreen");
    case Source::None: break;
    }
    return QString();
}

bool SystemGestures::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_window)
        return QObject::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::NativeGesture: {
        auto *gesture = static_cast<QNativeGestureEvent *>(event);
        switch (gesture->gestureType()) {
        case Qt::BeginNativeGesture:
            if (!m_enabled || m_source != Source::None || gesture->fingerCount() < m_minimumFingers)
                return false;
            begin(Source::Touchpad, gesture->fingerCount(), gesture->timestamp());
            return true;
        case Qt::PanNativeGesture:
            if (m_source != Source::Touchpad)
                return false;
            move(m_translation + gesture->delta(), gesture->timestamp());
            return true;
        case Qt::EndNativeGesture:
            if (m_source != Source::Touchpad)
                return false;
            release(gesture->timestamp());
            return true;
        default:
            // Pinches and rotations with as many fingers belong to the swipe.
            return m_source == Source::Touchpad;
        }
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: return touchEvent(static_cast<QTouchEvent *>(event));
    case QEvent::FocusOut:
    case QEvent::Hide: cancel(); break;
    default: break;
    }
    return QObject::eventFilter(watched, event);
}

bool SystemGestures::touchEvent(QTouchEvent *event)
{
    if (m_sendingCancel || !event->device() || event->device()->type() != QInputDevice::DeviceType::TouchScreen)
        return false;

    QHash<int, QPointF> points;
    for (const QEventPoint &point : event->points())
        if (point.state() != QEventPoint::Released)
            points.insert(point.id(), point.scenePosition());
    const auto centroid = [&points] {
        QPointF sum;
        for (const QPointF &p : std::as_const(points))
            sum += p;
        return points.isEmpty() ? QPointF() : sum / points.size();
    };

    if (m_source == Source::Touchscreen) {
        if (event->type() == QEvent::TouchCancel) {
            cancel();
            return false;
        }
        if (points.size() < m_minimumFingers || event->type() == QEvent::TouchEnd) {
            release(event->timestamp());
            m_touchPoints.clear();
        } else {
            // The mean motion of the fingers still down, so one landing or
            // lifting does not make the swipe jump.
            QPointF sum;
            int matched = 0;
            for (auto it = points.cbegin(); it != points.cend(); ++it) {
                const auto before = m_touchPoints.constFind(it.key());
                if (before != m_touchPoints.cend()) {
                    sum += it.value() - before.value();
                    ++matched;
                }
            }
            m_touchPoints = points;
            m_fingers = int(points.size());
            if (matched)
                move(m_translation + sum / matched, event->timestamp());
        }
        event->accept();
        return true;
    }

    if (!m_enabled || m_source != Source::None || points.size() < m_minimumFingers
        || event->type() == QEvent::TouchCancel) {
        m_touchCandidate = false;
        return false;
    }
    if (!m_touchCandidate || points.size() != m_touchPoints.size()) {
        // Enough fingers have landed (or another one has): measure from here.
        m_touchCandidate = true;
        m_touchStart = centroid();
        m_touchPoints = points;
        return false;
    }
    const QPointF moved = centroid() - m_touchStart;
    m_touchPoints = points;
    if (std::hypot(moved.x(), moved.y()) <= kSlop * 1.6)
        return false;

    // A system swipe: what was under the fingers lets go of them.
    m_touchCandidate = false;
    QTouchEvent cancelEvent(QEvent::TouchCancel, event->pointingDevice(), event->modifiers());
    cancelEvent.setTimestamp(event->timestamp());
    m_sendingCancel = true;
    QCoreApplication::sendEvent(m_window, &cancelEvent);
    m_sendingCancel = false;
    begin(Source::Touchscreen, int(points.size()), event->timestamp());
    move(moved, event->timestamp());
    event->accept();
    return true;
}

void SystemGestures::touchpadSwipe(TouchpadSwipe::Phase phase, QPointF delta, int fingers)
{
    // Delivered to this window as native gestures while it has focus, the
    // way Qt delivers them on other platforms.
    QQuickWindow *window = m_window;
    if (!window)
        return;
    const bool focused = QGuiApplication::focusWindow() == window;
    if (phase == TouchpadSwipe::Phase::Begin && (!focused || fingers < m_minimumFingers))
        return;
    if (phase != TouchpadSwipe::Phase::Begin && !m_touchpadInjecting)
        return;
    const QPointF global = QCursor::pos(window->screen());
    const QPointF nativeLocal = QHighDpi::toNativeLocalPosition(window->mapFromGlobal(global), window);
    const QPointF nativeGlobal = QHighDpi::toNativeGlobalPosition(global, window);
    const QPointingDevice *device = TouchpadSource::instance()->device();
    const ulong timestamp = touchpadTimestamp();
    switch (phase) {
    case TouchpadSwipe::Phase::Begin:
        m_touchpadInjecting = true;
        QWindowSystemInterface::handleGestureEvent(window, timestamp, device, Qt::BeginNativeGesture, nativeLocal,
                                                   nativeGlobal, fingers);
        break;
    case TouchpadSwipe::Phase::Update:
        QWindowSystemInterface::handleGestureEventWithValueAndDelta(window, timestamp, device, Qt::PanNativeGesture, 0,
                                                                    delta, nativeLocal, nativeGlobal, fingers);
        break;
    case TouchpadSwipe::Phase::End:
        m_touchpadInjecting = false;
        QWindowSystemInterface::handleGestureEvent(window, timestamp, device, Qt::EndNativeGesture, nativeLocal,
                                                   nativeGlobal, fingers);
        break;
    }
}

void SystemGestures::begin(Source source, int fingers, quint64 timestamp)
{
    m_source = source;
    m_fingers = fingers;
    m_translation = {};
    m_velocity = {};
    m_samples = { { timestamp, {} } };
}

void SystemGestures::move(QPointF translation, quint64 timestamp)
{
    m_translation = translation;
    m_samples.append({ timestamp, translation });
    while (m_samples.size() > 2 && m_samples.constFirst().timestamp + kVelocityWindowMs < timestamp)
        m_samples.removeFirst();
    m_velocity = velocityAt(timestamp);
    if (!m_active && std::hypot(translation.x(), translation.y()) > kSlop) {
        m_axis = std::abs(translation.x()) >= std::abs(translation.y()) ? Qt::Horizontal : Qt::Vertical;
        m_active = true;
        Q_EMIT activeChanged();
        Q_EMIT translationChanged();
        Q_EMIT started();
        return;
    }
    if (m_active)
        Q_EMIT translationChanged();
}

QPointF SystemGestures::velocityAt(quint64 timestamp) const
{
    // Over the motion in the last 100 ms: fingers that stopped before lifting
    // release at rest.
    const Sample *first = nullptr;
    for (const Sample &sample : m_samples) {
        if (sample.timestamp + kVelocityWindowMs >= timestamp) {
            first = &sample;
            break;
        }
    }
    const Sample &last = m_samples.constLast();
    if (!first || last.timestamp <= first->timestamp)
        return {};
    return (last.translation - first->translation) * (1000.0 / qreal(last.timestamp - first->timestamp));
}

void SystemGestures::release(quint64 timestamp)
{
    if (m_source == Source::None)
        return;
    if (m_active) {
        m_velocity = velocityAt(timestamp);
        Q_EMIT translationChanged();
        Q_EMIT finished();
    }
    reset();
}

void SystemGestures::cancel()
{
    m_touchCandidate = false;
    if (m_source == Source::None)
        return;
    if (m_active)
        Q_EMIT canceled();
    reset();
}

void SystemGestures::reset()
{
    const bool wasActive = m_active;
    m_source = Source::None;
    m_active = false;
    m_axis = Qt::Orientation(0);
    m_samples.clear();
    m_touchPoints.clear();
    if (wasActive)
        Q_EMIT activeChanged();
}

} // namespace qtos::input
