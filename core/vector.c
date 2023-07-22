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

#include "rayforce.h"
#include "vector.h"
#include "util.h"
#include "format.h"
#include "env.h"

#define panic_type(m, t1) panic(str_fmt(0, "%s: '%s'", m, env_get_typename(t1)))
#define panic_type2(m, t1, t2) panic(str_fmt(0, "%s: '%s', '%s'", m, env_get_typename(t1), env_get_typename(t2)))

i64_t size_of_val(type_t type)
{
    switch (type)
    {
    case TYPE_BOOL:
        return sizeof(bool_t);
    case TYPE_I64:
        return sizeof(i64_t);
    case TYPE_F64:
        return sizeof(f64_t);
    case TYPE_SYMBOL:
        return sizeof(i64_t);
    case TYPE_TIMESTAMP:
        return sizeof(i64_t);
    case TYPE_GUID:
        return sizeof(guid_t);
    case TYPE_CHAR:
        return sizeof(char_t);
    case TYPE_LIST:
        return sizeof(rf_object_t);
    default:
        panic_type("size of val unknown type", type);
    }
}

/*
 * Creates new vector of type type
 */
rf_object_t vector(type_t type, i64_t len)
{
    i64_t size = capacity(len * size_of_val(type) + sizeof(header_t));
    header_t *adt = rf_malloc(size);

    if (adt == NULL)
        panic("OOM");

    adt->len = len;
    adt->rc = 1;
    adt->attrs = (attrs_t){0};

    rf_object_t v = {
        .type = type,
        .adt = adt,
    };

    return v;
}

rf_object_t _push(rf_object_t *vec, rf_object_t value)
{
    switch (vec->type)
    {
    case TYPE_BOOL:
        push(vec, bool_t, value.bool);
        return null();
    case TYPE_I64:
        push(vec, i64_t, value.i64);
        return null();
    case TYPE_F64:
        push(vec, f64_t, value.f64);
        return null();
    case TYPE_SYMBOL:
        push(vec, i64_t, value.i64);
        return null();
    case TYPE_TIMESTAMP:
        push(vec, i64_t, value.i64);
        return null();
    case TYPE_GUID:
        push(vec, guid_t, *value.guid);
        return null();
    case TYPE_CHAR:
        push(vec, char_t, value.schar);
        return null();
    case TYPE_LIST:
        push(vec, rf_object_t, value);
        return null();
    default:
        panic("vector push: can not push to a unknown type");
    }
}

rf_object_t list_push(rf_object_t *vec, rf_object_t value)
{
    debug_assert(is_vector(vec));

    if (vec->type != -value.type && vec->type != TYPE_LIST)
        panic("vector push: can not push to a non-list");

    return _push(vec, value);
}

rf_object_t vector_push(rf_object_t *vec, rf_object_t value)
{
    u64_t i, l;
    rf_object_t lst;

    debug_assert(is_vector(vec));

    l = vec->adt->len;

    if (l == 0)
    {
        if (is_scalar(&value))
            vec->type = -value.type;
        else
            vec->type = TYPE_LIST;
    }
    else
    {
        // change vector type to a list
        if (vec->type != -value.type && vec->type != TYPE_LIST)
        {
            lst = list(l + 1);
            for (i = 0; i < l; i++)
                as_list(&lst)[i] = vector_get(vec, i);

            as_list(&lst)[l] = value;

            rf_object_free(vec);

            *vec = lst;
            return null();
        }
    }

    return _push(vec, value);
}

rf_object_t vector_pop(rf_object_t *vec)
{
    guid_t *g;

    if (!is_vector(vec) || vec->adt->len == 0)
        return null();

    switch (vec->type)
    {
    case TYPE_BOOL:
        return bool(pop(vec, bool_t));
    case TYPE_I64:
        return i64(pop(vec, i64_t));
    case TYPE_F64:
        return f64(pop(vec, f64_t));
    case TYPE_SYMBOL:
        return symboli64(pop(vec, i64_t));
    case TYPE_TIMESTAMP:
        return i64(pop(vec, i64_t));
    case TYPE_GUID:
        g = as_vector_guid(vec);
        return guid(g[--vec->adt->len].data);
    case TYPE_CHAR:
        return schar(pop(vec, char_t));
    case TYPE_LIST:
        if (vec->adt == NULL)
            return null();
        return pop(vec, rf_object_t);
    default:
        panic_type("vector pop: unknown type", vec->type);
    }
}

