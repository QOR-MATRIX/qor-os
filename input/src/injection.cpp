// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "injection.h"

#include "keycodes.h"

#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtGui/qpa/qwindowsysteminterface_p.h>

#include <iterator>

namespace qtos::input {

namespace {

// Injection happens on the GUI thread only.
int injectionDepth = 0;

// The modifier keys, in the order sendKeyStroke() presses them.
struct ModifierKey
{
    Qt::KeyboardModifier modifier;
    int key;
};
constexpr ModifierKey modifierKeys[] = {
    { Qt::ControlModifier, Qt::Key_Control },
    { Qt::AltModifier, Qt::Key_Alt },
    { Qt::ShiftModifier, Qt::Key_Shift },
    { Qt::MetaModifier, Qt::Key_Meta },
};

} // namespace

InjectedInput::InjectedInput() : m_wasSynchronous(QWindowSystemInterfacePrivate::synchronousWindowSystemEvents)
{
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
    ++injectionDepth;
}

InjectedInput::~InjectedInput()
{
    --injectionDepth;
    QWindowSystemInterface::setSynchronousWindowSystemEvents(m_wasSynchronous);
}

bool isInjecting() { return injectionDepth > 0; }

bool sendKeyStroke(QWindow *window, ulong timestamp, int key, Qt::KeyboardModifiers modifiers, const QString &text)
{
    const InjectedInput injected;
    if (needsShift(key))
        modifiers |= Qt::ShiftModifier;
    const auto send = [&](QEvent::Type type, int k, Qt::KeyboardModifiers mods, const QString &t = {}) {
        // Unknown keys go without a scan code; QtWaylandCompositor then looks
        // one up in its keymap.
        const quint32 code = evdevKeyCode(k);
        return QWindowSystemInterface::handleExtendedKeyEvent(window, timestamp, type, k, mods, code ? code + 8 : 0, 0,
                                                              0, t);
    };
    Qt::KeyboardModifiers held;
    for (const ModifierKey &m : modifierKeys) {
        if ((modifiers & m.modifier) && key != m.key) {
            held |= m.modifier;
            send(QEvent::KeyPress, m.key, held);
        }
    }
    const bool accepted = send(QEvent::KeyPress, key, modifiers, text);
    send(QEvent::KeyRelease, key, modifiers, text);
    for (auto m = std::rbegin(modifierKeys); m != std::rend(modifierKeys); ++m) {
        if (held & m->modifier) {
            held &= ~m->modifier;
            send(QEvent::KeyRelease, m->key, held);
        }
    }
    return accepted;
}

} // namespace qtos::input
