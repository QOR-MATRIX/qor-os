// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace qor::store {

// A QOR OS package (.qorpkg): a zip archive holding
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
    QByteArray manifestBytes;           // the manifest and its signature, kept with what is installed
    QByteArray signature;
};

// Opens a package and verifies it completely: the developer's signature over
// the manifest, then every file against the manifest. Paths must be plain
// relative names; symbolic links, unlisted or missing files, and more than
// `maximumSize` bytes of content are refused.
PackageResult openPackage(const QByteArray &archive, const QByteArray &developerKey,
                          qint64 maximumSize = 32 * 1024 * 1024);

// Whether a package's files, installed in `folder`, are still exactly what its developer signed: the signature kept
// beside them over the manifest, every listed file against it, and no other file. An install is verified once; what
// runs is verified each time it is loaded, since anything running as this user can write there.
bool verifyInstalled(const QString &folder, const QByteArray &developerKey);

bool isSafePackagePath(const QString &path);

} // namespace qor::store