null_t vector_reserve(rf_object_t *vec, u32_t len)
{
    switch (vec->type)
    {
    case TYPE_BOOL:
        reserve(vec, bool_t, len);
        return;
    case TYPE_I64:
        reserve(vec, i64_t, len);
        return;
    case TYPE_F64:
        reserve(vec, f64_t, len);
        return;
    case TYPE_SYMBOL:
        reserve(vec, i64_t, len);
        return;
    case TYPE_TIMESTAMP:
        reserve(vec, i64_t, len);
        return;
    case TYPE_GUID:
        reserve(vec, guid_t, len);
        return;
    case TYPE_CHAR:
        reserve(vec, char_t, len);
        return;
    case TYPE_LIST:
        if (vec->adt == NULL)
            panic_type("vector reserve: can not reserve a null", vec->type);
        reserve(vec, rf_object_t, len);
        return;
    default:
        panic_type("vector reserve: unknown type", vec->type);
    }
}

null_t vector_grow(rf_object_t *vec, u32_t len)
{
    debug_assert(is_vector(vec));

    // calculate size of vector with new length
    i64_t new_size = capacity(len * size_of_val(vec->type) + sizeof(header_t));

    rf_realloc(vec->adt, new_size);
    vec->adt->len = len;
}

null_t vector_shrink(rf_object_t *vec, u32_t len)
{
    debug_assert(is_vector(vec));

    if (vec->adt->len == len)
        return;

    // calculate size of vector with new length
    i64_t new_size = capacity(len * size_of_val(vec->type) + sizeof(header_t));

    rf_realloc(vec->adt, new_size);
    vec->adt->len = len;
}

i64_t vector_find(rf_object_t *vec, rf_object_t *key)
{
    char_t kc, *vc;
    bool_t kb, *vb;
    i64_t ki, *vi;
    f64_t kf, *vf;
    rf_object_t *kl, *vl;
    i64_t i, l;
    guid_t *kg, *vg;

    debug_assert(is_vector(vec));

    l = vec->adt->len;

    if (key->type != -vec->type && vec->type != TYPE_LIST)
        return l;

    switch (vec->type)
    {
    case TYPE_BOOL:
        vb = as_vector_bool(vec);
        kb = key->bool;
        for (i = 0; i < l; i++)
            if (vb[i] == kb)
                return i;
        return l;
    case TYPE_I64:
        vi = as_vector_i64(vec);
        ki = key->i64;
        for (i = 0; i < l; i++)
            if (vi[i] == ki)
                return i;
        return l;
    case TYPE_F64:
        vf = as_vector_f64(vec);
        kf = key->f64;
        for (i = 0; i < l; i++)
            if (vf[i] == kf)
                return i;
        return l;
    case TYPE_SYMBOL:
        vi = as_vector_symbol(vec);
        ki = key->i64;
        for (i = 0; i < l; i++)
            if (vi[i] == ki)
                return i;
        return l;
    case TYPE_TIMESTAMP:
        vi = as_vector_timestamp(vec);
        ki = key->i64;
        for (i = 0; i < l; i++)
            if (vi[i] == ki)
                return i;
        return l;
    case TYPE_GUID:
        vg = as_vector_guid(vec);
        kg = key->guid;
        for (i = 0; i < l; i++)
            if (memcmp(vg + i, kg, sizeof(guid_t)) == 0)
                return i;
        return l;
    case TYPE_CHAR:
        vc = as_string(vec);
        kc = key->schar;
        for (i = 0; i < l; i++)
            if (vc[i] == kc)
                return i;
        return l;
    case TYPE_LIST:
        vl = as_list(vec);
        kl = key;
        for (i = 0; i < l; i++)
            if (rf_object_eq(&vl[i], kl))
                return i;
        return l;
    default:
        return l;
    }
}

rf_object_t vector_get(rf_object_t *vec, i64_t index)
{
    i64_t l;

    debug_assert(is_vector(vec));

    l = vec->adt->len;

    switch (vec->type)
    {
    case TYPE_BOOL:
        if (index < l)
            return bool(as_vector_bool(vec)[index]);
        return bool(false);
    case TYPE_I64:
        if (index < l)
            return i64(as_vector_i64(vec)[index]);
        return i64(NULL_I64);
    case TYPE_F64:
        if (index < l)
            return f64(as_vector_f64(vec)[index]);
        return f64(NULL_F64);
    case TYPE_SYMBOL:
        if (index < l)
            return symboli64(as_vector_i64(vec)[index]);
        return symboli64(NULL_I64);
    case TYPE_TIMESTAMP:
        if (index < l)
            return timestamp(as_vector_i64(vec)[index]);
        return timestamp(NULL_I64);
    case TYPE_GUID:
        if (index < l)
            return guid(as_vector_guid(vec)[index].data);
        return guid(NULL);
    case TYPE_CHAR:
        if (index < l)
            return schar(as_string(vec)[index]);
        return schar(0);
    case TYPE_LIST:
        if (index < l)
            return rf_object_clone(&as_list(vec)[index]);
        return null();
    default:
        return null();
    }
}

