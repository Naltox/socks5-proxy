//
// Created by Narek Abovyan on 22/04/2018.
//

#include <stdlib.h>
#include <sys/event.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "io_events.h"

int events_init(io_events* ev_desc) {
    int kq = kqueue();
    assert(kq > 0);
    ev_desc->poll_fd = kq;
    return kq;
}

void events_subscribe(io_events* ev_desc, int fd, void* user_data) {
    struct kevent events[2];
    // READ event
    events[0].data     = 0;
    events[0].fflags   = 0;
    events[0].filter   = EVFILT_READ;
    events[0].flags    = EV_ADD | EV_CLEAR;
    events[0].ident    = (uintptr_t) fd;
    events[0].udata = user_data;

    // WRITE event
    events[1].data     = 0;
    events[1].fflags   = 0;
    events[1].filter   = EVFILT_WRITE;
    events[1].flags    = EV_ADD | EV_CLEAR;
    events[1].ident    = (uintptr_t) fd;
    events[1].udata = user_data;

    assert(kevent(ev_desc->poll_fd, &events[0], 2, NULL, 0, NULL) == 0);
}

void events_wait(io_events* ev_desc, void* noop_user_data) {
    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
    struct kevent events[1024];

    while (1) {
        int events_count = kevent(ev_desc->poll_fd, NULL, 0, events, 1024, &timeout);
        assert(events_count >= 0);

        for (unsigned int i = 0; i < events_count; ++i) {
            struct kevent event = events[i];

            if (event.filter == EVFILT_WRITE) {
                ev_desc->on_write((int) event.data, event.udata);
            }
            if (event.filter == EVFILT_READ) {
                ev_desc->on_read((int) event.data, event.udata);
            }
        }
        ev_desc->noop(noop_user_data);
    }
}

char* events_api_name() {
    return "kqueue";
}