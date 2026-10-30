// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary

#include "controllerback.h"

#include <QGuiApplication>

#include <utility>

#include "gamepadmanager.h"

namespace arqade {

ControllerBack::ControllerBack(QObject *parent) : QObject(parent)
{
    auto *pads = qor::input::GamepadManager::instance();
    connect(pads, &qor::input::GamepadManager::mergedChanged, this, &ControllerBack::read);
    connect(pads, &qor::input::GamepadManager::disconnected, this, &ControllerBack::read);
    m_down = pads->merged().pressed(qor::input::Button::Back);
}

void ControllerBack::read()
{
    const bool down = qor::input::GamepadManager::instance()->merged().pressed(qor::input::Button::Back);
    const bool was = std::exchange(m_down, down);
    if (down && !was && QGuiApplication::applicationState() == Qt::ApplicationActive)
        emit pressed();
}

}  // namespace arqade
