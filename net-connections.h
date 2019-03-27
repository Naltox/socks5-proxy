//
// Created by Narek Abovyan on 13/05/2018.
//

#ifndef C_PLAYGROUND_NET_CONNECTIONS_H
#define C_PLAYGROUND_NET_CONNECTIONS_H

#include <stdint.h>
#include "buffer.h"
#include "io_events.h"
#include "buffer_chain.h"

enum connection_state {
    WAITING_CONNECT,
    CONNECTED,
    CLOSED
};

enum connection_type {
    OUTGOING,
    INCOMING
};

typedef struct connection connection;

typedef struct conn_functions conn_functions;

//
//  vector<connection*>
//
#define V_TYPE connection*
#define V_NAME vec_conn
#include "vector.h"
#undef V_TYPE
#undef V_NAME
//
//
//

struct conn_functions {
    int port;                       // port
    int sfd;                        // server socket
    vec_conn* connections_to_close; // connections to close, we close them after loop

    io_events* events;

    void (*on_connect)(connection* c);      // called after accept, when connection established
    void (*on_disconnect)(connection* c);   // called before close() and before freeing connection
    void (*accept)(connection* c);          // accept function
    int (*ready_to_handle)(connection* c);  // whether or not we ready to handle data, 1 == ready
    int (*on_data)(connection* c);          // called if ready_to_handle returned 1, in buff is cleaned after this call
    void (*noop)(void);                     // noop
};

struct connection {
    int fd;

    buffer* in;
//    buffer* out;

    buff_chain* out_head;
    buff_chain* out_tail;

    conn_functions* conn_functions;

    void* udata;

    enum connection_state state;
    enum connection_type type;
};

void connections_init(conn_functions* connections);

void connections_start(conn_functions* connections);

connection* connection_new(conn_functions* connections, int fd);

void connection_free(connection* conn);

int server_socket(int port);

void conn_write(connection* conn, char* data, int size);

connection* connect_to(conn_functions* connections, uint32_t addr, uint16_t port);

//
// schedules connection close, connection will be closed on next event loop tick
//
void connection_close(connection* conn);

//
// closes connection immediately (be careful, conn* will be freed
//
void connection_close_immediately(connection* conn);

void connection_try_handle(connection* conn);

#endif //C_PLAYGROUND_NET_CONNECTIONS_H
