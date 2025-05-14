#include "signatureencryptor.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

SignatureEncryptor::SignatureEncryptor(const QByteArray& key)
    : key_(key.left(32))
{
}

QByteArray SignatureEncryptor::Encrypt(const QByteArray& plaintext, QByteArray& out_iv)
{
    const int iv_length { 16 };
    out_iv.resize(iv_length);
    RAND_bytes(reinterpret_cast<unsigned char*>(out_iv.data()), iv_length);

    QByteArray ciphertext {};
    ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len { 0 };
    int ciphertext_len { 0 };

    EVP_EncryptInit_ex(
        ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key_.constData()), reinterpret_cast<const unsigned char*>(out_iv.constData()));

    EVP_EncryptUpdate(
        ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len, reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

QByteArray SignatureEncryptor::Decrypt(const QByteArray& ciphertext, const QByteArray& iv)
{
    QByteArray decrypted {};
    decrypted.resize(ciphertext.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len { 0 };
    int decrypted_len { 0 };

    EVP_DecryptInit_ex(
        ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key_.constData()), reinterpret_cast<const unsigned char*>(iv.constData()));

    EVP_DecryptUpdate(
        ctx, reinterpret_cast<unsigned char*>(decrypted.data()), &len, reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size());
    decrypted_len = len;

    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(decrypted.data()) + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    decrypted_len += len;
    EVP_CIPHER_CTX_free(ctx);
    decrypted.resize(decrypted_len);
    return decrypted;
}
