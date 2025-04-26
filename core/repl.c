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

#include <stdio.h>
#include <unistd.h>
#include "repl.h"
#include "io.h"
#include "eval.h"
#include "term.h"
#include "error.h"
#include "symbols.h"
#include "heap.h"
#include "poll.h"

repl_p repl_create(poll_p poll) {
    repl_p repl;
    struct poll_registry_t registry;

    repl = (repl_p)heap_alloc(sizeof(struct repl_t));
    repl->name = symbol("repl", 4);
    repl->term = term_create();

    registry.fd = STDIN_FILENO;
    registry.type = SELECTOR_TYPE_STDIN;
    registry.events = POLL_EVENT_READ | POLL_EVENT_ERROR | POLL_EVENT_HUP;
    registry.open_fn = repl_on_open;
    registry.close_fn = repl_on_close;
    registry.recv_fn = repl_recv;
    registry.recv_error_fn = repl_on_error;
    registry.send_fn = NULL;
    registry.send_error_fn = NULL;
    registry.data = repl;

    repl->id = poll_register(poll, &registry);

    if (repl->id == NULL_I64) {
        repl_destroy(repl);
        return NULL;
    }

    return repl;
}

nil_t repl_destroy(repl_p repl) {
    drop_obj(repl->name);
    term_destroy(repl->term);
    heap_free(repl);
}

poll_result_t repl_on_open(poll_p poll, selector_p selector) {
    UNUSED(poll);
    UNUSED(selector);

    repl_p repl = (repl_p)selector->data;

    term_prompt(repl->term);

    return POLL_READY;
}

poll_result_t repl_on_close(poll_p poll, selector_p selector) {
    repl_destroy(selector->data);

    return POLL_READY;
}

poll_result_t repl_on_error(poll_p poll, selector_p selector) {
    UNUSED(poll);
    UNUSED(selector);

    return POLL_READY;
}

poll_result_t repl_recv(poll_p poll, selector_p selector) {
    b8_t error;
    obj_p str, res;
    repl_p repl = (repl_p)selector->data;

    if (!term_getc(poll->term)) {
        poll->code = 1;
        return POLL_EXIT;
    }

    str = term_read(poll->term);
    if (str != NULL) {
        if (IS_ERR(str))
            io_write(STDOUT_FILENO, 2, str);
        else if (str != NULL_OBJ) {
            res = ray_eval_str(str, repl->name);
            drop_obj(str);
            io_write(STDOUT_FILENO, 2, res);
            error = IS_ERR(res);
            drop_obj(res);
            if (!error)
                timeit_print();
        }

        term_prompt(poll->term);
    }

    return POLL_READY;
}
