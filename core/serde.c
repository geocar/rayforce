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

#include "serde.h"
#include "util.h"
#include "format.h"
#include "symbols.h"
#include "string.h"
#include "heap.h"
#include "lambda.h"
#include "cc.h"

/*
 * Returns size (in bytes) that an obj occupy in memory via serialization
 */
u64_t size_obj(obj_t obj)
{
    u64_t i, l, size;

    switch (obj->type)
    {
    case -TYPE_BOOL:
        return sizeof(type_t) + sizeof(bool_t);
    case -TYPE_BYTE:
        return sizeof(type_t) + sizeof(byte_t);
    case -TYPE_I64:
    case -TYPE_TIMESTAMP:
        return sizeof(type_t) + sizeof(i64_t);
    case -TYPE_F64:
        return sizeof(type_t) + sizeof(f64_t);
    case -TYPE_SYMBOL:
        return sizeof(type_t) + strlen(symtostr(obj->i64)) + 1;
    case -TYPE_CHAR:
        return sizeof(type_t) + sizeof(char_t);
    case TYPE_GUID:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(guid_t);
    case TYPE_BOOL:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(bool_t);
    case TYPE_BYTE:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(byte_t);
    case TYPE_I64:
    case TYPE_TIMESTAMP:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(i64_t);
    case TYPE_F64:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(f64_t);
    case TYPE_CHAR:
        return sizeof(type_t) + sizeof(u64_t) + obj->len * sizeof(char_t);
    case TYPE_SYMBOL:
        l = obj->len;
        size = sizeof(type_t) + sizeof(u64_t);
        for (i = 0; i < l; i++)
            size += strlen(symtostr(as_symbol(obj)[i])) + 1;
        return size;
    case TYPE_LIST:
        l = obj->len;
        size = sizeof(type_t) + sizeof(u64_t);
        for (i = 0; i < l; i++)
            size += size_obj(as_list(obj)[i]);
        return size;
    case TYPE_TABLE:
    case TYPE_DICT:
        return sizeof(type_t) + size_obj(as_list(obj)[0]) + size_obj(as_list(obj)[1]);
    case TYPE_LAMBDA:
        return sizeof(type_t) + size_obj(as_lambda(obj)->args) + size_obj(as_lambda(obj)->body);
    default:
        return 0;
    }
}

u64_t save_obj(byte_t *buf, u64_t len, obj_t obj)
{
    u64_t i, l, c;
    str_t s;

    buf[0] = obj->type;
    buf++;

    switch (obj->type)
    {
    case -TYPE_BOOL:
        buf[0] = obj->bool;
        return sizeof(type_t) + sizeof(bool_t);

    case -TYPE_BYTE:
        buf[0] = obj->byte;
        return sizeof(type_t) + sizeof(u8_t);

    case -TYPE_I64:
    case -TYPE_TIMESTAMP:
        memcpy(buf, &obj->i64, sizeof(i64_t));
        return sizeof(type_t) + sizeof(i64_t);

    case -TYPE_SYMBOL:
        s = symtostr(obj->i64);
        strncpy(buf, s, len);

        return sizeof(type_t) + str_len(s, len) + 1;

    case -TYPE_CHAR:
        buf[0] = obj->vchar;
        return sizeof(type_t) + sizeof(char_t);
        // case TYPE_GUID:
        //     memcpy(buf, &obj->guid, sizeof(guid_t));
        //     return sizeof(type_t) + sizeof(guid_t);

    case TYPE_BOOL:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0; i < l; i++)
            buf[i] = as_bool(obj)[i];

        return sizeof(type_t) + sizeof(u64_t) + l * sizeof(bool_t);

    case TYPE_BYTE:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0; i < l; i++)
            buf[i] = as_byte(obj)[i];

        return sizeof(type_t) + sizeof(u64_t) + l * sizeof(byte_t);

    case TYPE_CHAR:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        memcpy(buf, as_string(obj), l);

        return sizeof(type_t) + sizeof(u64_t) + l * sizeof(char_t);

    case TYPE_I64:
    case TYPE_TIMESTAMP:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0; i < l; i++)
            memcpy(buf + i * sizeof(i64_t), &as_i64(obj)[i], sizeof(i64_t));

        return sizeof(type_t) + sizeof(u64_t) + l * sizeof(i64_t);

    case TYPE_F64:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0; i < l; i++)
            memcpy(buf + i * sizeof(f64_t), &as_f64(obj)[i], sizeof(f64_t));

        return sizeof(type_t) + sizeof(u64_t) + l * sizeof(f64_t);

    case TYPE_SYMBOL:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0, c = 0; i < l; i++)
        {
            s = symtostr(as_symbol(obj)[i]);
            c += str_cpy(buf + c, s);
            buf[c] = '\0';
            c++;
        }

        return sizeof(type_t) + sizeof(u64_t) + c;

    case TYPE_LIST:
        l = obj->len;
        memcpy(buf, &l, sizeof(u64_t));
        buf += sizeof(u64_t);
        for (i = 0, c = 0; i < l; i++)
            c += save_obj(buf + c, len, as_list(obj)[i]);

        return sizeof(type_t) + sizeof(u64_t) + c;

    case TYPE_TABLE:
    case TYPE_DICT:
        c = save_obj(buf, len, as_list(obj)[0]);
        c += save_obj(buf + c, len, as_list(obj)[1]);
        return sizeof(type_t) + c;

    case TYPE_LAMBDA:
        c = save_obj(buf, len, as_lambda(obj)->args);
        c += save_obj(buf + c, len, as_lambda(obj)->body);
        return sizeof(type_t) + c;

    default:
        return 0;
    }
}

