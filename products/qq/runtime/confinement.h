// Confinement: what a game's logic may reach in the QQ Player (ADR-087 decision 2).
//
// A game's scenes and logic are QML and JavaScript, run in the Player's engine. Confined, that engine:
//
//   loads QML, scripts and files only from the game's own package (its project folder, or the executable's bundled
//     game) and from Qt's and QQ's module folders;
//   imports only the modules a game needs (allowedModules()): Qt Quick, Quick 3D and its physics, particles, helpers and
//     spatial audio, Controls, and QQ; not storage, settings, devices, dialogs, sockets or workers;
//   makes no network request at all;
//   reads scenes only from the package through SceneIO, and writes or copies no file (SceneIO::confineTo) but its one
//     store, which the runtime keeps for it under a name the game does not choose (store.h, P3.16); every file
//     a scene names (a model, a sound, a picture, a sky, a map) is found through SceneIO::resolvedUrl, which gives
//     nothing outside the package, since Qt Quick 3D reads pictures and models past the engine's interceptor;
//   refuses to hand URLs to the operating system (Qt.openUrlExternally) for every scheme in refusedSchemes().
//
// It is a second wall, not the boundary: a stranger's game plays only in the browser, whose sandbox is the boundary
// (ADR-087 decision 1), and on Linux the QQ Player also walls itself in with the kernel (player/sandbox.h). This wall has
// limits that only those can close, and QOR OS's QOR-0018 keeps the list:
//
//   Qt Multimedia (its camera, microphone, screen capture and media recorder types among them) cannot be refused
//     natively: Qt's spatial audio library links Qt Multimedia's QML library, which registers its types when it loads,
//     and a module whose types are registered is imported without its qmldir, which is all an interceptor sees.
//   Qt Quick reads and writes some files past every interceptor: a Canvas's drawImage reads any picture, and an item's
//     grabToImage saves over any picture a person can write.
//
// Changed for QOR OS: see confinement.cpp.

#pragma once

#include <QStringList>
#include <QUrl>

class QQmlEngine;

namespace qq {

class Confinement
{
public:
    /// Confine `engine` to the game whose package is `package` (a folder: file: or qrc:). Call before anything is
    /// loaded into the engine. There is no way back.
    static void apply(QQmlEngine *engine, const QUrl &package);

    /// The package a scene file belongs to: the folder above `scenes/`, or the scene's own folder.
    static QUrl packageOf(const QUrl &scene);

    /// The modules a confined game may import, as module paths ("QtQuick/Layouts").
    static const QStringList &allowedModules();
    /// The URL schemes refused to Qt.openUrlExternally.
    static const QStringList &refusedSchemes();

    /// Where a file really is, through any symbolic link: a link in a package to a file elsewhere is elsewhere. What is
    /// not there yet is taken from the real place of the nearest folder that is.
    static QString realPath(const QString &path);
    /// Whether real path `where` is `root` or lies inside it, by the file system's own rule of case.
    static bool within(const QString &where, const QString &root);
    /// Whether a folder is too wide to be a game's package: a person's home, Downloads, Documents and the like, or one
    /// that holds them. A scene saved straight there would otherwise reach all of it.
    static bool tooWide(const QUrl &package);
    /// What the Studio asks, for its engine, since it runs a project's own code: no module but from the import folders,
    /// no network, and nothing opened outside.
    static void guardStudio(QQmlEngine *engine);

    /// What has been refused so far, oldest first (each "kind: what"): for the log and the checks.
    static QStringList refusals();
};

}  // namespace qq
