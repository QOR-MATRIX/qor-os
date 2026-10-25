// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtCore/qnamespace.h>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

namespace qtos::input {

// Input QTOS makes itself (the controller interface, shell scripts) enters
// through QWindowSystemInterface like a device's. Inside an InjectedInput
// scope it is delivered synchronously and isInjecting() is true, so a filter
// can tell it from a person's: a scripted test drops real input, which would
// otherwise cancel its drags.
class InjectedInput
{
public:
    InjectedInput();
    ~InjectedInput();
    Q_DISABLE_COPY_MOVE(InjectedInput)

private:
    bool m_wasSynchronous;
};

bool isInjecting();

// Presses and releases a key as a keyboard does: the modifier keys go down
// first and come up last, each with key events of its own. In Prism,
// QtWaylandCompositor rebuilds every key event from its XKB state, which only
// modifier key events change, so modifiers given as flags alone would be
// lost there. Shift is added when the key needs it. Returns whether anything
// accepted the key press.
bool sendKeyStroke(QWindow *window, ulong timestamp, int key, Qt::KeyboardModifiers modifiers,
                   const QString &text = {});

} // namespace qtos::input
