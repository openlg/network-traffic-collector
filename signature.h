//
// Created by Leon on 2024/5/31.
//

#ifndef NETWORK_TRAFFIC_COLLECTOR_SIGNATURE_H
#define NETWORK_TRAFFIC_COLLECTOR_SIGNATURE_H

unsigned char* compute_hmac_sha1(const char *key, const char *data, unsigned int *len) ;
char* base64_encode(unsigned char *input, unsigned int length) ;
char* sign(char *nonce, char *signVersion, char *accessKey, char *secretKey, char *ts, char *body);

#endif //NETWORK_TRAFFIC_COLLECTOR_SIGNATURE_H
