//
// Created by Narek Abovyan on 22/04/2018.
//

#ifndef C_PLAYGROUND_SOCKS5_H
#define C_PLAYGROUND_SOCKS5_H

#include <stdint.h>
#include "net-connections.h"

#define SOCKS5_VERSION 0x05
#define SOCKS5_NO_AUTH_CODE 0x00
#define SOCKS5_HOST_NOT_FOUND 0x04
#define SOCKS5_COMMAND_NOT_SUPPORTED 0x07
#define SOCKS5_ADDRESS_TYPE_NOT_SUPPORTED 0x08
#define SOCKS5_COMMAND_SUCCEEDED 0x00

#define SOCKS5_COMMAND_CONNECT 0x01
#define SOCKS5_COMMAND_BIND 0x02
#define SOCKS5_COMMAND_UDP_ASSOCIATE 0x03

#define SOCKS5_IPV4_ADDRESS 0x01
#define SOCKS5_DOMAINNAME 0x03
#define SOCKS5_IPV6_ADDRESS 0x04

typedef struct {
    unsigned char version;
    unsigned char auth_count;
    unsigned char auth_codes[256];
} socks5_welcome_message;

socks5_welcome_message* socks5_parse_welcome_message(char* data);
void socks5_free_welcome_message(socks5_welcome_message* msg);

#pragma pack(push,1)
typedef struct {
    unsigned char version;
    unsigned char auth_code;
} socks5_auth_answer;
#pragma pack(pop)

static socks5_auth_answer socks5_free_auth = {
    .version = SOCKS5_VERSION,
    .auth_code = SOCKS5_NO_AUTH_CODE
};

#pragma pack(push,1)
typedef struct {
    unsigned char version;
    unsigned char command_code;
    unsigned char reserved;
    unsigned char address_type;
    union {
        uint32_t ipV4_address;
        struct domain {
            unsigned char len;
            char hostname[256];
        } domain;
    };
    uint16_t port;
} socks5_request;
#pragma pack(pop)

socks5_request* socks5_request_parse(char* data);

void socks5_request_free(socks5_request* req);

void socks5_response_write_to_conn(socks5_request* response, connection* conn);

enum socks5connection_state {
    INITIAL_STATE,
    AUTH_DONE_STATE,
    READY_STATE,
    CLOSED_STATE
};

enum socks5connection_type {
    CLIENT,
    DEST
};

typedef struct {
    enum socks5connection_state state;
    enum socks5connection_type type;
    connection* to_connection;
    socks5_request* req;
} socks5connection;

socks5connection* socks5_connection_new(enum socks5connection_type type);

void socks5_connection_free(socks5connection* conn);

#endif //C_PLAYGROUND_SOCKS5_H
