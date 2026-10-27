// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "signature.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <bcrypt.h>

#include <QtCore/QCryptographicHash>

#include <cstring>

namespace qor::store {

bool verifySignature(const QByteArray &publicKey, const QByteArray &message, const QByteArray &signature)
{
    if (publicKey.size() != 65 || publicKey.at(0) != 0x04 || signature.size() != 64)
        return false;
    const QByteArray digest = QCryptographicHash::hash(message, QCryptographicHash::Sha256);

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0)))
        return false;

    // A CNG public key blob: the header, then X and Y.
    QByteArray blob(qsizetype(sizeof(BCRYPT_ECCKEY_BLOB)) + 64, '\0');
    BCRYPT_ECCKEY_BLOB header { BCRYPT_ECDSA_PUBLIC_P256_MAGIC, 32 };
    std::memcpy(blob.data(), &header, sizeof(header));
    std::memcpy(blob.data() + sizeof(header), publicKey.constData() + 1, 64);

    bool valid = false;
    BCRYPT_KEY_HANDLE key = nullptr;
    if (BCRYPT_SUCCESS(BCryptImportKeyPair(algorithm, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
                                           reinterpret_cast<PUCHAR>(blob.data()), ULONG(blob.size()), 0))) {
        valid = BCRYPT_SUCCESS(BCryptVerifySignature(
            key, nullptr, reinterpret_cast<PUCHAR>(const_cast<char *>(digest.constData())), ULONG(digest.size()),
            reinterpret_cast<PUCHAR>(const_cast<char *>(signature.constData())), ULONG(signature.size()), 0));
        BCryptDestroyKey(key);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return valid;
}

} // namespace qor::store
