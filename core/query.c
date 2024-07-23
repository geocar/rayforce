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

#include "query.h"
#include "env.h"
#include "util.h"
#include "unary.h"
#include "binary.h"
#include "vary.h"
#include "eval.h"
#include "items.h"
#include "compose.h"
#include "error.h"
#include "math.h"
#include "aggr.h"
#include "iter.h"
#include "index.h"
#include "group.h"
#include "filter.h"
#include "update.h"
#include "pool.h"
#include "time.h"
#include "runtime.h"

obj_p remap_filter(obj_p tab, obj_p index)
{
    return filter_map(tab, index);
}

obj_p remap_group(obj_p *gvals, obj_p cols, obj_p tab, obj_p filter, obj_p gkeys, obj_p gcols)
{
    u64_t i, l;
    obj_p index, v, lst, res;

    switch (gkeys->type)
    {
    case -TYPE_SYMBOL:
        index = index_group(cols, filter);
        timeit_tick("build index");

        if (is_error(index))
            return index;

        res = group_map(tab, index);
        v = (gcols == NULL_OBJ) ? aggr_first(cols, index) : aggr_first(gcols, index);
        if (is_error(v))
        {
            drop_obj(index);
            drop_obj(res);
            return v;
        }

        *gvals = v;
        drop_obj(index);

        timeit_tick("apply 'first' on group columns");

        return res;
    case TYPE_SYMBOL:
        index = index_group_list(cols, filter);
        timeit_tick("build compound index");

        if (is_error(index))
            return index;

        res = group_map(tab, index);

        l = cols->len;
        lst = list(l);

        for (i = 0; i < l; i++)
        {
            v = aggr_first(as_list(cols)[i], index);

            if (is_error(v))
            {
                lst->len = i;
                drop_obj(res);
                drop_obj(index);
                return v;
            }

            as_list(lst)[i] = v;
        }

        *gvals = lst;
        drop_obj(index);

        timeit_tick("apply 'first' on group columns");

        return res;
    default:
        return error(ERR_TYPE, "grouping key mapping(s) must be a symbol(s)");
    }
}

obj_p get_gkeys(obj_p cols, obj_p obj)
{
    u64_t i, l;
    obj_p x;

    switch (obj->type)
    {
    case -TYPE_SYMBOL:
        l = cols->len;
        for (i = 0; i < l; i++)
            if (as_i64(cols)[i] == obj->i64)
                return symboli64(obj->i64);
        return NULL_OBJ;
    case TYPE_LIST:
        l = obj->len;
        for (i = 0; i < l; i++)
        {
            x = get_gkeys(cols, as_list(obj)[i]);
            if (x != NULL_OBJ)
                return x;
        }
        return NULL_OBJ;
    case TYPE_DICT:
        x = as_list(obj)[0];
        if (x->type != TYPE_SYMBOL)
            return error(ERR_TYPE, "grouping key(s) must be a symbol(s)");

        if (x->len == 1)
            return at_idx(as_list(obj)[0], 0);

        return clone_obj(as_list(obj)[0]);

    default:
        return NULL_OBJ;
    }
}

obj_p get_gvals(obj_p obj)
{
    u64_t i, l;
    obj_p vals, v, r, res;

    switch (obj->type)
    {
    case TYPE_DICT:
        vals = as_list(obj)[1];
        l = vals->len;

        if (l == 0)
            return NULL_OBJ;

        if (l == 1)
        {
            v = at_idx(vals, 0);
            res = eval(v);
            drop_obj(v);
            return res;
        }

        res = list(l);
        for (i = 0; i < l; i++)
        {
            v = at_idx(vals, i);
            r = eval(v);
            drop_obj(v);

            if (is_error(r))
            {
                res->len = i;
                drop_obj(res);
                return r;
            }

            as_list(res)[i] = r;
        }

        return res;
    default:
        return eval(obj);
    }
}

nil_t query_ctx_init(query_ctx_p ctx)
{
    ctx->tablen = 0;
    ctx->table = NULL_OBJ;
    ctx->filter = NULL_OBJ;
    ctx->group_fields = NULL_OBJ;
    ctx->group_values = NULL_OBJ;
    ctx->query_fields = NULL_OBJ;
    ctx->query_values = NULL_OBJ;
}

nil_t query_ctx_destroy(query_ctx_p ctx)
{
    drop_obj(ctx->table);
    drop_obj(ctx->filter);
    drop_obj(ctx->group_fields);
    drop_obj(ctx->group_values);
    drop_obj(ctx->query_fields);
    drop_obj(ctx->query_values);
}

