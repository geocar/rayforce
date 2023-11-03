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

#include "runtime.h"
#include "mmap.h"
#include "heap.h"
#include "util.h"
#include "cc.h"
#include "poll.h"
#include "unary.h"
#include "io.h"

// Global runtime reference
__thread runtime_t __RUNTIME = NULL;

nil_t usage()
{
    printf("%s%s%s", BOLD, YELLOW, "Usage: rayforce [-f file...] [-p port]\n");
    exit(EXIT_FAILURE);
}

obj_t parse_cmdline(i32_t argc, str_t argv[])
{
    i32_t opt;
    obj_t keys = vector_symbol(0), vals = list(0), str;

    for (opt = 1; opt < argc && argv[opt][0] == '-'; opt++)
    {
        switch (argv[opt][1])
        {
        case 'f':
            opt++;

            if (argv[opt] == NULL)
                usage();

            push_sym(&keys, "file");
            str = string_from_str(argv[opt], strlen(argv[opt]));
            push_obj(&vals, str);
            break;

        case 'p':
            opt++;

            if (argv[opt] == NULL)
                usage();

            push_sym(&keys, "port");
            str = string_from_str(argv[opt], strlen(argv[opt]));
            push_obj(&vals, str);
            break;
        default:
            usage();
        }
    }

    argv += opt;

    return dict(keys, vals);
}

nil_t runtime_init(i32_t argc, str_t argv[])
{
    i64_t i;
    obj_t file, res;
    str_t fmt;

    heap_init();
    __RUNTIME = mmap_malloc(sizeof(struct runtime_t));
    __RUNTIME->sync = false;
    __RUNTIME->symbols = symbols_new();
    __RUNTIME->env = create_env();
    __RUNTIME->parser = parser_new();
    __RUNTIME->vm = vm_new(NULL);
    __RUNTIME->addr = (sock_addr_t){0};
    __RUNTIME->slaves = 0;
    __RUNTIME->fds = dict(vector_i64(0), vector_i64(0));

    if (argc)
    {
        __RUNTIME->args = parse_cmdline(argc, argv);
        i = find_sym(as_list(__RUNTIME->args)[0], "port");
        if (i < (i64_t)as_list(__RUNTIME->args)[0]->len)
            __RUNTIME->addr.port = atoi(as_string(as_list(as_list(__RUNTIME->args)[1])[i]));

        // load file
        file = runtime_get_arg("file");
        if (!is_null(file))
        {
            res = ray_load(file);
            fmt = obj_fmt(res);
            printf("%s\n", fmt);
            heap_free(fmt);
            dropn(2, file, res);
        }

        __RUNTIME->poll = poll_init(__RUNTIME->addr.port);
    }
    else
    {
        __RUNTIME->poll = NULL;
    }
}

i32_t runtime_run()
{
    return poll_run(__RUNTIME->poll);
}

nil_t runtime_cleanup()
{
    drop(__RUNTIME->args);
    if (__RUNTIME->poll)
        poll_cleanup(__RUNTIME->poll);
    symbols_free(__RUNTIME->symbols);
    mmap_free(__RUNTIME->symbols, sizeof(symbols_t));
    free_env(&__RUNTIME->env);
    drop(__RUNTIME->fds);
    parser_free(&__RUNTIME->parser);
    vm_free(&__RUNTIME->vm);
    mmap_free(__RUNTIME, sizeof(struct runtime_t));
    heap_cleanup();
    __RUNTIME = NULL;
}

runtime_t runtime_get()
{
    return __RUNTIME;
}

obj_t runtime_get_arg(str_t key)
{
    i64_t i = find_sym(as_list(__RUNTIME->args)[0], key);
    if (i < (i64_t)as_list(__RUNTIME->args)[0]->len)
        return at_idx(as_list(__RUNTIME->args)[1], i);
    return null(0);
}