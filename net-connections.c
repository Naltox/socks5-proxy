//
// Created by Narek Abovyan on 13/05/2018.
//

#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "net-connections.h"
#define V_TYPE connection*
#define V_NAME vec_conn
#include "vector.c"
#undef V_TYPE
#undef V_NAME

void ev_on_end(int fd, void* user_data);
void ev_on_write(int possible_data_count, void* user_data);
void ev_on_read(int possible_data_count, void* user_data);
void ev_noop(void* user_data);
void on_connect_noop(connection* c);
void on_disconnect_noop(connection* c);
void conn_try_flush_out(connection* conn);
void connection_set_connected(connection* conn);
void close_connections(conn_functions* connections);

void accept_connection(connection* connection);

int connects_count = 0;

void connections_init(conn_functions* connections) {
    if (!connections->port) {
        connections->port = 8000;
    }

    if (!connections->events) {
        connections->events = malloc(sizeof(io_events));
        events_init(connections->events);

        connections->events->on_write = ev_on_write;
        connections->events->on_read = ev_on_read;
        connections->events->noop = ev_noop;
    }

    if (!connections->sfd) {
        connections->sfd = server_socket(connections->port);
        connection* main_conn = connection_new(connections, connections->sfd);
        connection_set_connected(main_conn);
        events_subscribe(connections->events, connections->sfd, main_conn);
    }


    if (!connections->accept) {
        connections->accept = accept_connection;
    }

    if (!connections->on_connect) {
        connections->on_connect = on_connect_noop;
    }

    if (!connections->on_disconnect) {
        connections->on_disconnect = on_disconnect_noop;
    }

    if (!connections->connections_to_close) {
        connections->connections_to_close = malloc(sizeof(vec_conn));
        vec_conn_init(connections->connections_to_close);
    }
}

void connections_start(conn_functions* connections) {
    events_wait(connections->events, connections);
}

connection* connection_new(conn_functions* connections, int fd) {
    connection* conn = malloc(sizeof(connection));

    conn->fd = fd;
    conn->in = buffer_new();

    conn->out_head = conn->out_tail = buff_chain_new(1l << 12);

    conn->conn_functions = connections;

    return conn;
}

void connection_free(connection* conn) {
    buffer_free(conn->in);
    buff_chain_free(conn->out_head);
    free(conn);
}

#define MAX_UDP_SENDBUF_SIZE	(1L << 24)

void maximize_sndbuf (int sfd, int max) {
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, avg;
    int old_size;

    if (max <= 0) {
        max = MAX_UDP_SENDBUF_SIZE;
    }

    /* Start with the default size. */
    if (getsockopt (sfd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize)) {
        //if (verbosity > 0) {
        perror ("getsockopt (SO_SNDBUF)");
        //}
        return;
    }

    /* Binary-search for the real maximum. */
    min = last_good = old_size;
    max = MAX_UDP_SENDBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int) min + max) / 2;
        if (setsockopt (sfd, SOL_SOCKET, SO_SNDBUF, &avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }

    printf("<%d send buffer was %d, now %d\n", sfd, old_size, last_good);
}

#define MAX_UDP_RCVBUF_SIZE	(1L << 24)

void maximize_rcvbuf (int sfd, int max) {
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, avg;
    int old_size;

    if (max <= 0) {
        max = MAX_UDP_RCVBUF_SIZE;
    }

    /* Start with the default size. */
    if (getsockopt (sfd, SOL_SOCKET, SO_RCVBUF, &old_size, &intsize)) {
        perror ("getsockopt (SO_RCVBUF)");
        return;
    }

    /* Binary-search for the real maximum. */
    min = last_good = old_size;
    max = MAX_UDP_RCVBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int) min + max) / 2;
        if (setsockopt (sfd, SOL_SOCKET, SO_RCVBUF, &avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }
    printf (">%d receive buffer was %d, now %d\n", sfd, old_size, last_good);
}

