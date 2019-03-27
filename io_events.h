//
// Created by Narek Abovyan on 22/04/2018.
//

#ifndef C_PLAYGROUND_EVENTS_H
#define C_PLAYGROUND_EVENTS_H

typedef struct {
    int poll_fd;
    void (*on_read)(int possible_data_count, void* user_data);
    void (*on_write)(int possible_data_count, void* user_data);
    void (*noop)(void* user_data);
} io_events;

int events_init(io_events* ev_desc);

void events_wait(io_events* ev_desc, void* noop_user_data);

void events_subscribe(io_events* ev_desc, int fd, void* user_data);

char* events_api_name();

#endif //C_PLAYGROUND_EVENTS_H
