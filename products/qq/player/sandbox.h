// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// The QQ Player walls itself in with the kernel (Linux), before it makes a thread or reads a game: the operating-system
// half of Demiurge's ADR-087, for what QQ's own checks (confinement.h) cannot close. Qt Quick's Canvas and grabToImage
// reach files past every interceptor, and Qt Multimedia's types cannot be refused at all.
//
//   Landlock: the game's package, QOR OS's runtime and the system's own folders may be read; nothing else, so nothing in
//     the person's home but what fonts and sound need. Only the game's store, a cache of the Player's own, what the
//     command line names and the devices it draws with may be written. Nothing may be executed.
//   seccomp: no network socket can be made. Local sockets stay, for the display and sound.
//
// A kernel without Landlock plays the game behind QQ's own checks alone, and says so. QQ_PLAYER_SANDBOX=off plays it with
// neither of these: for finding out whether the wall is what broke something.

#pragma once

#include <QString>
#include <QStringList>

namespace qq {

struct PlayerWall
{
    /// The game's package: read.
    QString package;
    /// What this run was told to write: --capture, --measure, --record. Read and written.
    QStringList writes;
    /// What this run was told to read: --replay.
    QStringList reads;
};

/// Raises the wall, for this process and all it starts. Call with one thread, before QGuiApplication. Returns what is
/// standing, for the log: "landlock+seccomp", "seccomp" or "".
QString raisePlayerWall(const PlayerWall &wall);

}  // namespace qq
