//
// Created by Narek Abovyan on 23/04/2018.
//

#include <stdlib.h>
#include <sys/epoll.h>
#include <assert.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include "io_events.h"

int events_init(io_events* ev_desc) {
    int queue = epoll_create1(0);
    assert(queue > 0);
    ev_desc->poll_fd = queue;
    return queue;
}

void events_subscribe(io_events* ev_desc, int fd, void* user_data) {
    struct epoll_event event;

    event.data.ptr = user_data;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    assert(epoll_ctl(ev_desc->poll_fd, EPOLL_CTL_ADD, fd, &event) == 0);
}

void events_wait(io_events* ev_desc, void* noop_user_data) {
    struct epoll_event events[1024];
    int nevents;

    while (1) {
        fflush(stdout);

        nevents = epoll_wait (ev_desc->poll_fd, events, 1024, 1000);

        fprintf(stderr, "Got %u events to handle...\n", nevents);

        for (unsigned int i = 0; i < nevents; ++i) {
            struct epoll_event event = events[i];

            if (event.events & EPOLLIN) {
                ev_desc->on_read((1L << 20), event.data.ptr);
            }
            if (event.events & EPOLLOUT) {
                ev_desc->on_write((1L << 20), event.data.ptr);
            }
        }

        ev_desc->noop(noop_user_data);
    }
}

char* events_api_name() {
    return "epoll";
}