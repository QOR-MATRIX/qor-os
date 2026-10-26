// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointF>

#include <optional>

QT_BEGIN_NAMESPACE
class QPointingDevice;
QT_END_NAMESPACE

namespace qtos::input {

// One finger on a touchpad, in millimetres from the pad's top-left corner.
struct TouchpadContact
{
    int id = 0;
    QPointF position;
    bool touching = true; // false for a finger reported as just lifted
};

// Assembles a precision touchpad's input reports into frames. A frame may
// span several reports ("hybrid mode"): the first carries the frame's contact
// count and the rest a count of 0. Every report has the same number of finger
// collections, of which only as many as the frame still needs are valid.
class TouchpadFrames
{
public:
    // `contactCount` is the report's contact count; `contacts` its finger
    // collections in order. Returns the frame this report completes.
    std::optional<QList<TouchpadContact>> add(int contactCount, const QList<TouchpadContact> &contacts);

private:
    QList<TouchpadContact> m_frame;
    int m_expected = 0;
};

// Turns touchpad frames into multi-finger swipes, as Wayland's pointer
// gestures do: a swipe begins when `minimumFingers` fingers touch, moves by
// the mean motion of the fingers present in consecutive frames (so a finger
// landing or lifting does not make it jump), and ends when fewer remain.
class TouchpadSwipe
{
public:
    enum class Phase { Begin, Update, End };
    struct Event
    {
        Phase phase;
        QPointF delta; // logical pixels
        int fingers;
    };

    int minimumFingers = 3;
    // About a pointer's travel for the same finger motion.
    qreal pixelsPerMillimetre = 8;

    QList<Event> add(const QList<TouchpadContact> &frame);
    // Ends a swipe in progress (the touchpad went quiet).
    std::optional<Event> reset();
    bool isActive() const { return m_active; }

private:
    QList<TouchpadContact> m_previous;
    bool m_active = false;
    int m_fingers = 0;
};

// Multi-finger touchpad swipes read from the hardware where Qt does not
// report them: on Windows, precision touchpads through Raw Input. Elsewhere Qt
// already delivers them as native gestures (Wayland pointer gestures, macOS)
// and there is no source. SystemGestures turns these into native gesture
// events for the window in focus, so both arrive the same way.
class TouchpadSource : public QObject
{
    Q_OBJECT

public:
    // nullptr where the platform needs no source, or with QTOS_TOUCHPAD=off.
    static TouchpadSource *instance();
    // The touchpad device the gesture events carry.
    virtual const QPointingDevice *device() const = 0;

Q_SIGNALS:
    void swipe(qtos::input::TouchpadSwipe::Phase phase, QPointF delta, int fingers);

protected:
    using QObject::QObject;
};

} // namespace qtos::input