int server_socket(int port) {
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    maximize_sndbuf(sock, 0);

    fcntl(sock, F_SETFL, O_NONBLOCK);

    if (sock < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    int optval = 1;
    setsockopt(
        sock,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const void *) &optval,
        sizeof(int)
    );

    setsockopt (sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof (optval));
    setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (optval));

    struct sockaddr_in addr;

    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;    // Internet
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    //if (listen(sock, SOMAXCONN) < 0) {
    if (listen(sock, 1024) < 0) {
        perror("ERROR on listen");
        exit(1);
    }

    return sock;
}

void ev_on_end(int fd, void* user_data) {
    connection* conn = user_data;

    if (conn->state == CLOSED) {
        return;
    }

    conn->conn_functions->on_disconnect(conn);
    connection_close(conn);
}

void ev_on_read(int possible_data_count, void* user_data) {
    connection* conn = user_data;

    if (conn->state != CONNECTED) {
        return;
    }

    assert(conn->state == CONNECTED);

    if (conn->fd == conn->conn_functions->sfd) {
        conn->conn_functions->accept(conn);
        return;
    }

    printf("bytes %d are available to read on fd: %d\n", possible_data_count, conn->fd);


    int continue_reading = 1;
    int read_size = 0;

    while (continue_reading) {
        buffer_need_bytes(conn->in, 1l << 12);

        int read_len = (int) read(conn->fd, buffer_cur_data(conn->in), (size_t) 1l << 12);

        if (read_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue_reading = 0;
                printf("EAGAIN\n");
                continue;
            }
            perror("read()");
            ev_on_end(conn->fd, user_data);
            break;
        }

        if (read_len == 0) {
            ev_on_end(conn->fd, user_data);
            break;
        }

        buffer_add_used(conn->in, read_len);

        printf("read_len: %d\n", read_len);
        printf("in buff: %d\n", conn->in->used);
        read_size += read_len;
    }
    if (read_size > 0 && conn->conn_functions->ready_to_handle(conn) == 1) {
        int clean = conn->conn_functions->on_data(conn);

        if (clean == 1)
            buffer_clean(conn->in);
    }
}

void connection_try_handle(connection* conn) {
    printf("connection_try_handle used: %d\n", conn->in->used);
    if (conn->in->used == 0)
        return;

    if (conn->conn_functions->ready_to_handle(conn) == 1) {
        int clean = conn->conn_functions->on_data(conn);
        if (clean == 1)
            buffer_clean(conn->in);
    }
}

void ev_on_write(int possible_data_count, void* user_data) {
    connection* conn = user_data;

    if (conn->state == WAITING_CONNECT) {
        connection_set_connected(conn);
        conn->conn_functions->on_connect(conn);
    }

    if (conn->state == CONNECTED) {
        conn_try_flush_out(conn);
    }
}

void accept_connection(struct connection* connection) {
    int try_accept = 1;

    while (try_accept) {
        int cfd = accept(connection->conn_functions->sfd, NULL, NULL);

        // In this case client slosed connection before we accepted if
        if (cfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                try_accept = 0;
                continue;
            }
            return;
        }

        connects_count++;

        /**
         * On Linux, the new socket returned by accept() does not inherit file
           status flags such as O_NONBLOCK and O_ASYNC from the listening
           socket.  This behavior differs from the canonical BSD sockets
           implementation.  Portable programs should not rely on inheritance or
           noninheritance of file status flags and always explicitly set all
           required flags on the socket returned from accept().

           what about buffer sizes?
         */

        fcntl(cfd, F_SETFL, O_NONBLOCK);
        int optval = 1;
        setsockopt (cfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof (int));

        struct sockaddr_in addr;

        socklen_t addr_size = sizeof(struct sockaddr_in);
        int res = getpeername(cfd, (struct sockaddr *)&addr, &addr_size);

        printf("port is: %d\n", (int) ntohs(addr.sin_port));
        printf("ip is:  %s\n",  inet_ntoa(addr.sin_addr));
        printf("client sock is:  %d\n",  cfd);

        maximize_rcvbuf(cfd, 0);
        maximize_sndbuf(cfd, 0);

        struct connection* conn = connection_new(connection->conn_functions, cfd);
        connection_set_connected(conn);
        conn->type = INCOMING;

        events_subscribe(connection->conn_functions->events, cfd, conn);

        connection->conn_functions->on_connect(conn);
    }
}

