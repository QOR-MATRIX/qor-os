// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QFutureWatcher>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtQml/qqmlregistration.h>

namespace qor {

// The Library (VISION §6, Phase 1): every game and every piece of media the
// shell can play or open, and what was last played or opened, for Home and
// the Library. Until the QOR core's index (Phase 2) it finds them itself:
//
//   - QQ games QOR OS carries: each folder of QOR_GAMES (paths separated as
//     PATH is), else of `share/qor-os/games` beside Prism's program and in the
//     person's data folder, that has scenes; its scene is
//     `scenes/<folder>.qml`, else its first.
//   - The person's own QQ games: such folders in Documents, two deep.
//   - ARQADE's six games, while ARQADE can be started (`arqade`).
//   - Media: video, music and pictures in the person's own folders, newest
//     first, three deep and at most 400 a folder.
//
// Folders are read on a worker thread (refresh()). What was played or opened
// is kept in the shell's data folder (library-history.json): when, and where
// a film or song stopped, which QOR Media reports (reported()).
//
// An item: { id, kind ("game", "media"), source ("qq", "mine", "arqade",
// "media"), title, subtitle, glyph, hue, path (a scene or a file), game
// (ARQADE's, by kind), mediaKind ("video", "audio", "picture"), modified,
// lastUsed (ms since the epoch, 0 if never), position and duration
// (seconds, 0 if not known) }.
class Library : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList games READ games NOTIFY changed)
    Q_PROPERTY(QVariantList media READ media NOTIFY changed)
    // What was played or opened, the latest first: at most twelve.
    Q_PROPERTY(QVariantList recent READ recent NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    // Whether ARQADE can be started here, so its games are in the Library.
    Q_PROPERTY(bool arqade READ arqade WRITE setArqade NOTIFY changed)

public:
    explicit Library(QObject *parent = nullptr);
    ~Library() override;

    QVariantList games() const;
    QVariantList media() const;
    QVariantList recent() const;
    bool scanning() const { return m_watcher.isRunning(); }
    bool arqade() const { return m_arqade; }
    void setArqade(bool arqade);

    // Reads the folders again, unless they were read in the last ten seconds
    // (`force` reads them anyway).
    Q_INVOKABLE void refresh(bool force = false);
    // The item, by its id, or an empty map.
    Q_INVOKABLE QVariantMap item(const QString &id) const;
    // It was played or opened now.
    Q_INVOKABLE void used(const QString &id);
    // A report of QOR Media's: where a file stopped ("position"), or how long
    // it is ("learned"). Anything else is not the Library's.
    Q_INVOKABLE void reported(const QVariantMap &report);

    // The folders QOR OS's games are found in.
    static QStringList gameFolders();

    struct Found
    {
        QVariantList games;
        QVariantList media;
    };
    // Reads them (the worker's work; for the tests too).
    static Found scan(const QStringList &gameFolders, const QStringList &mineFolders, const QStringList &mediaFolders);

signals:
    void changed();
    void scanningChanged();

private:
    struct Use
    {
        qint64 lastUsed = 0;
        double position = 0;
        double duration = 0;
    };

    QVariantMap withUse(QVariantMap item) const;
    void loadHistory();
    void saveHistory() const;
    QString historyFile() const;
    QVariantList arqadeGames() const;

    Found m_found;
    QHash<QString, Use> m_uses;
    bool m_arqade = false;
    QDateTime m_read;
    QFutureWatcher<Found> m_watcher;
};

} // namespace qor
