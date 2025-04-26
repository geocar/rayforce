/*
 *   Copyright (c) 2025 Anton Kundenko <singaraiona@gmail.com>
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

#include "ipc.h"
#include "eval.h"
#include "binary.h"
#include "symbols.h"
#include "error.h"

nil_t poll_set_usr_fd(i64_t fd) {
    obj_p s, k, v;

    s = symbol(".z.w", 4);
    k = i64(fd);
    v = binary_set(s, k);
    drop_obj(k);
    drop_obj(v);
    drop_obj(s);
}

poll_result_t ipc_on_open(poll_p poll, selector_p selector) {
    UNUSED(poll);
    // i64_t clbnm;
    // obj_p v, f, *clbfn;

    // stack_push(NULL_OBJ);  // null env
    // clbnm = symbols_intern(".z.po", 5);
    // clbfn = resolve(clbnm);
    // stack_pop();  // null env

    // // Call the callback if it's a lambda
    // if (clbfn != NULL && (*clbfn)->type == TYPE_LAMBDA) {
    //     poll_set_usr_fd(selector->id);
    //     stack_push(i64(selector->id));
    //     v = call(*clbfn, 1);
    //     drop_obj(stack_pop());
    //     poll_set_usr_fd(0);
    //     if (IS_ERR(v)) {
    //         f = obj_fmt(v, B8_FALSE);
    //         fprintf(stderr, "Error in .z.po callback: \n%.*s\n", (i32_t)f->len, AS_C8(f));
    //         drop_obj(f);
    //     }

    //     drop_obj(v);
    // }

    return POLL_READY;
}

poll_result_t ipc_on_close(poll_p poll, selector_p selector) {
    UNUSED(poll);
    // i64_t clbnm;
    // obj_p v, f, *clbfn;

    // stack_push(NULL_OBJ);  // null env
    // clbnm = symbols_intern(".z.pc", 5);
    // clbfn = resolve(clbnm);
    // stack_pop();  // null env

    // // Call the callback if it's a lambda
    // if (clbfn != NULL && (*clbfn)->type == TYPE_LAMBDA) {
    //     poll_set_usr_fd(selector->id);
    //     stack_push(i64(selector->id));
    //     v = call(*clbfn, 1);
    //     drop_obj(stack_pop());
    //     poll_set_usr_fd(0);
    //     if (IS_ERR(v)) {
    //         f = obj_fmt(v, B8_FALSE);
    //         fprintf(stderr, "Error in .z.pc callback: \n%.*s\n", (i32_t)f->len, AS_C8(f));
    //         drop_obj(f);
    //     }

    //     drop_obj(v);
    // }

    return POLL_READY;
}

// accept new connections
// else if (ev.data.fd == poll->ipc_fd) {
//     sock = sock_accept(poll->ipc_fd);
//     if (sock != -1)
//         poll_register(poll, sock, 0);
// }
i64_t ipc_listen(poll_p poll, i64_t port) {
    i64_t listen_fd;
    // struct epoll_event ev;

    // if (poll == NULL)
    //     return -1;

    // if (poll->ipc_fd != -1)
    //     return -2;

    // listen_fd = sock_listen(port);
    // if (listen_fd == -1)
    //     return -1;

    // ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    // ev.data.fd = listen_fd;

    // if (epoll_ctl(poll->poll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
    //     return -1;

    // poll->ipc_fd = listen_fd;

    return listen_fd;
}

poll_result_t ipc_on_error(poll_p poll, selector_p selector) {
    UNUSED(poll);
    UNUSED(selector);

    return POLL_READY;
}

poll_result_t ipc_send_handshake(poll_p poll, selector_p selector) {
    UNUSED(poll);

    // i64_t sz, size;
    // u8_t handshake[2] = {RAYFORCE_VERSION, 0x00};

    // size = 0;
    // while (size < (i64_t)sizeof(handshake)) {
    //     sz = sock_send(selector->fd, &handshake[size], sizeof(handshake) - size);

    //     if (sz == -1)
    //         return POLL_ERROR;

    //     size += sz;
    // }

    return POLL_READY;
}

poll_result_t ipc_recv_handshake(poll_p poll, selector_p selector) {
    UNUSED(poll);

    i64_t size;

    // if (selector->rx.buf == NULL)
    //     selector->rx.buf = (u8_t *)heap_alloc(sizeof(struct header_t));

    // while (selector->rx.bytes_transfered == 0 || selector->rx.buf[selector->rx.bytes_transfered - 1] != '\0') {
    //     size = sock_recv(selector->fd, &selector->rx.buf[selector->rx.bytes_transfered], sizeof(struct header_t));
    //     if (size == -1)
    //         return POLL_ERROR;
    //     else if (size == 0)
    //         return POLL_PENDING;

    //     selector->rx.bytes_transfered += size;

    //     if (selector->rx.bytes_transfered == sizeof(struct header_t)) {
    //         if (heap_realloc(selector->rx.buf, selector->rx.bytes_transfered + sizeof(struct header_t)) == NULL)
    //             return POLL_ERROR;
    //     }
    // }

    // selector->rx.bytes_transfered = 0;

    // // send handshake response
    // if (ipc_send_handshake(poll, selector) == POLL_ERROR)
    //     return POLL_ERROR;

    // // Now we are ready for income messages and can call userspace callback (if any)
    // poll_call_usr_on_open(poll, selector->id);

    // selector->recv_fn = ipc_recv_header;

    return ipc_recv_header(poll, selector);
}

poll_result_t ipc_recv_msg(poll_p poll, selector_p selector) {
    UNUSED(poll);

    // i64_t size;

    // while (selector->rx.bytes_transfered < selector->rx.size) {
    //     size = sock_recv(selector->fd, &selector->rx.buf[selector->rx.bytes_transfered],
    //                      selector->rx.size - selector->rx.bytes_transfered);
    //     if (size == -1)
    //         return POLL_ERROR;
    //     else if (size == 0)
    //         return POLL_PENDING;

    //     selector->rx.bytes_transfered += size;
    // }

    return POLL_READY;
}

poll_result_t ipc_recv_header(poll_p poll, selector_p selector) {
    UNUSED(poll);

    // i64_t size;
    // header_t *header;

    // while (selector->rx.bytes_transfered < (i64_t)sizeof(struct header_t)) {
    //     size = sock_recv(selector->fd, &selector->rx.buf[selector->rx.bytes_transfered],
    //                      sizeof(struct header_t) - selector->rx.bytes_transfered);
    //     if (size == -1)
    //         return POLL_ERROR;
    //     else if (size == 0)
    //         return POLL_PENDING;

    //     selector->rx.bytes_transfered += size;
    // }

    // header = (header_t *)selector->rx.buf;
    // selector->rx.msgtype = header->msgtype;
    // selector->rx.size = header->size + sizeof(struct header_t);
    // selector->rx.buf = (u8_t *)heap_realloc(selector->rx.buf, selector->rx.size);

    return POLL_READY;
}

poll_result_t ipc_recv(poll_p poll, selector_p selector) {
    if (selector->rx.buf == NULL)
        selector->rx.buf = (u8_t *)heap_alloc(sizeof(struct header_t));

    return ipc_recv_header(poll, selector);
}

// Send
poll_result_t _send(poll_p poll, selector_p selector) {
    //     i64_t size;
    //     obj_p obj;
    //     nil_t *v;
    //     i8_t msg_type = MSG_TYPE_RESP;
    //     struct epoll_event ev;

    // send:
    //     while (selector->tx.bytes_transfered < selector->tx.size) {
    //         size = sock_send(selector->fd, &selector->tx.buf[selector->tx.bytes_transfered],
    //                          selector->tx.size - selector->tx.bytes_transfered);
    //         if (size == -1)
    //             return POLL_ERROR;
    //         else if (size == 0) {
    //             // setup epoll for EPOLLOUT only if it's not already set
    //             if (!selector->tx.isset) {
    //                 selector->tx.isset = B8_TRUE;
    //                 ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
    //                 ev.data.ptr = selector;
    //                 if (epoll_ctl(poll->fd, EPOLL_CTL_MOD, selector->fd, &ev) == -1)
    //                     return POLL_ERROR;
    //             }

    //             return POLL_PENDING;
    //         }

    //         selector->tx.bytes_transfered += size;
    //     }

    //     heap_free(selector->tx.buf);
    //     selector->tx.buf = NULL;
    //     selector->tx.size = 0;
    //     selector->tx.bytes_transfered = 0;

    //     v = queue_pop(selector->tx.queue);

    //     if (v != NULL) {
    //         obj = (obj_p)((i64_t)v & ~(3ll << 61));
    //         msg_type = (((i64_t)v & (3ll << 61)) >> 61);
    //         size = ser_raw(&selector->tx.buf, obj);
    //         selector->tx.size = size;
    //         drop_obj(obj);
    //         if (size == -1)
    //             return POLL_ERROR;

    //         ((header_t *)selector->tx.buf)->msgtype = msg_type;
    //         goto send;
    //     }

    //     // remove EPOLLOUT only if it's set
    //     if (selector->tx.isset) {
    //         selector->tx.isset = B8_FALSE;
    //         ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
    //         ev.data.ptr = selector;
    //         if (epoll_ctl(poll->fd, EPOLL_CTL_MOD, selector->fd, &ev) == -1)
    //             return POLL_ERROR;
    //     }

    return POLL_READY;
}

obj_p read_obj(selector_p selector) {
    obj_p res;

    res = de_raw(selector->rx.buf, selector->rx.size);
    heap_free(selector->rx.buf);
    selector->rx.buf = NULL;
    selector->rx.bytes_transfered = 0;
    selector->rx.size = 0;

    return res;
}

nil_t process_request(poll_p poll, selector_p selector) {
    // poll_result_t poll_result;
    // obj_p v, res;

    // res = read_obj(selector);

    // poll_set_usr_fd(selector->id);

    // if (IS_ERR(res) || is_null(res))
    //     v = res;
    // else if (res->type == TYPE_C8) {
    //     // v = ray_eval_str(res, poll->ipcfile);
    //     drop_obj(res);
    // } else {
    //     v = eval_obj(res);
    //     drop_obj(res);
    // }

    // poll_set_usr_fd(0);

    // // sync request
    // if (selector->rx.msgtype == MSG_TYPE_SYNC) {
    //     queue_push(selector->tx.queue, (nil_t *)((i64_t)v | ((i64_t)MSG_TYPE_RESP << 61)));
    //     poll_result = _send(poll, selector);

    //     if (poll_result == POLL_ERROR)
    //         poll_deregister(poll, selector->id);
    // } else
    //     drop_obj(v);
}

obj_p ipc_send_sync(poll_p poll, i64_t id, obj_p msg) {
    poll_result_t poll_result = POLL_PENDING;
    selector_p selector;
    i32_t result;
    i64_t idx;
    obj_p res;
    fd_set fds;

    //     idx = freelist_get(poll->selectors, id - SELECTOR_ID_OFFSET);

    //     if (idx == NULL_I64)
    //         THROW(ERR_IO, "ipc_send_sync: invalid socket fd: %lld", id);

    //     selector = (selector_p)idx;

    //     queue_push(selector->tx.queue, (nil_t *)((i64_t)msg | ((i64_t)MSG_TYPE_SYNC << 61)));

    //     while (B8_TRUE) {
    //         poll_result = _send(poll, selector);

    //         if (poll_result != POLL_PENDING)
    //             break;

    //         // block on select until we can send
    //         FD_ZERO(&fds);
    //         FD_SET(selector->fd, &fds);
    //         result = select(selector->fd + 1, NULL, &fds, NULL, NULL);

    //         if (result == -1) {
    //             if (errno != EINTR) {
    //                 poll_deregister(poll, selector->id);
    //                 return sys_error(ERROR_TYPE_OS, "ipc_send_sync: error sending message (can't block on send)");
    //             }
    //         }
    //     }

    //     if (poll_result == POLL_ERROR) {
    //         poll_deregister(poll, selector->id);
    //         return sys_error(ERROR_TYPE_OS, "ipc_send_sync: error sending message");
    //     }

    // recv:
    //     while (B8_TRUE) {
    //         poll_result = _recv(poll, selector);

    //         if (poll_result != POLL_PENDING)
    //             break;

    //         // block on select until we can recv
    //         FD_ZERO(&fds);
    //         FD_SET(selector->fd, &fds);
    //         result = select(selector->fd + 1, &fds, NULL, NULL, NULL);

    //         if (result == -1) {
    //             if (errno != EINTR) {
    //                 poll_deregister(poll, selector->id);
    //                 return sys_error(ERROR_TYPE_OS, "ipc_send_sync: error receiving message (can't block on recv)");
    //             }
    //         }
    //     }

    //     if (poll_result == POLL_ERROR) {
    //         poll_deregister(poll, selector->id);
    //         return sys_error(ERROR_TYPE_OS, "ipc_send_sync: error receiving message");
    //     }

    //     // recv until we get response
    //     switch (selector->rx.msgtype) {
    //         case MSG_TYPE_RESP:
    //             res = read_obj(selector);
    //             break;
    //         default:
    //             process_request(poll, selector);
    //             goto recv;
    //     }

    return res;
}

obj_p ipc_send_async(poll_p poll, i64_t id, obj_p msg) {
    i64_t idx;
    selector_p selector;

    // idx = freelist_get(poll->selectors, id - SELECTOR_ID_OFFSET);

    // if (idx == NULL_I64)
    //     THROW(ERR_IO, "ipc_send_sync: invalid socket fd: %lld", id);

    // selector = (selector_p)idx;
    // if (selector == NULL)
    //     THROW(ERR_IO, "ipc_send_async: invalid socket fd: %lld", id);

    // queue_push(selector->tx.queue, (nil_t *)msg);

    // if (_send(poll, selector) == POLL_ERROR)
    //     THROW(ERR_IO, "ipc_send_async: error sending message");

    return NULL_OBJ;
}