void conn_write(connection* conn, char* data, int size) {
    if (conn->state != CONNECTED) {
        printf("Attempt to write to closed conn\n");
    }
    conn->out_tail = buff_chain_write(conn->out_tail, data, size);
    conn_try_flush_out(conn);
}

void conn_try_flush_out(connection* conn) {
    while((conn->out_head->wptr - conn->out_head->rptr) > 0) {
        ssize_t r = write(conn->fd, conn->out_head->rptr, conn->out_head->wptr - conn->out_head->rptr);
        printf("try: %d, write count: %zd\n", conn->out_head->wptr - conn->out_head->rptr, r);
        if (r > 0) {
            conn->out_head->rptr += r;

            if (conn->out_head->rptr == conn->out_head->wptr) {
                buff_chain *b = conn->out_head;
                conn->out_head = b->next;
                if (conn->out_head == NULL) {
                    conn->out_head = conn->out_tail = buff_chain_new(1l << 12);
                }
                buff_chain_free_block(b);
            }
        } else if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                perror("write()");
                conn->conn_functions->on_disconnect(conn);
                connection_close(conn);
                return;
            }
        }
    }
}

void ev_noop(void* user_data) {
    conn_functions* connections = user_data;

    close_connections(connections);

    if (connections->noop) {
        connections->noop();
    }

    printf("opened connects: %d\n", connects_count);
}

void on_connect_noop(connection* c) {

}

void on_disconnect_noop(connection* c) {

}

void print_ip(int ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
}

connection* connect_to(conn_functions* connections, uint32_t addr, uint16_t port) {
    connects_count++;
    printf("%zu\n", addr);
    printf("port: %d, ip: ", port);
    print_ip(addr);
    printf("\n");

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    struct in_addr ip;
    ip.s_addr = htonl(addr);

    server_address.sin_port = htons(port);
    server_address.sin_addr = ip;

    // open a stream socket
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("could not create socket");
        return NULL;
    }

    // set socket non-blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // TCP is connection oriented, a reliable connection
    // **must** be established before any data is exchanged
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        if (errno == EINPROGRESS) {
            // need to wait for connection
            printf("EINPROGRESS\n");

            connection* conn = connection_new(connections, sock);
            conn->state = WAITING_CONNECT;
            conn->type = OUTGOING;

            events_subscribe(connections->events, sock, conn);
            maximize_rcvbuf(sock, 0);
            maximize_sndbuf(sock, 0);

            return conn;
        } else {
            perror("could not connect to server\n");
        }

        return NULL;
    }

    // connection established

    printf("connected immediately\n");

    connection* conn = connection_new(connections, sock);
    connection_set_connected(conn);
    conn->type = OUTGOING;
    events_subscribe(connections->events, sock, conn);
    maximize_rcvbuf(sock, 0);
    maximize_sndbuf(sock, 0);

    return conn;
}

void connection_set_connected(connection* conn) {
    conn->state = CONNECTED;
}

void connection_close(connection* conn) {
    if (conn->state == CLOSED) {
        return;
    }

    conn->state = CLOSED;
    vec_conn_push(conn->conn_functions->connections_to_close, conn);
}

void connection_close_immediately(connection* conn) {
    close(conn->fd);
    connects_count--;
    connection_free(conn);
}

//
// closes connections scheduled to close
//
void close_connections(conn_functions* connections) {
    size_t conns_to_close_num = vec_conn_length(connections->connections_to_close);

    if (conns_to_close_num > 0) {
        printf("connections to close: %zd\n", conns_to_close_num);

        for (int i = 0; i < conns_to_close_num; i++) {
            connection* conn = vec_conn_get(connections->connections_to_close, i);

            printf("close: %d\n", conn->fd);

            connection_close_immediately(conn);
        }
    }

    vec_conn_clean(connections->connections_to_close);
}