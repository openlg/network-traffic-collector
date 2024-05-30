//
// Created by Leon on 2024/5/27.
//
#include <string.h>
#include <stdio.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdlib.h>

void compute_hmac_sha1(const char *key, const char *data, unsigned char *hmac_result) {
    unsigned int len = EVP_MAX_MD_SIZE;
    HMAC(EVP_sha1(), key, strlen(key),
         (unsigned char*)data, strlen(data), hmac_result, &len);
}

void base64_encode(unsigned char input[EVP_MAX_MD_SIZE], int length, char *base64text) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);

    //base64text = realloc(base64text, (bufferPtr->length + 1) * sizeof(char));
    memcpy(base64text, bufferPtr->data, bufferPtr->length);
    base64text[bufferPtr->length] = '\0';

    BUF_MEM_free(bufferPtr);
    BIO_free_all(bio);
}

void sign(char *nonce, char *signVersion, char *accessKey, char *secretKey, char *ts, char *body, char *sign_str) {
    char stringToSign[strlen(nonce) + strlen(signVersion) + strlen(accessKey) + strlen(body) + 40];
    sprintf(stringToSign, "POST:accessKey=%s&nonce=%s&signVersion=%s&ts=%s:%s",
            accessKey, nonce, signVersion, ts, body);
    unsigned char *hmac_sha1 = (unsigned char *) malloc(EVP_MAX_MD_SIZE);
    memset(hmac_sha1, 0, EVP_MAX_MD_SIZE);
    compute_hmac_sha1(secretKey, stringToSign, hmac_sha1);
    size_t len = strlen(hmac_sha1);
    base64_encode(hmac_sha1, (int) len, sign_str);
    free(hmac_sha1);
}

