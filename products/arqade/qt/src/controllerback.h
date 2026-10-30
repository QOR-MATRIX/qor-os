// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// ControllerBack: a controller's Back (View, Share, Create), the way back from a game to ARQADE's home (GameView).
//
// It is read from QOR OS's controller service, as QQ's Input reads the rest of the controller, but it is not one of
// QQ's actions: a game hears every action pressed while it plays and keeps it in its run, and Back is not the game's.
// Heard only while ARQADE is the application in front, as the game is.

#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace arqade {

class ControllerBack : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ControllerBack(QObject *parent = nullptr);

signals:
    /// Back went down on any controller, ARQADE in front.
    void pressed();

private:
    void read();

    bool m_down = false;
};

}  // namespace arqade