rf_object_t vector_set(rf_object_t *vec, i64_t index, rf_object_t value)
{
    guid_t *g;
    i64_t i, l;
    rf_object_t lst;

    debug_assert(is_vector(vec));

    l = vec->adt->len;

    if (index >= l)
        return error(ERR_LENGTH, "vector set: index out of bounds");

    // change vector type to a list
    if (vec->type != -value.type && vec->type != TYPE_LIST)
    {
        lst = list(l);
        for (i = 0; i < l; i++)
            as_list(&lst)[i] = vector_get(vec, i);

        as_list(&lst)[index] = value;

        rf_object_free(vec);

        *vec = lst;

        return null();
    }

    switch (vec->type)
    {
    case TYPE_BOOL:
        as_vector_bool(vec)[index] = value.bool;
        break;
    case TYPE_I64:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_F64:
        as_vector_f64(vec)[index] = value.f64;
        break;
    case TYPE_SYMBOL:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_TIMESTAMP:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_GUID:
        g = as_vector_guid(vec);
        memcpy(g + index, value.guid, sizeof(guid_t));
        break;
    case TYPE_CHAR:
        as_string(vec)[index] = value.schar;
        break;
    case TYPE_LIST:
        rf_object_free(&as_list(vec)[index]);
        as_list(vec)[index] = value;
        break;
    default:
        panic_type("vector_set: unknown type", vec->type);
    }

    return null();
}

/*
 * same as vector_set, but does not check bounds and does not free old value in case of list
 */
null_t vector_write(rf_object_t *vec, i64_t index, rf_object_t value)
{
    guid_t *g;
    i64_t i, l;
    rf_object_t lst;

    l = vec->adt->len;

    // change vector type to a list
    if (vec->type != -value.type && vec->type != TYPE_LIST)
    {
        lst = list(l);

        for (i = 0; i < index; i++)
            as_list(&lst)[i] = vector_get(vec, i);

        as_list(&lst)[index] = value;

        rf_object_free(vec);

        *vec = lst;

        return;
    }

    switch (vec->type)
    {
    case TYPE_BOOL:
        as_vector_bool(vec)[index] = value.bool;
        break;
    case TYPE_I64:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_F64:
        as_vector_f64(vec)[index] = value.f64;
        break;
    case TYPE_SYMBOL:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_TIMESTAMP:
        as_vector_i64(vec)[index] = value.i64;
        break;
    case TYPE_GUID:
        g = as_vector_guid(vec);
        memcpy(g + index, value.guid, sizeof(guid_t));
        break;
    case TYPE_CHAR:
        as_string(vec)[index] = value.schar;
        break;
    case TYPE_LIST:
        as_list(vec)[index] = value;
        break;
    default:
        panic_type("vector_set: unknown type", vec->type);
    }
}

rf_object_t vector_filter(rf_object_t *vec, bool_t mask[], i64_t len)
{
    i64_t i, j = 0, l, ol;
    rf_object_t res;

    debug_assert(is_vector(vec));

    l = vec->adt->len;
    ol = (len == NULL_I64) ? (i64_t)vec->adt->len : len;

    switch (vec->type)
    {
    case TYPE_BOOL:
        res = vector_bool(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_bool(&res)[j++] = as_vector_bool(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_I64:
        res = vector_i64(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_i64(&res)[j++] = as_vector_i64(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_F64:
        res = vector_f64(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_f64(&res)[j++] = as_vector_f64(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_SYMBOL:
        res = vector_symbol(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_i64(&res)[j++] = as_vector_i64(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_TIMESTAMP:
        res = vector_timestamp(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_i64(&res)[j++] = as_vector_i64(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_GUID:
        res = vector_guid(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_vector_guid(&res)[j++] = as_vector_guid(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_CHAR:
        res = string(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_string(&res)[j++] = as_string(vec)[i];
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    case TYPE_LIST:
        res = list(ol);
        for (i = 0; (j < ol && i < l); i++)
            if (mask[i])
                as_list(&res)[j++] = rf_object_clone(&as_list(vec)[i]);
        if (len == NULL_I64)
            vector_shrink(&res, j);
        return res;
    default:
        panic_type("vector_filter: unknown type", vec->type);
    }
}

null_t vector_clear(rf_object_t *vec)
{
    if (vec->type == TYPE_LIST)
    {
        i64_t i, l = vec->adt->len;
        rf_object_t *list = as_list(vec);

        for (i = 0; i < l; i++)
            rf_object_free(&list[i]);
    }

    vector_shrink(vec, 0);
}

null_t vector_free(rf_object_t *vec)
{
    rf_free(vec->adt);
}

rf_object_t rf_list(rf_object_t *x, u32_t n)
{
    rf_object_t l = list(n);
    u32_t i;

    for (i = 0; i < n; i++)
        as_list(&l)[i] = rf_object_clone(x + i);

    return l;
}

rf_object_t rf_enlist(rf_object_t *x, u32_t n)
{
    rf_object_t l = list(0);
    u32_t i;

    for (i = 0; i < n; i++)
    {
        rf_object_t *item = x + i;
        vector_push(&l, rf_object_clone(item));
    }

    return l;
}
