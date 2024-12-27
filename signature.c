//
// Created by Leon on 2024/5/27.
//
#include <string.h>
#include <stdio.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdlib.h>
#include "signature.h"

unsigned char* compute_hmac_sha1(const char *key, const char *data, unsigned int *len) {
    unsigned char *hmac_result = (unsigned char*) malloc(EVP_MAX_MD_SIZE);
    memset(hmac_result, 0, EVP_MAX_MD_SIZE);
    HMAC(EVP_sha1(), key, strlen(key),
         (unsigned char*)data, strlen(data), hmac_result, len);
    return hmac_result;
}

char* base64_encode(unsigned char *input, unsigned int length) {
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

    char *base64text = (char *) malloc((bufferPtr->length + 1) * sizeof(char));
    memcpy(base64text, bufferPtr->data, bufferPtr->length);
    base64text[bufferPtr->length] = '\0';

    BUF_MEM_free(bufferPtr);
    BIO_free_all(bio);
    return base64text;
}

char* sign(char *nonce, char *signVersion, char *accessKey, char *secretKey, char *ts, char *body) {
    char stringToSign[strlen(nonce) + strlen(signVersion) + strlen(accessKey) + strlen(body) + 100];
    sprintf(stringToSign, "POST:accessKey=%s&nonce=%s&signVersion=%s&ts=%s:%s",
            accessKey, nonce, signVersion, ts, body);

    unsigned int hmac_sha1_len = 0;
    unsigned char *hmac_sha1 = compute_hmac_sha1(secretKey, stringToSign, &hmac_sha1_len);

    char *base64 = base64_encode(hmac_sha1, hmac_sha1_len);

    free(hmac_sha1);
    return base64;
}

