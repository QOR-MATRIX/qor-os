// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#include "signature.h"

#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

namespace qor::store {

namespace {

// The DER SubjectPublicKeyInfo of a P-256 key, up to the point itself:
// id-ecPublicKey, prime256v1, then a 66-byte BIT STRING.
constexpr unsigned char kP256InfoPrefix[] = { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48,
                                              0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48,
                                              0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00 };

} // namespace

bool verifySignature(const QByteArray &publicKey, const QByteArray &message, const QByteArray &signature)
{
    if (publicKey.size() != 65 || publicKey.at(0) != 0x04 || signature.size() != 64)
        return false;

    QByteArray info(reinterpret_cast<const char *>(kP256InfoPrefix), sizeof(kP256InfoPrefix));
    info.append(publicKey);
    const auto *cursor = reinterpret_cast<const unsigned char *>(info.constData());
    EVP_PKEY *key = d2i_PUBKEY(nullptr, &cursor, long(info.size()));
    if (!key)
        return false;

    // OpenSSL takes the signature DER-encoded.
    const auto *raw = reinterpret_cast<const unsigned char *>(signature.constData());
    ECDSA_SIG *sig = ECDSA_SIG_new();
    BIGNUM *r = BN_bin2bn(raw, 32, nullptr);
    BIGNUM *s = BN_bin2bn(raw + 32, 32, nullptr);
    unsigned char *der = nullptr;
    int derLength = 0;
    if (sig && r && s && ECDSA_SIG_set0(sig, r, s) == 1) {
        r = s = nullptr; // owned by sig now
        derLength = i2d_ECDSA_SIG(sig, &der);
    }
    BN_free(r);
    BN_free(s);

    bool valid = false;
    EVP_MD_CTX *context = EVP_MD_CTX_new();
    if (context && derLength > 0 && EVP_DigestVerifyInit(context, nullptr, EVP_sha256(), nullptr, key) == 1)
        valid = EVP_DigestVerify(context, der, size_t(derLength),
                                 reinterpret_cast<const unsigned char *>(message.constData()), size_t(message.size()))
                == 1;
    EVP_MD_CTX_free(context);
    OPENSSL_free(der);
    ECDSA_SIG_free(sig);
    EVP_PKEY_free(key);
    return valid;
}

} // namespace qor::store
