// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#include "package.h"

#include "catalog.h"
#include "signature.h"

#include <QtCore/QBuffer>
#include <QtCore/QCryptographicHash>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/private/qzipreader_p.h>

using namespace Qt::StringLiterals;

namespace qtos::store {

namespace {

const QString kManifest = u"manifest.json"_s;
const QString kSignature = u"manifest.sig"_s;

PackageResult failure(const QString &error)
{
    PackageResult result;
    result.error = error;
    return result;
}

} // namespace

bool isSafePackagePath(const QString &path)
{
    static const QRegularExpression pattern(u"^[A-Za-z0-9_][A-Za-z0-9._-]*(/[A-Za-z0-9_][A-Za-z0-9._-]*)*$"_s);
    return path.size() <= 200 && pattern.match(path).hasMatch() && !path.contains(u".."_s);
}

PackageResult openPackage(const QByteArray &archive, const QByteArray &developerKey, qint64 maximumSize)
{
    QBuffer buffer;
    buffer.setData(archive);
    buffer.open(QIODevice::ReadOnly);
    QZipReader zip(&buffer);
    if (zip.status() != QZipReader::NoError || !zip.isReadable())
        return failure(u"The package is not a readable archive."_s);

    QSet<QString> entries;
    qint64 total = 0;
    for (const QZipReader::FileInfo &info : zip.fileInfoList()) {
        if (info.isDir)
            continue;
        if (info.isSymLink || !info.isFile || !isSafePackagePath(info.filePath))
            return failure(u"The package contains an unsafe entry: %1"_s.arg(info.filePath));
        total += info.size;
        if (total > maximumSize)
            return failure(u"The package is too large."_s);
        entries.insert(info.filePath);
    }

    const QByteArray manifestBytes = zip.fileData(kManifest);
    const QByteArray signature = zip.fileData(kSignature);
    if (manifestBytes.isEmpty() || signature.isEmpty())
        return failure(u"The package has no signed manifest."_s);
    if (!verifySignature(developerKey, manifestBytes, signature))
        return failure(u"The package's developer signature does not match."_s);

    const QJsonObject manifest = QJsonDocument::fromJson(manifestBytes).object();
    PackageResult result;
    PackageManifest &m = result.manifest;
    m.id = manifest.value(u"id"_s).toString();
    m.kind = manifest.value(u"kind"_s).toString();
    m.version = manifest.value(u"version"_s).toString();
    m.name = manifest.value(u"name"_s).toString();
    m.developer = manifest.value(u"developer"_s).toString();
    if (manifest.value(u"format"_s).toInt() != 1 || !isValidItemId(m.id) || !isValidVersion(m.version)
        || m.kind.isEmpty() || m.developer.isEmpty())
        return failure(u"The package manifest is not valid."_s);

    const QJsonObject files = manifest.value(u"files"_s).toObject();
    for (auto it = files.begin(); it != files.end(); ++it) {
        const QByteArray digest = QByteArray::fromHex(it.value().toString().toLatin1());
        if (!isSafePackagePath(it.key()) || it.key() == kManifest || it.key() == kSignature || digest.size() != 32)
            return failure(u"The package manifest lists an invalid file: %1"_s.arg(it.key()));
        m.files.insert(it.key(), digest);
    }

    // Exactly the listed files, each matching its digest.
    QSet<QString> expected(m.files.keyBegin(), m.files.keyEnd());
    expected << kManifest << kSignature;
    if (entries != expected)
        return failure(u"The package's files do not match its manifest."_s);
    for (auto it = m.files.cbegin(); it != m.files.cend(); ++it) {
        const QByteArray data = zip.fileData(it.key());
        if (QCryptographicHash::hash(data, QCryptographicHash::Sha256) != it.value())
            return failure(u"%1 does not match the package manifest."_s.arg(it.key()));
        result.contents.insert(it.key(), data);
    }
    result.ok = true;
    return result;
}

} // namespace qtos::store
