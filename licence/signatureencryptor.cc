#include "signatureencryptor.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

SignatureEncryptor::SignatureEncryptor(const QByteArray& key)
{
    if (key.size() < 32) {
        throw std::invalid_argument("AES-256 key must be at least 32 bytes");
    }

    key_ = key.left(32); // truncate or pad to 32 bytes
}

EncryptedData SignatureEncryptor::Encrypt(const QByteArray& plaintext)
{
    const int iv_len { 12 }; // GCM recommended
    const int tag_len { 16 };

    QByteArray iv(iv_len, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), iv_len) != 1) {
        return {};
    }

    QByteArray ciphertext(plaintext.size() + 16, Qt::Uninitialized); // no padding needed in GCM
    QByteArray tag(tag_len, Qt::Uninitialized);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len { 0 };
    int ciphertext_len { 0 };

    EVP_EncryptInit_ex(
        ctx, EVP_aes_256_gcm(), nullptr, reinterpret_cast<const unsigned char*>(key_.constData()), reinterpret_cast<const unsigned char*>(iv.constData()));

    EVP_EncryptUpdate(
        ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len, reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len);
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // get the tag
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_len, tag.data());
    EVP_CIPHER_CTX_free(ctx);

    return { ciphertext, iv, tag };
}

QByteArray SignatureEncryptor::Decrypt(const QByteArray& ciphertext, const QByteArray& iv, const QByteArray& tag)
{
    QByteArray decrypted(ciphertext.size(), Qt::Uninitialized);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len { 0 };
    int decrypted_len { 0 };

    EVP_DecryptInit_ex(
        ctx, EVP_aes_256_gcm(), nullptr, reinterpret_cast<const unsigned char*>(key_.constData()), reinterpret_cast<const unsigned char*>(iv.constData()));

    EVP_DecryptUpdate(
        ctx, reinterpret_cast<unsigned char*>(decrypted.data()), &len, reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size());
    decrypted_len = len;

    // set expected tag
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), const_cast<char*>(tag.constData()));

    // Finalize and check tag
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(decrypted.data()) + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    decrypted_len += len;
    EVP_CIPHER_CTX_free(ctx);
    decrypted.resize(decrypted_len);
    return decrypted;
}
