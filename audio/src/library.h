// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QStringList>
#include <QtCore/QVariantList>

namespace qor::audio {

// The folders music is read from: $QOR_MUSIC_DIR (any number, separated like
// PATH) and the user's Music folder.
QStringList libraryRoots();

// Music files under `roots` (three levels deep at most), as media queue
// entries: { kind: "file", path, title, artist, album, duration: 0, hue }.
// Titles come from the file layout, "Artist - Title.ext" or
// "Artist/Album/01 Title.ext"; the duration is learnt when a file is played.
QVariantList scanLibrary(const QStringList &roots, int limit = 500);

} // namespace qor::audio
