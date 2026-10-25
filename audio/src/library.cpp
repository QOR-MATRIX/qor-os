// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "library.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QVariantMap>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace qtos::audio {

namespace {

const QStringList &musicFilters()
{
    static const QStringList filters { u"*.mp3"_s, u"*.flac"_s, u"*.ogg"_s, u"*.oga"_s, u"*.opus"_s, u"*.wav"_s,
                                       u"*.m4a"_s, u"*.aac"_s,  u"*.wma"_s, u"*.aif"_s, u"*.aiff"_s, u"*.alac"_s };
    return filters;
}

QVariantMap describe(const QFileInfo &file, const QDir &root)
{
    static const QRegularExpression trackNumber(u"^\\s*\\d{1,3}\\s*[-._)]?\\s+"_s);

    QString title = file.completeBaseName().trimmed();
    QString artist;
    QString album;
    const QStringList folders = root.relativeFilePath(file.absolutePath()).split(u'/', Qt::SkipEmptyParts);
    if (folders.size() >= 2 && folders.first() != u".."_s) {
        artist = folders.at(folders.size() - 2);
        album = folders.last();
    } else if (folders.size() == 1 && folders.first() != u"."_s) {
        album = folders.first();
    }

    title.remove(trackNumber);
    if (const qsizetype dash = title.indexOf(u" - "_s); dash > 0) {
        artist = title.left(dash).trimmed();
        title = title.mid(dash + 3).trimmed();
    }
    title.replace(u'_', u' ');
    if (title.isEmpty())
        title = file.completeBaseName();

    const QString identity = album.isEmpty() ? (artist.isEmpty() ? title : artist) : album;
    return {
        { u"kind"_s, u"file"_s },
        { u"path"_s, file.absoluteFilePath() },
        { u"title"_s, title },
        { u"artist"_s, artist.isEmpty() ? u"Unknown Artist"_s : artist },
        { u"album"_s, album },
        { u"duration"_s, 0.0 },
        { u"hue"_s, double(qHash(identity) % 360) },
    };
}

} // namespace

QStringList libraryRoots()
{
    QStringList roots;
    const QString configured = qEnvironmentVariable("QTOS_MUSIC_DIR");
    for (const QString &path : configured.split(QDir::listSeparator(), Qt::SkipEmptyParts))
        roots.append(QDir::cleanPath(path));
    for (const QString &path : QStandardPaths::standardLocations(QStandardPaths::MusicLocation))
        if (!roots.contains(QDir::cleanPath(path)))
            roots.append(QDir::cleanPath(path));
    return roots;
}

QVariantList scanLibrary(const QStringList &roots, int limit)
{
    QList<std::pair<QString, QVariantMap>> found;
    for (const QString &rootPath : roots) {
        const QDir root(rootPath);
        if (!root.exists())
            continue;
        const int baseDepth = int(QDir::cleanPath(root.absolutePath()).count(u'/'));
        QDirIterator it(root.absolutePath(), musicFilters(), QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext() && found.size() < limit) {
            const QFileInfo file(it.next());
            if (int(file.absolutePath().count(u'/')) - baseDepth > 3)
                continue;
            found.append({ file.absoluteFilePath().toLower(), describe(file, root) });
        }
    }
    std::sort(found.begin(), found.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
    found.erase(
        std::unique(found.begin(), found.end(), [](const auto &a, const auto &b) { return a.first == b.first; }),
        found.end());

    QVariantList tracks;
    for (const auto &entry : found)
        tracks.append(entry.second);
    return tracks;
}

} // namespace qtos::audio
