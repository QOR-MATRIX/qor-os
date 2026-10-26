// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <QtCore/QByteArray>

namespace qtos::store {

// Verifies an ECDSA P-256 signature over SHA-256 of `message`.
//
// `publicKey` is the uncompressed point (65 bytes: 0x04, X, Y) and
// `signature` is r and s (64 bytes, as IEEE P1363 orders them). The work is
// done by the platform's vetted crypto: CNG on Windows, OpenSSL elsewhere.
bool verifySignature(const QByteArray &publicKey, const QByteArray &message, const QByteArray &signature);

} // namespace qtos::store
