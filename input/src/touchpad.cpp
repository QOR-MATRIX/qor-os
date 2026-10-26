// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "touchpad.h"

#include <utility>

namespace qtos::input {

std::optional<QList<TouchpadContact>> TouchpadFrames::add(int contactCount, const QList<TouchpadContact> &contacts)
{
    if (contactCount > 0) {
        // A new frame; an incomplete one before it is dropped.
        m_frame.clear();
        m_expected = contactCount;
    }
    for (const TouchpadContact &contact : contacts) {
        if (m_frame.size() >= m_expected)
            break;
        m_frame.append(contact);
    }
    if (m_expected == 0 || m_frame.size() < m_expected)
        return std::nullopt;
    m_expected = 0;
    return std::exchange(m_frame, {});
}

QList<TouchpadSwipe::Event> TouchpadSwipe::add(const QList<TouchpadContact> &frame)
{
    QList<TouchpadContact> touching;
    for (const TouchpadContact &contact : frame)
        if (contact.touching)
            touching.append(contact);

    QList<Event> events;
    const int fingers = int(touching.size());
    if (!m_active) {
        if (fingers >= minimumFingers) {
            m_active = true;
            m_fingers = fingers;
            events.append({ Phase::Begin, {}, fingers });
        }
    } else if (fingers < minimumFingers) {
        m_active = false;
        events.append({ Phase::End, {}, m_fingers });
    } else {
        QPointF sum;
        int matched = 0;
        for (const TouchpadContact &contact : std::as_const(touching)) {
            for (const TouchpadContact &before : std::as_const(m_previous)) {
                if (before.id == contact.id) {
                    sum += contact.position - before.position;
                    ++matched;
                    break;
                }
            }
        }
        m_fingers = fingers;
        const QPointF delta = matched ? sum / matched * pixelsPerMillimetre : QPointF();
        if (!delta.isNull())
            events.append({ Phase::Update, delta, fingers });
    }
    m_previous = touching;
    return events;
}

std::optional<TouchpadSwipe::Event> TouchpadSwipe::reset()
{
    m_previous.clear();
    if (!m_active)
        return std::nullopt;
    m_active = false;
    return Event { Phase::End, {}, m_fingers };
}

} // namespace qtos::input
