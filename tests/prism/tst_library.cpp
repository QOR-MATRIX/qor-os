// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
//
// The Library (prism/shell/library.h): the QQ games and media it finds in
// folders made here, ARQADE's games while ARQADE can start, what was played
// or opened and in what order, and QOR Media's reports of where a film
// stopped, all kept from one run to the next.

#include "library.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <cmath>

using namespace Qt::StringLiterals;
using qor::Library;

namespace {

void touch(const QString &path, const QDateTime &modified = {})
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();
    if (modified.isValid()) {
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
    }
}

QStringList idsOf(const QVariantList &items)
{
    QStringList ids;
    for (const QVariant &item : items)
        ids.append(item.toMap().value(u"id"_s).toString());
    return ids;
}

QVariantMap byId(const QVariantList &items, const QString &id)
{
    for (const QVariant &item : items)
        if (item.toMap().value(u"id"_s) == id)
            return item.toMap();
    return {};
}

QString historyFile()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"library-history.json"_s);
}

QJsonObject history()
{
    QFile file(historyFile());
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object() : QJsonObject();
}

} // namespace

class tst_Library : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void init() { QFile::remove(historyFile()); }

    // A folder with scenes is a QQ game: its own-named scene, else its first.
    // QOR OS's are one deep; the person's own, in Documents, two deep.
    void qq_games_are_found_by_their_scenes()
    {
        QTemporaryDir games;
        QTemporaryDir documents;
        touch(games.filePath(u"orb-run/scenes/orb-run.qml"_s));
        touch(games.filePath(u"orb-run/scenes/credits.qml"_s));
        touch(games.filePath(u"ember-climb/scenes/level.qml"_s));
        touch(games.filePath(u"notes/readme.txt"_s));
        touch(documents.filePath(u"Projects/my_first_game/scenes/main.qml"_s));
        touch(documents.filePath(u"Deep/a/b/scenes/far.qml"_s));
        // A download unpacked in Documents is nobody's own game: only what QQ Studio made or a person trusted is.
        touch(documents.filePath(u"Downloaded/strangers_game/scenes/main.qml"_s));
        const QString ownList =
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/qor-os/own-projects.json"_s;
        QDir().mkpath(QFileInfo(ownList).absolutePath());
        QFile own(ownList);
        QVERIFY(own.open(QIODevice::WriteOnly));
        own.write(
            QJsonDocument(QJsonArray { QFileInfo(documents.filePath(u"Projects/my_first_game"_s)).canonicalFilePath() })
                .toJson());
        own.close();

        const Library::Found found = Library::scan({ games.path() }, { documents.path() }, {});
        const QStringList ids = idsOf(found.games);
        QCOMPARE(ids.size(), 3);
        const QVariantMap orb = byId(found.games, u"qq:"_s + games.filePath(u"orb-run/scenes/orb-run.qml"_s));
        QCOMPARE(orb.value(u"title"_s).toString(), u"Orb Run"_s);
        QCOMPARE(orb.value(u"source"_s).toString(), u"qq"_s);
        QCOMPARE(orb.value(u"kind"_s).toString(), u"game"_s);
        QVERIFY(!byId(found.games, u"qq:"_s + games.filePath(u"ember-climb/scenes/level.qml"_s)).isEmpty());
        const QVariantMap mine =
            byId(found.games, u"qq:"_s + documents.filePath(u"Projects/my_first_game/scenes/main.qml"_s));
        QCOMPARE(mine.value(u"title"_s).toString(), u"My First Game"_s);
        QCOMPARE(mine.value(u"source"_s).toString(), u"mine"_s);
        QVERIFY(
            byId(found.games, u"qq:"_s + documents.filePath(u"Downloaded/strangers_game/scenes/main.qml"_s)).isEmpty());
        QFile::remove(ownList);
        // The same game twice is one.
        QCOMPARE(Library::scan({ games.path(), games.path() }, {}, {}).games.size(), 2);
    }

    // Video, music and pictures, newest first, three deep at most; nothing
    // else, and nothing hidden.
    void media_is_found_newest_first_and_by_kind()
    {
        QTemporaryDir media;
        const QDateTime now = QDateTime::currentDateTime();
        touch(media.filePath(u"film.mkv"_s), now.addDays(-3));
        touch(media.filePath(u"Albums/song.flac"_s), now.addDays(-2));
        touch(media.filePath(u"photo.JPG"_s), now.addDays(-1));
        touch(media.filePath(u"notes.txt"_s), now);
        touch(media.filePath(u".hidden/secret.mp4"_s), now);
        touch(media.filePath(u"a/b/c/too-deep.mp4"_s), now);

        const Library::Found found = Library::scan({}, {}, { media.path() });
        const QVariantList items = found.media;
        QCOMPARE(idsOf(items), (QStringList { u"media:"_s + media.filePath(u"photo.JPG"_s),
                                              u"media:"_s + media.filePath(u"Albums/song.flac"_s),
                                              u"media:"_s + media.filePath(u"film.mkv"_s) }));
        QCOMPARE(items.at(0).toMap().value(u"mediaKind"_s).toString(), u"picture"_s);
        QCOMPARE(items.at(1).toMap().value(u"mediaKind"_s).toString(), u"audio"_s);
        QCOMPARE(items.at(2).toMap().value(u"mediaKind"_s).toString(), u"video"_s);
        QCOMPARE(items.at(2).toMap().value(u"title"_s).toString(), u"film"_s);
    }

    // ARQADE's seven games, while ARQADE can be started here.
    void arqades_games_are_there_while_it_can_start()
    {
        Library library;
        QVERIFY(idsOf(library.games()).filter(u"arqade:"_s).isEmpty());
        QSignalSpy changed(&library, &Library::changed);
        library.setArqade(true);
        QCOMPARE(changed.size(), 1);
        const QStringList arqade = idsOf(library.games()).filter(u"arqade:"_s);
        QCOMPARE(arqade, (QStringList { u"arqade:void"_s, u"arqade:synapse"_s, u"arqade:orbital"_s, u"arqade:rift"_s,
                                        u"arqade:delve"_s, u"arqade:gloam"_s, u"arqade:flux"_s, u"arqade:reversi"_s }));
        QCOMPARE(library.item(u"arqade:orbital"_s).value(u"game"_s).toString(), u"orbital"_s);
        library.setArqade(false);
        QVERIFY(library.item(u"arqade:orbital"_s).isEmpty());
    }

    // What was played comes first, and is remembered from one run to the
    // next.
    void what_was_played_comes_first_and_is_remembered()
    {
        {
            Library library;
            library.setArqade(true);
            QVERIFY(library.recent().isEmpty());
            library.used(u"arqade:orbital"_s);
            QTest::qWait(5);
            library.used(u"arqade:void"_s);
            QCOMPARE(idsOf(library.recent()), (QStringList { u"arqade:void"_s, u"arqade:orbital"_s }));
            QVERIFY(library.item(u"arqade:void"_s).value(u"lastUsed"_s).toLongLong() > 0);
        }
        Library again;
        again.setArqade(true);
        QCOMPARE(idsOf(again.recent()), (QStringList { u"arqade:void"_s, u"arqade:orbital"_s }));
    }

    // QOR Media's reports: where a film stopped, and how long it is, kept by
    // its path. Anything else, or anything malformed, is not kept.
    void where_a_film_stopped_is_kept()
    {
        Library library;
        const QString path = u"/srv/films/clip.mkv"_s;
        library.reported({ { u"type"_s, u"learned"_s }, { u"path"_s, path }, { u"duration"_s, 100.0 } });
        library.reported({ { u"type"_s, u"position"_s }, { u"path"_s, path }, { u"position"_s, 42.5 } });
        const QJsonObject kept = history().value(u"media:"_s + path).toObject();
        QCOMPARE(kept.value(u"position"_s).toDouble(), 42.5);
        QCOMPARE(kept.value(u"duration"_s).toDouble(), 100.0);
        QVERIFY(kept.value(u"lastUsed"_s).toInteger() > 0);

        const QJsonObject before = history();
        library.reported({ { u"type"_s, u"position"_s }, { u"path"_s, path }, { u"position"_s, -1.0 } });
        library.reported({ { u"type"_s, u"position"_s }, { u"path"_s, path }, { u"position"_s, std::nan("") } });
        library.reported({ { u"type"_s, u"closed"_s }, { u"path"_s, path } });
        library.reported({ { u"type"_s, u"position"_s }, { u"path"_s, QString(5000, u'a') }, { u"position"_s, 1.0 } });
        library.reported({ { u"type"_s, u"position"_s }, { u"position"_s, 1.0 } });
        QCOMPARE(history(), before);
    }

    // refresh() reads QOR_GAMES' folders on a worker thread, then says so.
    void refresh_reads_the_folders_on_a_worker()
    {
        QTemporaryDir games;
        touch(games.filePath(u"orb-run/scenes/orb-run.qml"_s));
        qputenv("QOR_GAMES", games.path().toUtf8());
        QCOMPARE(Library::gameFolders(), QStringList { games.path() });
        Library library;
        QSignalSpy changed(&library, &Library::changed);
        library.refresh();
        QTRY_VERIFY(changed.size() >= 1);
        QVERIFY(!library.scanning());
        QVERIFY(idsOf(library.games()).contains(u"qq:"_s + games.filePath(u"orb-run/scenes/orb-run.qml"_s)));
        // Read again only when forced, within ten seconds.
        library.refresh();
        QVERIFY(!library.scanning());
        library.refresh(true);
        QTRY_VERIFY(!library.scanning());
        qunsetenv("QOR_GAMES");
    }
};

QTEST_GUILESS_MAIN(tst_Library)
#include "tst_library.moc"
