// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "desktopentries.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QStandardPaths>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace qor {

namespace {

struct Entry
{
    QString name;
    QString icon;
    QString wmClass;
    bool valid = false;
};

// A desktop file is a page or two: nothing larger is read, on the shell's thread.
constexpr qint64 kLargestEntry = 64 * 1024;
// So many ids remembered: a client can change its id as often as it likes.
constexpr qsizetype kMostRemembered = 256;
// How long a look through every desktop file for a StartupWMClass is kept.
constexpr qint64 kScanKeptMs = 60 * 1000;

// An app id is whatever a client says it is. Only what could be a desktop file's
// own name is looked for: never a path out of the applications folder.
bool canBeAnId(const QString &appId)
{
    if (appId.isEmpty() || appId.size() > 255 || appId.startsWith(u'.'))
        return false;
    return std::all_of(appId.cbegin(), appId.cend(), [](QChar c) {
        return c.isLetterOrNumber() ? c.unicode() < 128 : c == u'.' || c == u'_' || c == u'-';
    });
}

// Reads the [Desktop Entry] group: Name (localised if available), Icon and
// StartupWMClass. Desktop files are not quite INI files (QSettings would split
// values at commas), so they are parsed directly. Only a plain file of a sensible
// size: a pipe would block the shell for ever, and a device would never end.
Entry readEntry(const QString &path)
{
    Entry entry;
    const QFileInfo info(path);
    if (!info.isFile() || info.size() > kLargestEntry)
        return entry;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return entry;
    const QString language = QLocale().name();                  // e.g. de_DE
    const QString shortLanguage = language.section(u'_', 0, 0); // de
    QString localisedName;
    bool inGroup = false;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(u'[')) {
            inGroup = line == u"[Desktop Entry]"_s;
            continue;
        }
        if (!inGroup || line.startsWith(u'#'))
            continue;
        const qsizetype eq = line.indexOf(u'=');
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == u"Name"_s)
            entry.name = value;
        else if (key == u"Name[%1]"_s.arg(language)
                 || (localisedName.isEmpty() && key == u"Name[%1]"_s.arg(shortLanguage)))
            localisedName = value;
        else if (key == u"Icon"_s)
            entry.icon = value;
        else if (key == u"StartupWMClass"_s)
            entry.wmClass = value;
    }
    if (!localisedName.isEmpty())
        entry.name = localisedName;
    entry.valid = !entry.name.isEmpty();
    return entry;
}

QString resolveIcon(const QString &icon)
{
    if (icon.isEmpty())
        return {};
    if (QFileInfo(icon).isAbsolute())
        return QFileInfo::exists(icon) ? icon : QString();

    QStringList themes;
    if (!QIcon::themeName().isEmpty())
        themes << QIcon::themeName();
    themes << u"hicolor"_s << u"Adwaita"_s;
    static const QStringList sizes { u"scalable"_s, u"512x512"_s, u"256x256"_s, u"128x128"_s,
                                     u"96x96"_s,    u"64x64"_s,   u"48x48"_s };
    static const QStringList extensions { u".svg"_s, u".png"_s };
    const QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dir : dataDirs) {
        for (const QString &theme : themes)
            for (const QString &size : sizes)
                for (const QString &extension : extensions) {
                    const QString candidate = dir + u"/icons/"_s + theme + u'/' + size + u"/apps/"_s + icon + extension;
                    if (QFileInfo::exists(candidate))
                        return candidate;
                }
        for (const QString &extension : extensions) {
            const QString candidate = dir + u"/pixmaps/"_s + icon + extension;
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }
    return {};
}

} // namespace

DesktopEntries::DesktopEntries(QObject *parent) : QObject(parent) { }

QVariantMap DesktopEntries::lookup(const QString &appId)
{
    if (!canBeAnId(appId))
        return { { u"found"_s, false } };
    auto it = m_cache.constFind(appId);
    if (it == m_cache.constEnd()) {
        if (m_cache.size() >= kMostRemembered)
            m_cache.clear();
        it = m_cache.insert(appId, resolve(appId));
    }
    return *it;
}

QStringList DesktopEntries::entriesFor(const QString &wmClass) const
{
    if (!m_scanned.isValid() || m_scanned.elapsed() > kScanKeptMs) {
        m_scanned.start();
        m_byWmClass.clear();
        for (const QString &dir : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
            const QStringList files = QDir(dir).entryList({ u"*.desktop"_s }, QDir::Files);
            for (const QString &file : files) {
                const QString path = dir + u'/' + file;
                const QString found = readEntry(path).wmClass.toLower();
                if (!found.isEmpty())
                    m_byWmClass[found].append(path);
            }
        }
    }
    return m_byWmClass.value(wmClass.toLower());
}

QVariantMap DesktopEntries::resolve(const QString &appId) const
{
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    Entry entry;
    for (const QString &dir : dirs) {
        for (const QString &name : { appId, appId.toLower() }) {
            const QString path = dir + u'/' + name + u".desktop"_s;
            entry = readEntry(path);
            if (entry.valid)
                break;
        }
        if (entry.valid)
            break;
    }
    if (!entry.valid) {
        // Some apps' ids differ from their desktop file names but match StartupWMClass.
        for (const QString &path : entriesFor(appId)) {
            entry = readEntry(path);
            if (entry.valid)
                break;
        }
    }
    if (!entry.valid)
        return { { u"found"_s, false } };
    const QString icon = resolveIcon(entry.icon);
    return {
        { u"found"_s, true },
        { u"name"_s, entry.name },
        { u"icon"_s, icon.isEmpty() ? QUrl() : QUrl::fromLocalFile(icon) },
    };
}

} // namespace qor
