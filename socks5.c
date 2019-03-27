//
// Created by Narek Abovyan on 22/04/2018.
//

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <printf.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "socks5.h"
#include "buff_utils.h"

socks5_welcome_message* socks5_parse_welcome_message(char* data) {
    socks5_welcome_message*  message = malloc(sizeof(message));

    READ_CHAR(data, message->version);
    READ_CHAR(data, message->auth_count);

    for (int i = 0; i < message->auth_count; ++i) {
        READ_CHAR(data, message->auth_codes[i]);
    }

    return message;
}

void socks5_free_welcome_message(socks5_welcome_message* msg) {
    free(msg);
}

socks5_request* socks5_request_parse(char* data) {
    socks5_request* request = malloc(sizeof(socks5_request));

    READ_CHAR(data, request->version);
    READ_CHAR(data, request->command_code);
    READ_CHAR(data, request->reserved);
    READ_CHAR(data, request->address_type);

    if (request->address_type == 0x03) {
        READ_CHAR(data, request->domain.len);
        request->domain.hostname[request->domain.len] = '\0';
        memcpy(request->domain.hostname, data, request->domain.len);
        data += request->domain.len;
    } else if (request->address_type == 0x01) {
        uint32_t ip;
        READ_INT(data, ip);
        ip = ntohl(ip);
        request->ipV4_address = ip;
    }
    uint16_t port;
    READ_INT_16(data, port);
    port = ntohs(port);

    request->port = port;

    return request;
}

void socks5_request_free(socks5_request* req) {
    free(req);
}

void socks5_response_write_to_conn(socks5_request* response, connection* conn) {
    char response_data[sizeof(socks5_request)];

    char* data = response_data;

    WRITE_CHAR(data, response->version);
    WRITE_CHAR(data, response->command_code);
    WRITE_CHAR(data, response->reserved);
    WRITE_CHAR(data, response->address_type);

    if (response->address_type == 0x03) {
        WRITE_CHAR(data, response->domain.len);
        memcpy(data, response->domain.hostname, response->domain.len);
        data += response->domain.len;
    }
    else if (response->address_type == 0x01) {
        WRITE_UINT(data, htonl(response->ipV4_address));
    }

    WRITE_UINT_16(data, (htons(response->port)));

    printf("socks5_response_write %d\n", data - response_data);

    conn_write(conn, response_data, data - response_data);
}

socks5connection* socks5_connection_new(enum socks5connection_type type) {
    socks5connection* connection = malloc(sizeof(socks5connection));
    connection->state = INITIAL_STATE;
    connection->type = type;
    connection->to_connection = NULL;
    connection->req= NULL;
    return connection;
}

void socks5_connection_free(socks5connection* conn) {
    if (conn->req != NULL) {
        socks5_request_free(conn->req);
    }
    free(conn);
}