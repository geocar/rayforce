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
#include "alloc.h"
#include "vm.h"
#include "ops.h"
#include "util.h"
#include "format.h"
#include "sort.h"
#include "vector.h"
#include "guid.h"
#include "set.h"

rf_object_t rf_til_i64(rf_object_t *x)
{
    i32_t i, l = (i32_t)x->i64;
    i64_t *v;
    rf_object_t vec;

    vec = vector_i64(l);

    v = as_vector_i64(&vec);

    for (i = 0; i < l; i++)
        v[i] = i;

    vec.adt->attrs = VEC_ATTR_ASC | VEC_ATTR_WITHOUT_NULLS | VEC_ATTR_DISTINCT;
    return vec;
}

u32_t i64_hash(i64_t key)
{
    return (u32_t)key;
}

rf_object_t rf_distinct_I64(rf_object_t *x)
{
#define MAX_LINEAR_VALUE 1024 * 1024 * 64
#define normalize(k) ((u64_t)(k - min))

    i64_t i, j = 0, p = 0, w = 0, xl = x->adt->len;
    i64_t n = 0, range, min, max, *m, *iv1 = as_vector_i64(x), *ov;
    rf_object_t mask, vec;
    set_t *set;

    if (xl == 0)
        return vector_i64(0);

    if (x->adt->attrs & VEC_ATTR_DISTINCT)
        return rf_object_clone(x);

    max = min = iv1[0];

    for (i = 0; i < xl; i++)
    {
        if (iv1[i] < min)
            min = iv1[i];
        else if (iv1[i] > max)
            max = iv1[i];
    }

    range = max - min + 1;

    if (range < MAX_LINEAR_VALUE)
    {
        vec = vector_i64(xl);
        ov = as_vector_i64(&vec);

        mask = vector_i64(range / 64);
        m = as_vector_i64(&mask);

        for (i = 0; i < range / 64; i++)
            m[i] = 0;

        for (i = 0; i < xl; i++)
        {
            n = normalize(iv1[i]);
            p = n / 64;
            w = 1ull << (n & 63);

            if (!(m[p] & w))
            {
                m[p] |= w;
                ov[j++] = iv1[i];
            }
        }

        rf_object_free(&mask);

        vector_shrink(&vec, j);

        vec.adt->attrs |= VEC_ATTR_DISTINCT;

        return vec;
    }

    vec = vector_i64(xl);
    ov = as_vector_i64(&vec);

    set = set_new(xl, &kmh_hash, &i64_cmp);

    j = 0;
    for (i = 0; i < xl; i++)
        if (set_insert(set, iv1[i]))
            ov[j++] = iv1[i];

    vec.adt->attrs |= VEC_ATTR_DISTINCT;

    vector_shrink(&vec, j);

    return vec;
}

i64_t cnt_update(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval)
{
    UNUSED(key);
    UNUSED(*tkey);
    UNUSED(seed);

    *tval += 1;
    return val;
}

i64_t pos_update(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval)
{
    UNUSED(key);
    UNUSED(*tkey);

    // u32_t idx = (u32_t)*tval;
    // rf_object_t *vv = (rf_object_t *)seed + idx;
    // i64_t *vi = as_vector_i64(vv);
    // vi[vv->adt->len++] = val;

    return val;
}

rf_object_t rf_group_I64(rf_object_t *x)
{
    i64_t i, j = 0, l, xl = x->adt->len, *iv1 = as_vector_i64(x), *kv, *ek, *ev;
    rf_object_t keys, vals, *vv, v;
    ht_t *ht;

    keys = vector_i64(xl);
    kv = as_vector_i64(&keys);
    vals = list(xl);
    vv = as_list(&vals);

    ht = ht_new(xl, &kmh_hash, &i64_cmp);

    // calculate counts for each key
    for (i = 0; i < xl; i++)
    {
        if (iv1[i] == NULL_I64)
            panic("NULL_I64");
        ht_upsert_with(ht, iv1[i], 1, NULL, &cnt_update);
    }

    // allocate vectors of positions for each key
    ek = as_vector_i64(&ht->keys);
    ev = as_vector_i64(&ht->vals);
    l = ht->size;

    for (i = 0; i < l; i++)
    {
        if (ek[i] == NULL_I64)
            continue;

        kv[j] = ek[i];
        v = vector_i64(ev[i]);
        v.adt->len = 0;
        ev[i] = j;
        vv[j++] = v;
    }

    // finally, fill vectors with positions
    for (i = 0; i < xl; i++)
        ht_upsert_with(ht, iv1[i], i, vv, &pos_update);

    ht_free(ht);

    vector_shrink(&keys, j);
    vector_shrink(&vals, j);

    return dict(keys, vals);
}

rf_object_t rf_sum_I64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len, sum = 0, *v = as_vector_i64(x);

    for (i = 0; i < l; i++)
        sum += v[i];

    return i64(sum);
}