obj_p select_fetch_table(obj_p obj, query_ctx_p ctx)
{
    obj_p prm, val;

    prm = at_sym(obj, "from", 4);

    if (is_null(prm))
        throw(ERR_LENGTH, "'select' expects 'from' param");

    val = eval(prm);
    drop_obj(prm);

    if (is_error(val))
        return val;

    if (val->type != TYPE_TABLE)
    {
        drop_obj(val);
        throw(ERR_TYPE, "'select' from: expects table");
    }

    ctx->tablen = as_list(val)[0]->len;
    ctx->table = val;

    timeit_tick("fetch table");

    return NULL_OBJ;
}

obj_p select_apply_filters(obj_p obj, query_ctx_p ctx)
{
    obj_p prm, val, fil;

    prm = at_sym(obj, "where", 5);
    if (prm != NULL_OBJ)
    {
        val = eval(prm);
        drop_obj(prm);

        if (is_error(val))
            return val;

        fil = ray_where(val);
        drop_obj(val);

        if (is_error(fil))
            return fil;

        ctx->filter = fil;

        timeit_tick("apply filters");
    }

    return NULL_OBJ;
}

obj_p select_apply_groupings(obj_p obj, query_ctx_p ctx)
{
    u64_t tablen;
    obj_p prm, val, gkeys = NULL_OBJ, gvals = NULL_OBJ,
                    groupby = NULL_OBJ, gcol = NULL_OBJ;

    prm = at_sym(obj, "by", 2);
    if (prm != NULL_OBJ)
    {
        timeit_span_start("group");

        gkeys = get_gkeys(as_list(ctx->table)[0], prm);
        groupby = get_gvals(prm);

        if (gkeys == NULL_OBJ)
            gkeys = symbol("By", 2);
        else if (prm->type != TYPE_DICT)
            gvals = eval(gkeys);

        drop_obj(prm);

        tablen = ctx->tablen;
        unmount_env(ctx->tablen);
        ctx->tablen = 0;

        if (is_error(groupby))
        {
            drop_obj(gkeys);
            drop_obj(gvals);
            timeit_span_end("group");
            return groupby;
        }

        timeit_tick("get keys");

        prm = remap_group(&gcol, groupby, ctx->table, ctx->filter, gkeys, gvals);

        drop_obj(gvals);
        drop_obj(groupby);

        if (is_error(prm))
        {
            drop_obj(gkeys);
            timeit_span_end("group");
            return prm;
        }

        mount_env(prm);
        ctx->tablen = tablen;
        drop_obj(prm);

        if (is_error(gcol))
        {
            drop_obj(gkeys);
            timeit_span_end("group");
            return gcol;
        }

        ctx->group_fields = gkeys;
        ctx->group_values = gcol;

        timeit_span_end("group");
    }
    else if (ctx->filter != NULL_OBJ)
    {
        // Unmount table columns from a local env
        tablen = ctx->tablen;
        unmount_env(ctx->tablen);
        ctx->tablen = 0;

        val = remap_filter(ctx->table, ctx->filter);

        if (is_error(val))
            return val;

        mount_env(val);
        ctx->tablen = tablen;
        drop_obj(val);
    }

    return NULL_OBJ;
}

obj_p select_apply_mappings(obj_p obj, query_ctx_p ctx)
{
    u64_t i, l;
    obj_p prm, sym, val, keys, res;

    // Find all mappings (non-keyword fields)
    keys = ray_except(as_list(obj)[0], runtime_get()->env.keywords);
    l = keys->len;

    // Mapppings specified
    if (l)
    {
        res = list(l);

        for (i = 0; i < l; i++)
        {
            sym = at_idx(keys, i);
            prm = at_obj(obj, sym);
            drop_obj(sym);
            val = eval(prm);
            drop_obj(prm);

            if (is_error(val))
            {
                res->len = i;
                drop_obj(res);
                drop_obj(keys);
                return val;
            }

            // Materialize fields
            if (val->type == TYPE_GROUPMAP)
            {
                prm = aggr_collect(as_list(val)[0], as_list(val)[1]);
                drop_obj(val);
                val = prm;
            }
            else if (val->type == TYPE_FILTERMAP)
            {
                prm = filter_collect(as_list(val)[0], as_list(val)[1]);
                drop_obj(val);
                val = prm;
            }
            else if (val->type == TYPE_ENUM)
            {
                prm = ray_value(val);
                drop_obj(val);
                val = prm;
            }

            as_list(res)[i] = val;
        }

        ctx->query_fields = keys;
        ctx->query_values = res;

        timeit_tick("apply mappings");

        return NULL_OBJ;
    }

    drop_obj(keys);

    return NULL_OBJ;
}