obj_t ser(obj_t obj)
{
    u64_t size = size_obj(obj);
    obj_t buf;
    header_t *header;

    if (size == 0)
        raise(ERR_NOT_SUPPORTED, "ser: unsupported type: %d", obj->type);

    buf = vector(TYPE_BYTE, sizeof(struct header_t) + size);
    header = (header_t *)as_byte(buf);

    header->prefix = SERDE_PREFIX;
    header->version = RAYFORCE_VERSION;
    header->flags = 0;
    header->reserved = 0;
    header->size = size;

    if (save_obj(as_byte(buf) + sizeof(struct header_t), size, obj) == 0)
    {
        drop(buf);
        raise(ERR_NOT_SUPPORTED, "ser: unsupported type: %d", obj->type);
    }

    return buf;
}

obj_t load_obj(byte_t **buf, u64_t len)
{
    u64_t i, l, c, id;
    obj_t obj, k, v;
    type_t type = **buf;
    (*buf)++;

    switch (type)
    {
    case -TYPE_BOOL:
        obj = bool(**buf);
        (*buf)++;
        return obj;

    case -TYPE_BYTE:
        obj = vbyte(**buf);
        (*buf)++;
        return obj;

    case -TYPE_I64:
    case -TYPE_TIMESTAMP:
        obj = i64(0);
        memcpy(&obj->i64, *buf, sizeof(i64_t));
        *buf += sizeof(i64_t);
        return obj;

    case -TYPE_SYMBOL:
        l = str_len(*buf, len);
        i = intern_symbol(*buf, l);
        obj = symboli64(i);
        *buf += l + 1;
        return obj;

    case -TYPE_CHAR:
        obj = vchar((*buf)[0]);
        (*buf)++;
        return obj;

        // case TYPE_GUID:
        //     obj = guid(buf);
        //     return obj;

    case TYPE_BOOL:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector_bool(l);
        memcpy(as_bool(obj), *buf, l * sizeof(bool_t));
        *buf += l * sizeof(bool_t);
        return obj;

    case TYPE_BYTE:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector_byte(l);
        memcpy(as_byte(obj), *buf, l * sizeof(byte_t));
        *buf += l * sizeof(byte_t);
        return obj;

    case TYPE_CHAR:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = string_from_str(*buf, l);
        *buf += l * sizeof(char_t);
        return obj;

    case TYPE_I64:
    case TYPE_TIMESTAMP:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector_i64(l);
        memcpy(as_i64(obj), *buf, l * sizeof(i64_t));
        *buf += l * sizeof(i64_t);
        return obj;

    case TYPE_F64:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector_f64(l);
        memcpy(as_f64(obj), *buf, l * sizeof(f64_t));
        *buf += l * sizeof(f64_t);
        return obj;

    case TYPE_SYMBOL:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector_symbol(l);
        for (i = 0; i < l; i++)
        {
            c = str_len(*buf, len);
            id = intern_symbol(*buf, c);
            as_symbol(obj)[i] = id;
            *buf += c + 1;
        }
        return obj;

    case TYPE_LIST:
        memcpy(&l, *buf, sizeof(u64_t));
        *buf += sizeof(u64_t);
        obj = vector(TYPE_LIST, l);
        for (i = 0; i < l; i++)
        {
            v = load_obj(buf, len);
            if (is_error(v))
            {
                obj->len = i;
                drop(obj);
                return v;
            }

            as_list(obj)[i] = v;
        }
        return obj;

    case TYPE_DICT:
        k = load_obj(buf, len);

        if (is_error(k))
            return k;

        v = load_obj(buf, len);

        if (is_error(v))
        {
            drop(k);
            return v;
        }

        return dict(k, v);

    case TYPE_LAMBDA:
        k = load_obj(buf, len);

        if (is_error(k))
            return k;

        v = load_obj(buf, len);

        if (is_error(v))
        {
            drop(k);
            return v;
        }

        return cc_compile_lambda("ipc", k, v, NULL);

    default:
        raise(ERR_NOT_SUPPORTED, "load_obj: unsupported type: %d", type);
    }
}

obj_t de_raw(byte_t *buf, u64_t len)
{
    header_t *header = (header_t *)buf;

    if (header->version > RAYFORCE_VERSION)
        return error(ERR_NOT_SUPPORTED, "de: version is higher than supported");

    if (header->size + sizeof(struct header_t) != len)
        return error(ERR_IO, "de: corrupted data in a buffer");

    buf += sizeof(struct header_t);
    return load_obj(&buf, header->size);
}

obj_t de(obj_t buf)
{
    return de_raw(as_byte(buf), buf->len);
}