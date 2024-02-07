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

#include "update.h"
#include "heap.h"
#include "util.h"
#include "runtime.h"
#include "error.h"
#include "eval.h"
#include "filter.h"
#include "binary.h"
#include "unary.h"
#include "vary.h"
#include "iter.h"

#define uncow(o, v, r)            \
    if ((v == NULL) || (*v != o)) \
        drop(o);                  \
    return r;

obj_t __fetch(obj_t obj, obj_t **val)
{
    if (obj->type == -TYPE_SYMBOL)
    {
        *val = deref(obj);
        if (*val == NULL)
            throw(ERR_NOT_FOUND, "fetch: symbol not found");

        obj = cow(**val);
    }
    else
        obj = cow(obj);

    return obj;
}

obj_t __alter(obj_t *obj, obj_t *x, u64_t n)
{
    obj_t v, idx, res;

    // special case for set
    if (x[1]->i64 == (i64_t)ray_set || x[1]->i64 == (i64_t)ray_let)
    {
        if (n != 4)
            throw(ERR_LENGTH, "alter: set expected a value");

        return set_obj(obj, x[2], clone(x[3]));
    }

    // retrieve the object via indices
    v = at_obj(*obj, x[2]);

    if (is_error(v))
        return v;

    idx = x[2];
    x[2] = v;
    res = ray_apply(x + 1, n - 1);
    x[2] = idx;
    drop(v);

    if (is_error(res))
        return res;

    return set_obj(obj, idx, res);
}

obj_t __commit(obj_t src, obj_t obj, obj_t *val)
{
    if (src->type == -TYPE_SYMBOL)
    {
        if ((val != NULL) && (*val != obj))
        {
            drop(*val);
            *val = obj;
        }

        return clone(src);
    }

    return obj;
}

obj_t ray_alter(obj_t *x, u64_t n)
{
    obj_t *val = NULL, obj, res;

    if (n < 3)
        throw(ERR_LENGTH, "alter: expected at least 3 arguments");

    if (x[1]->type < TYPE_LAMBDA || x[1]->type > TYPE_VARY)
        throw(ERR_TYPE, "alter: expected function as 2nd argument");

    obj = __fetch(x[0], &val);

    if (is_error(obj))
        return obj;

    res = __alter(&obj, x, n);
    if (is_error(res))
    {
        uncow(obj, val, res);
        return res;
    }

    return __commit(x[0], obj, val);
}

obj_t __modify(obj_t *obj, obj_t *x, u64_t n)
{
    obj_t v, idx, res;

    // special case for set
    if (x[1]->i64 == (i64_t)ray_set || x[1]->i64 == (i64_t)ray_let)
    {
        if (n != 4)
            throw(ERR_LENGTH, "alter: set expected a value");

        return set_obj(obj, x[2], clone(x[3]));
    }

    // retrieve the object via indices
    v = at_obj(*obj, x[2]);

    if (is_error(v))
        return v;

    idx = x[2];
    x[2] = v;
    res = ray_apply(x + 1, n - 1);
    x[2] = idx;
    drop(v);

    if (is_error(res))
        return res;

    return set_obj(obj, idx, res);
}

obj_t ray_modify(obj_t *x, u64_t n)
{
    obj_t *val = NULL, obj, res;

    if (n < 3)
        throw(ERR_LENGTH, "modify: expected at least 3 arguments, got %lld", n);

    if (x[1]->type < TYPE_LAMBDA || x[1]->type > TYPE_VARY)
        throw(ERR_TYPE, "modify: expected function as 2nd argument, got '%s'", typename(x[1]->type));

    obj = __fetch(x[0], &val);

    if (is_error(obj))
        return obj;

    res = __alter(&obj, x, n);
    if (is_error(res))
        uncow(obj, val, res);

    return obj;
}

/*
 * updates for tables
 */
obj_t ray_update(obj_t *x, u64_t n)
{
    unused(x);
    unused(n);
    throw(ERR_NOT_IMPLEMENTED, "update");
}

/*
 * inserts for tables
 */