obj_p select_collect_fields(query_ctx_p ctx)
{
    u64_t i, l;
    obj_p prm, sym, val, keys, res;

    // Already collected by mappings
    if (!is_null(ctx->query_fields))
        return NULL_OBJ;

    // Groupings
    if (!is_null(ctx->group_fields))
    {
        keys = ray_except(as_list(ctx->table)[0], ctx->group_fields);
        l = keys->len;
        res = list(l);

        for (i = 0; i < l; i++)
        {
            sym = at_idx(keys, i);
            prm = ray_get(sym);
            drop_obj(sym);

            if (is_error(prm))
            {
                res->len = i;
                drop_obj(res);
                drop_obj(keys);
                return prm;
            }

            val = aggr_first(as_list(prm)[0], as_list(prm)[1]);
            drop_obj(prm);

            as_list(res)[i] = val;
        }

        ctx->query_fields = keys;
        ctx->query_values = res;

        timeit_tick("collect fields");

        return NULL_OBJ;
    }

    keys = clone_obj(as_list(ctx->table)[0]);
    l = keys->len;
    res = list(l);

    for (i = 0; i < l; i++)
    {
        sym = at_idx(keys, i);
        prm = ray_get(sym);
        drop_obj(sym);

        if (prm->type == TYPE_FILTERMAP)
        {
            val = filter_collect(as_list(prm)[0], as_list(prm)[1]);
            drop_obj(prm);
        }
        else if (prm->type == TYPE_ENUM)
        {
            val = ray_value(prm);
            drop_obj(prm);
        }
        else
            val = prm;

        if (is_error(val))
        {
            res->len = i;
            drop_obj(res);
            drop_obj(keys);
            return val;
        }

        as_list(res)[i] = val;
    }

    ctx->query_fields = keys;
    ctx->query_values = res;

    timeit_tick("collect fields");

    return NULL_OBJ;
}

obj_p select_build_table(query_ctx_p ctx)
{
    u64_t i, l, m;
    obj_p res, keys, vals;

    switch (ctx->group_fields->type)
    {
    case -TYPE_SYMBOL: // Grouped by one column
        keys = ray_concat(ctx->group_fields, ctx->query_fields);
        l = ctx->query_values->len;
        vals = list(l + 1);
        as_list(vals)[0] = clone_obj(ctx->group_values);

        for (i = 0; i < l; i++)
            as_list(vals)[i + 1] = clone_obj(as_list(ctx->query_values)[i]);

        break;
    case TYPE_SYMBOL: // Grouped by multiple columns
        keys = ray_concat(ctx->group_fields, ctx->query_fields);
        l = ctx->group_values->len;
        m = ctx->query_values->len;
        vals = list(l + m);

        for (i = 0; i < l; i++)
            as_list(vals)[i] = clone_obj(as_list(ctx->group_values)[i]);

        for (i = 0; i < m; i++)
            as_list(vals)[i + l] = clone_obj(as_list(ctx->query_values)[i]);

        break;
    default:
        keys = clone_obj(ctx->query_fields);
        vals = clone_obj(ctx->query_values);
    }

    res = ray_table(keys, vals);
    drop_obj(keys);
    drop_obj(vals);

    timeit_tick("build table");

    return res;
}

obj_p ray_select(obj_p obj)
{
    obj_p res;
    struct query_ctx_t ctx;

    query_ctx_init(&ctx);

    if (obj->type != TYPE_DICT)
        throw(ERR_LENGTH, "'select' takes dict of params");

    if (as_list(obj)[0]->type != TYPE_SYMBOL)
        throw(ERR_LENGTH, "'select' takes dict with symbol keys");

    timeit_span_start("select");

    // Fetch table
    res = select_fetch_table(obj, &ctx);
    if (is_error(res))
        goto cleanup;

    // Mount table columns to a local env
    mount_env(ctx.table);

    // Apply filters
    res = select_apply_filters(obj, &ctx);
    if (is_error(res))
        goto cleanup;

    // Apply groupping
    res = select_apply_groupings(obj, &ctx);
    if (is_error(res))
        goto cleanup;

    // Apply mappings
    res = select_apply_mappings(obj, &ctx);
    if (is_error(res))
        goto cleanup;

    // Collect fields
    res = select_collect_fields(&ctx);
    if (is_error(res))
        goto cleanup;

    // Build result table
    res = select_build_table(&ctx);

cleanup:
    unmount_env(ctx.tablen);
    query_ctx_destroy(&ctx);
    timeit_span_end("select");

    return res;
}
