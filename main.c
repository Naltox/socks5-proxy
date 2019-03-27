//
// Created by Narek Abovyan on 16/05/2018.
//

#include <printf.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "net-connections.h"
#include "buff_utils.h"
#include "socks5.h"
#include "buffer_chain.h"

int is_ready_to_handle(connection *conn);

int on_conn_data(connection *c);

void on_connect(connection *c);

void on_disconnect(connection *c);

void noop();

int socks5_welcome_try_parse(buffer *buff);

int socks5_request_try_parse(buffer *buff);

conn_functions serv_funcs = {
    .port = 8000,
    .ready_to_handle = is_ready_to_handle,
    .on_data = on_conn_data,
    .on_connect = on_connect,
    .on_disconnect = on_disconnect,
    .noop = noop
};

int clients = 0;
int dests = 0;

int main() {
    signal(SIGPIPE, SIG_IGN);
    connections_init(&serv_funcs);
    printf("events API: %s\n", events_api_name());
    printf("server sock: %d, port: %d\n", serv_funcs.sfd, serv_funcs.port);
    connections_start(&serv_funcs);
    return 0;
}

void decr_counters(socks5connection *sc) {
    if (sc->type == CLIENT) {
        clients--;
    }
    if (sc->type == DEST) {
        dests--;
    }
}

void on_disconnect(connection *c) {
    socks5connection *sc = c->udata;
    decr_counters(sc);

    if (sc->type == CLIENT) {
        printf("client: %d leaved\n", c->fd);
    } else if (sc->type == DEST) {
        printf("dest: %d leaved\n", c->fd);
    }

    if (sc->to_connection) {
        socks5connection* to_conn = sc->to_connection->udata;
        to_conn->to_connection = NULL;
        socks5_connection_free(to_conn);

        if (sc->to_connection->state == CONNECTED || sc->to_connection->state == WAITING_CONNECT) {
            decr_counters(to_conn);
            connection_close(sc->to_connection);
            printf("close too: %d, state: %d\n", sc->to_connection->fd, sc->to_connection->state);
        }
    }

    socks5_connection_free(sc);
}

int is_ready_to_handle(connection *c) {
    socks5connection *sc = c->udata;

    if (c->type == OUTGOING) {
        return 1;
    } else if (sc->state == INITIAL_STATE) {
        return socks5_welcome_try_parse(c->in);
    } else if (sc->state == AUTH_DONE_STATE) {
        return socks5_request_try_parse(c->in);
    }

    return 1;
}

int on_conn_data(connection *c) {
    socks5connection *sc = c->udata;

    if (sc->state == READY_STATE) {
        if (sc->type == CLIENT) {
            if (sc->to_connection == NULL) {
                printf("client writes before connecting to destination\n");
                socks5_connection_free(sc);
                decr_counters(sc);
                connection_close(c);
                return 1;
            }
            printf("client{%d} -> dest{%d}\n", c->fd, sc->to_connection->fd);
            conn_write(sc->to_connection, c->in->data, (int) c->in->used);
        } else if (sc->type == DEST) {
            if (sc->to_connection == NULL) {
                printf("destination writes to unknown client\n");
                socks5_connection_free(sc);
                decr_counters(sc);
                connection_close(c);
                return 1;
            }
            printf("dest{%d} -> client{%d}\n", c->fd, sc->to_connection->fd);
            conn_write(sc->to_connection, c->in->data, (int) c->in->used);
        }
        return 1;
    } else if (sc->state == INITIAL_STATE) {
        socks5_welcome_message *msg = socks5_parse_welcome_message(c->in->data);
        conn_write(c, (char *) &socks5_free_auth, sizeof(socks5_auth_answer));
        sc->state = AUTH_DONE_STATE;
        socks5_free_welcome_message(msg);
        return 1;
    } else if (sc->state == AUTH_DONE_STATE) {
        socks5_request *req = socks5_request_parse(c->in->data);

        if (req->command_code != SOCKS5_COMMAND_CONNECT) {
            printf("Client tries to make non-tcp connection\n");
            req->command_code = SOCKS5_COMMAND_NOT_SUPPORTED;
            socks5_response_write_to_conn(req, c);
            socks5_request_free(req);
            socks5_connection_free(sc);
            decr_counters(sc);
            connection_close(c);
            return 1;
        }

        if (req->address_type == SOCKS5_IPV4_ADDRESS) {
            connection *dc = connect_to(c->conn_functions, req->ipV4_address, req->port);
            if (dc == NULL) {
                perror("connect_to()");
                return 1;
            }

            socks5connection *dsc = socks5_connection_new(DEST);
            dsc->state = READY_STATE;
            dsc->to_connection = c;
            dsc->req = req;

            dc->udata = dsc;
            sc->to_connection = dc;

            return 1;
        } else if (req->address_type == SOCKS5_DOMAINNAME) {
            printf("connect domain: %s fd: %d\n", req->domain.hostname, c->fd);

            struct addrinfo *res = NULL;
            int resolve_res = getaddrinfo(req->domain.hostname, NULL, NULL, &res);

            if (resolve_res != 0 || res->ai_family != AF_INET) {
                req->command_code = SOCKS5_HOST_NOT_FOUND;
                socks5_response_write_to_conn(req, c);
                printf("HOST NOT FOUND\n");
                freeaddrinfo(res);
                return 1;
            }

            struct sockaddr_in *ipv4 = (struct sockaddr_in *) res->ai_addr;
            uint32_t ip = ntohl(ipv4->sin_addr.s_addr);

            connection *dc = connect_to(c->conn_functions, ip, req->port);

            socks5connection *dsc = socks5_connection_new(DEST);
            dsc->state = READY_STATE;
            dsc->to_connection = c;
            dsc->req = req;

            dc->udata = dsc;
            sc->to_connection = dc;

            freeaddrinfo(res);
            return 1;
        } else {
            printf("Client tries to make ipv6 connection\n");
            req->command_code = SOCKS5_ADDRESS_TYPE_NOT_SUPPORTED;
            socks5_response_write_to_conn(req, c);
            socks5_request_free(req);
            socks5_connection_free(sc);
            decr_counters(sc);
            connection_close(c);
            return 1;
        }
    }

    return 1;
}

void on_connect(connection *c) {
    if (c->type == INCOMING) {
        clients++;
        socks5connection *sc = socks5_connection_new(CLIENT);
        c->udata = sc;
    } else if (c->type == OUTGOING) {
        dests++;
        socks5connection *dsc = c->udata;
        dsc->req->command_code = SOCKS5_COMMAND_SUCCEEDED;
        socks5_response_write_to_conn(dsc->req, dsc->to_connection);
        socks5connection *client_connection = dsc->to_connection->udata;
        client_connection->state = READY_STATE;
    }
}

int socks5_welcome_try_parse(buffer *buff) {
    char *data = buff->data;

    if (buff->used < 3) {
        return -1;
    }

    int version;
    int auth_count;

    READ_CHAR(data, version);
    READ_CHAR(data, auth_count);

    if ((buff->used - 2) < auth_count) {
        return -1;
    }

    return 1;
}

int socks5_request_try_parse(buffer *buff) {
    // TODO: do real parsing
    return 1;
}

void noop() {
    printf("clients: %d, dests: %d\n", clients, dests);
}