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

#ifndef POLL_H
#define POLL_H

#include <sys/epoll.h>
#include "rayforce.h"
#include "parse.h"
#include "serde.h"
#include "format.h"
#include "queue.h"
#include "freelist.h"
#include "chrono.h"
#include "term.h"

#define MAX_EVENTS 1024
#define BUF_SIZE 2048
#define TX_QUEUE_SIZE 16
#define SELECTOR_ID_OFFSET 3  // shifts all selector ids by 2 to avoid 0, 1 ,2 ids (stdin, stdout, stderr)

// forward declarations
struct poll_t;
struct selector_t;

typedef enum poll_result_t {
    POLL_READY = 0,
    POLL_PENDING = 1,
    POLL_ERROR = 2,
    POLL_EXIT = 3,
} poll_result_t;

typedef enum selector_type_t {
    SELECTOR_TYPE_STDIN = 0,
    SELECTOR_TYPE_STDOUT = 1,
    SELECTOR_TYPE_STDERR = 2,
    SELECTOR_TYPE_SOCKET = 3,
    SELECTOR_TYPE_FILE = 4,
} selector_type_t;

typedef poll_result_t (*poll_event_fn)(struct poll_t *, struct selector_t *);

#if defined(OS_WINDOWS)

typedef struct selector_t {
    i64_t fd;  // socket fd
    i64_t id;  // selector id
    u8_t version;

    struct {
        b8_t ignore;
        u8_t msgtype;
        b8_t header;
        OVERLAPPED overlapped;
        DWORD flags;
        DWORD bytes_transfered;
        i64_t size;
        u8_t *buf;
        WSABUF wsa_buf;
    } rx;

    struct {
        b8_t ignore;
        OVERLAPPED overlapped;
        DWORD flags;
        DWORD bytes_transfered;
        i64_t size;
        u8_t *buf;
        WSABUF wsa_buf;
        queue_p queue;  // queue for async messages waiting to be sent
    } tx;

} *selector_p;

#else

typedef struct selector_t {
    i64_t fd;  // socket fd
    i64_t id;  // selector id

    selector_type_t type;

    poll_event_fn open_fn;
    poll_event_fn close_fn;
    poll_event_fn recv_fn;
    poll_event_fn recv_error_fn;
    poll_event_fn send_fn;
    poll_event_fn send_error_fn;

    raw_p data;

    struct {
        u8_t msgtype;
        i64_t bytes_transfered;
        i64_t size;
        u8_t *buf;
    } rx;

    struct {
        b8_t isset;
        i64_t bytes_transfered;
        i64_t size;
        u8_t *buf;
        queue_p queue;  // queue for data waiting to be sent
    } tx;

} *selector_p;

typedef enum poll_events_t {
    POLL_EVENT_READ = EPOLLIN,
    POLL_EVENT_WRITE = EPOLLOUT,
    POLL_EVENT_ERROR = EPOLLERR,
    POLL_EVENT_HUP = EPOLLHUP,
} poll_events_t;

typedef struct poll_t {
    i64_t code;
    i64_t fd;
    term_p term;
    freelist_p selectors;  // freelist of selectors
    timers_p timers;       // timers heap
} *poll_p;

typedef struct poll_registry_t {
    i64_t fd;
    selector_type_t type;
    poll_events_t events;
    poll_event_fn open_fn;
    poll_event_fn close_fn;
    poll_event_fn recv_fn;
    poll_event_fn recv_error_fn;
    poll_event_fn send_fn;
    poll_event_fn send_error_fn;
    raw_p data;
} *poll_registry_p;

poll_p poll_create();
nil_t poll_destroy(poll_p poll);
i64_t poll_run(poll_p poll);
i64_t poll_register(poll_p poll, poll_registry_p registry);
nil_t poll_deregister(poll_p poll, i64_t id);
selector_p poll_get_selector(poll_p poll, i64_t id);

// Exit the app
nil_t poll_exit(poll_p poll, i64_t code);

#endif  // OS_WINDOWS

#endif  // POLL_H
