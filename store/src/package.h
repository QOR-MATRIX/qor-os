// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace qtos::store {

// A QTOS package (.qtospkg): a zip archive holding
//   manifest.json  id, kind, version, developer and the SHA-256 of every file
//   manifest.sig   the developer's ECDSA P-256 signature over manifest.json
//   the files the manifest lists, and nothing else.
struct PackageManifest
{
    QString id;
    QString kind;
    QString version;
    QString name;
    QString developer;
    QMap<QString, QByteArray> files; // path → raw SHA-256
};

struct PackageResult
{
    bool ok = false;
    QString error;
    PackageManifest manifest;
    QMap<QString, QByteArray> contents; // path → bytes, all verified
};

// Opens a package and verifies it completely: the developer's signature over
// the manifest, then every file against the manifest. Paths must be plain
// relative names; symbolic links, unlisted or missing files, and more than
// `maximumSize` bytes of content are refused.
PackageResult openPackage(const QByteArray &archive, const QByteArray &developerKey,
                          qint64 maximumSize = 32 * 1024 * 1024);

bool isSafePackagePath(const QString &path);

} // namespace qtos::store
