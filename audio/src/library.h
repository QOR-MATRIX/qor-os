// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QStringList>
#include <QtCore/QVariantList>

namespace qtos::audio {

// The folders music is read from: $QTOS_MUSIC_DIR (any number, separated like
// PATH) and the user's Music folder.
QStringList libraryRoots();

// Music files under `roots` (three levels deep at most), as media queue
// entries: { kind: "file", path, title, artist, album, duration: 0, hue }.
// Titles come from the file layout, "Artist - Title.ext" or
// "Artist/Album/01 Title.ext"; the duration is learnt when a file is played.
QVariantList scanLibrary(const QStringList &roots, int limit = 500);

} // namespace qtos::audio