obj_t ray_insert(obj_t *x, u64_t n)
{
    u64_t i, m, l;
    obj_t lst, col, *val = NULL, obj, res;
    bool_t need_drop;

    if (n < 2)
        throw(ERR_LENGTH, "insert: expected 2 arguments, got %lld", n);

    obj = __fetch(x[0], &val);

    if (is_error(obj))
        return obj;

    if (obj->type != TYPE_TABLE)
    {
        res = error(ERR_TYPE, "insert: expected 'Table as 1st argument, got '%s", typename(obj->type));
        uncow(obj, val, res);
    }

    // Check integrity of the table with the new object
    lst = x[1];
insert:
    switch (lst->type)
    {
    case TYPE_LIST:
        l = lst->len;
        if (l != as_list(obj)[0]->len)
        {
            res = error(ERR_LENGTH, "insert: expected list of length %lld, got %lld", as_list(obj)[0]->len, l);
            uncow(obj, val, res);
        }

        // There is one record to be inserted
        if (is_atom(as_list(lst)[0]))
        {
            // Check all the elements of the list
            for (i = 0; i < l; i++)
            {
                if ((as_list(as_list(obj)[1])[i]->type != TYPE_LIST) &&
                    (as_list(as_list(obj)[1])[i]->type != -as_list(lst)[i]->type))
                {
                    res = error(ERR_TYPE, "insert: expected '%s' as %lldth element, got '%s'", typename(-as_list(as_list(obj)[1])[i]->type), i, typename(as_list(lst)[i]->type));
                    uncow(obj, val, res);
                }
            }

            // Insert the record now
            for (i = 0; i < l; i++)
            {
                col = cow(as_list(as_list(obj)[1])[i]);
                need_drop = (col != as_list(as_list(obj)[1])[i]);
                push_obj(&col, clone(as_list(lst)[i]));
                if (need_drop)
                    drop(as_list(as_list(obj)[1])[i]);
                as_list(as_list(obj)[1])[i] = col;
            }
        }
        else
        {
            // There are multiple records to be inserted
            m = as_list(lst)[0]->len;
            if (m == 0)
            {
                res = error(ERR_LENGTH, "insert: expected non-empty list of records");
                uncow(obj, val, res);
            }

            // Check all the elements of the list
            for (i = 0; i < l; i++)
            {
                if ((as_list(as_list(obj)[1])[i]->type != TYPE_LIST) &&
                    (as_list(as_list(obj)[1])[i]->type != as_list(lst)[i]->type))
                {
                    res = error(ERR_TYPE, "insert: expected '%s' as %lldth element, got '%s'", typename(as_list(as_list(obj)[1])[i]->type), i, typename(as_list(lst)[i]->type));
                    uncow(obj, val, res);
                }

                if (as_list(lst)[i]->len != m)
                {
                    res = error(ERR_LENGTH, "insert: expected list of length %lld, as %lldth element in a values, got %lld", as_list(as_list(obj)[1])[i]->len, i, n);
                    uncow(obj, val, res);
                }
            }

            // Insert all the records now
            for (i = 0; i < l; i++)
            {
                col = cow(as_list(as_list(obj)[1])[i]);
                need_drop = (col != as_list(as_list(obj)[1])[i]);
                append(&col, as_list(lst)[i]);
                if (need_drop)
                    drop(as_list(as_list(obj)[1])[i]);
                as_list(as_list(obj)[1])[i] = col;
            }
        }

        break;

    case TYPE_TABLE:
        // Check columns
        l = as_list(lst)[0]->len;
        if (l != as_list(obj)[0]->len)
        {
            res = error(ERR_LENGTH, "insert: expected 'Table with the same number of columns");
            uncow(obj, val, res);
        }

        for (i = 0; i < l; i++)
        {
            if (as_symbol(as_list(lst)[0])[i] != as_symbol(as_list(obj)[0])[i])
            {
                res = error(ERR_TYPE, "insert: expected 'Table with the same columns");
                uncow(obj, val, res);
            }
        }

        lst = as_list(lst)[1];
        goto insert;

    default:
        res = error(ERR_TYPE, "insert: unsupported type '%s' as 2nd argument", typename(lst->type));
        uncow(obj, val, res);
    }

    return __commit(x[0], obj, val);
}

/*
 * update/inserts for tables
 */
obj_t ray_upsert(obj_t *x, u64_t n)
{
    unused(x);
    unused(n);
    throw(ERR_NOT_IMPLEMENTED, "upsert");
}
