/*
 *   Copyright (c) 2023 Anton Kundenko <singaraiona@gmail.com>
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "poll.h"
#include "heap.h"

__thread i32_t __EVENT_FD;  // eventfd to notify epoll loop of shutdown

nil_t sigint_handler(i32_t signo) {
    u64_t val = 1;
    i32_t res;

    UNUSED(signo);

    // Write to the eventfd to wake up the epoll loop.
    res = write(__EVENT_FD, &val, sizeof(val));
    UNUSED(res);
}

poll_p poll_create() {
    i64_t fd;
    poll_p poll;
    struct epoll_event ev;

    fd = epoll_create1(0);
    if (fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Add eventfd
    __EVENT_FD = eventfd(0, 0);
    if (__EVENT_FD == -1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = __EVENT_FD;
    if (epoll_ctl(fd, EPOLL_CTL_ADD, __EVENT_FD, &ev) == -1) {
        perror("epoll_ctl: eventfd");
        exit(EXIT_FAILURE);
    }

    // Set up the SIGINT signal handler
    signal(SIGINT, sigint_handler);

    poll = (poll_p)heap_alloc(sizeof(struct poll_t));
    poll->code = NULL_I64;
    poll->fd = fd;
    poll->term = term_create();
    poll->selectors = freelist_create(128);
    poll->timers = timers_create(16);

    return poll;
}

nil_t poll_destroy(poll_p poll) {
    i64_t i, l;

    // Free all selectors
    l = poll->selectors->data_pos;
    for (i = 0; i < l; i++) {
        if (poll->selectors->data[i] != NULL_I64)
            poll_deregister(poll, i + SELECTOR_ID_OFFSET);
    }

    term_destroy(poll->term);

    freelist_free(poll->selectors);
    timers_destroy(poll->timers);

    close(__EVENT_FD);
    close(poll->fd);
    heap_free(poll);
}

i64_t poll_register(poll_p poll, poll_registry_p registry) {
    i64_t id;
    selector_p selector;
    struct epoll_event ev;

    selector = (selector_p)heap_alloc(sizeof(struct selector_t));
    id = freelist_push(poll->selectors, (i64_t)selector) + SELECTOR_ID_OFFSET;
    selector->id = id;
    selector->fd = registry->fd;
    selector->type = registry->type;
    selector->open_fn = registry->open_fn;
    selector->close_fn = registry->close_fn;
    selector->recv_fn = registry->recv_fn;
    selector->recv_error_fn = registry->recv_error_fn;
    selector->send_fn = registry->send_fn;
    selector->send_error_fn = registry->send_error_fn;
    selector->data = registry->data;
    selector->tx.isset = B8_FALSE;
    selector->rx.buf = NULL;
    selector->rx.size = 0;
    selector->rx.bytes_transfered = 0;
    selector->tx.buf = NULL;
    selector->tx.size = 0;
    selector->tx.bytes_transfered = 0;
    selector->tx.queue = queue_create(TX_QUEUE_SIZE);

    ev.events = registry->events;
    ev.data.ptr = selector;

    if (epoll_ctl(poll->fd, EPOLL_CTL_ADD, selector->fd, &ev) == -1) {
        perror("epoll_ctl: add");
        return -1;
    }

    return id;
}

nil_t poll_deregister(poll_p poll, i64_t id) {
    i64_t idx;
    selector_p selector;

    idx = freelist_pop(poll->selectors, id - SELECTOR_ID_OFFSET);

    if (idx == NULL_I64)
        return;

    selector = (selector_p)idx;

    if (selector->close_fn != NULL)
        selector->close_fn(poll, selector);

    epoll_ctl(poll->fd, EPOLL_CTL_DEL, selector->fd, NULL);
    close(selector->fd);

    heap_free(selector->rx.buf);
    heap_free(selector->tx.buf);
    queue_free(selector->tx.queue);
    heap_free(selector);
}

i64_t poll_run(poll_p poll) {
    i64_t n, nfds, timeout;
    poll_result_t poll_result;
    selector_p selector;
    struct epoll_event ev, events[MAX_EVENTS];

    while (poll->code == NULL_I64) {
        timeout = timer_next_timeout(poll->timers);
        nfds = epoll_wait(poll->fd, events, MAX_EVENTS, timeout);

        if (nfds == -1 && errno == EINTR)
            continue;

        if (nfds == -1)
            return 1;

        for (n = 0; n < nfds; n++) {
            ev = events[n];

            // shutdown
            if (ev.data.fd == __EVENT_FD) {
                poll->code = 0;
                break;
            }

            selector = (selector_p)ev.data.ptr;

            // error or hang up
            if ((ev.events & POLL_EVENT_ERROR) || (ev.events & POLL_EVENT_HUP)) {
                poll_deregister(poll, selector->id);
                continue;
            }

            // read
            if (ev.events & POLL_EVENT_READ && selector->recv_fn != NULL) {
                poll_result = selector->recv_fn(poll, selector);
                if (poll_result == POLL_PENDING)
                    continue;

                if (poll_result == POLL_ERROR) {
                    poll_deregister(poll, selector->id);
                    continue;
                }
            }

            // write
            if (ev.events & POLL_EVENT_WRITE && selector->send_fn != NULL) {
                poll_result = selector->send_fn(poll, selector);

                if (poll_result == POLL_ERROR)
                    poll_deregister(poll, selector->id);
            }
        }
    }

    return poll->code;
}