rf_object_t rf_avg_I64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len, sum = 0,
          *v = as_vector_i64(x), n = 0;

    // vectorized version when we exactly know that there are no nulls
    if (x->adt->attrs & VEC_ATTR_WITHOUT_NULLS)
    {
        for (i = 0; i < l; i++)
            sum += v[i];

        return i64(sum / l);
    }

    // scalar version
    for (i = 0; i < l; i++)
    {
        if (v[i] ^ NULL_I64)
            sum += v[i];
        else
            n++;
    }

    return f64((f64_t)sum / (l - n));
}

rf_object_t rf_min_I64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len, min = 0,
          *v = as_vector_i64(x);

    if (!l)
        return i64(NULL_I64);

    // vectorized version when we exactly know that there are no nulls
    if (x->adt->attrs & VEC_ATTR_WITHOUT_NULLS)
    {
        if (x->adt->attrs & VEC_ATTR_ASC)
            return i64(v[0]);
        if (x->adt->attrs & VEC_ATTR_DESC)
            return i64(v[l - 1]);
        min = v[0];
        for (i = 1; i < l; i++)
            if (v[i] < min)
                min = v[i];

        return i64(min);
    }

    // scalar version
    // find first nonnull value
    for (i = 0; i < l; i++)
        if (v[i] ^ NULL_I64)
        {
            min = v[i];
            break;
        }

    for (i = 0; i < l; i++)
    {
        if (v[i] ^ NULL_I64)
            min = v[i] < min ? v[i] : min;
    }

    return i64(min);
}

rf_object_t rf_max_I64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len, max = x->adt->len ? as_vector_i64(x)[0] : 0, *v = as_vector_i64(x);

    if (x->adt->attrs & VEC_ATTR_ASC)
        return i64(v[l - 1]);
    if (x->adt->attrs & VEC_ATTR_DESC)
        return i64(v[0]);

    for (i = 0; i < l; i++)
        if (v[i] > max)
            max = v[i];

    return i64(max);
}

rf_object_t rf_sum_F64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len;
    f64_t sum = 0, *v = as_vector_f64(x);

    for (i = 0; i < l; i++)
        sum += v[i];

    return f64(sum);
}

rf_object_t rf_avg_F64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len;
    f64_t sum = 0, *v = as_vector_f64(x);

    for (i = 0; i < l; i++)
        sum += v[i];

    return f64(sum / l);
}

rf_object_t rf_min_F64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len;
    f64_t min = x->adt->len ? as_vector_f64(x)[0] : 0, *v = as_vector_f64(x);

    for (i = 0; i < l; i++)
        if (v[i] < min)
            min = v[i];

    return f64(min);
}

rf_object_t rf_max_F64(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len;
    f64_t max = x->adt->len ? as_vector_f64(x)[0] : 0, *v = as_vector_f64(x);

    for (i = 0; i < l; i++)
        if (v[i] > max)
            max = v[i];

    return f64(max);
}

rf_object_t rf_count(rf_object_t *x)
{
    if (x->type < TYPE_NULL)
        return i64(1);

    switch (x->type)
    {
    case TYPE_FUNCTION:
        return i64(1);
    default:
        return i64(x->adt->len);
    }
}

rf_object_t rf_not_bool(rf_object_t *x)
{
    return bool(!x->bool);
}

rf_object_t rf_not_Bool(rf_object_t *x)
{
    i32_t i;
    i64_t l = x->adt->len;
    rf_object_t res = vector_bool(l);
    bool_t *iv = as_vector_bool(x), *ov = as_vector_bool(&res);

    for (i = 0; i < l; i++)
        ov[i] = !iv[i];

    return res;
}

rf_object_t rf_iasc_I64(rf_object_t *x)
{
    return rf_sort_asc(x);
}

rf_object_t rf_idesc_I64(rf_object_t *x)
{
    return rf_sort_desc(x);
}

rf_object_t rf_asc_I64(rf_object_t *x)
{
    rf_object_t idx = rf_sort_asc(x);
    i64_t len = x->adt->len, i,
          *iv = as_vector_i64(x), *ov = as_vector_i64(&idx);

    for (i = 0; i < len; i++)
        ov[i] = iv[ov[i]];

    idx.adt->attrs |= VEC_ATTR_ASC;

    return idx;
}

rf_object_t rf_desc_I64(rf_object_t *x)
{
    rf_object_t idx = rf_sort_desc(x);
    i64_t len = x->adt->len, i,
          *iv = as_vector_i64(x), *ov = as_vector_i64(&idx);

    for (i = 0; i < len; i++)
        ov[i] = iv[ov[i]];

    idx.adt->attrs |= VEC_ATTR_DESC;

    return idx;
}

rf_object_t rf_flatten_List(rf_object_t *x)
{
    return list_flatten(x);
}

rf_object_t rf_guid_generate(rf_object_t *x)
{
    i64_t i, count = x->i64;
    rf_object_t vec = vector_guid(count);
    guid_t *g = as_vector_guid(&vec);

    for (i = 0; i < count; i++)
        guid_generate(g + i);

    return vec;
}