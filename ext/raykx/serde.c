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

#include "serde.h"
#include "raykx.h"
#include "k.h"
#include <stdio.h>
#include <string.h>
#include "../../core/util.h"
#include "../../core/ops.h"
#include "../../core/log.h"
#include "../../core/error.h"
#include "../../core/symbols.h"

/*
 * Returns size (in bytes) that an obj occupy in memory via serialization into KDB+ IPC format
 */
i64_t raykx_size_obj(obj_p obj) {
    i64_t size, i, l;

    switch (obj->type) {
        case -TYPE_B8:
            return ISIZEOF(i8_t) + ISIZEOF(b8_t);
        case -TYPE_U8:
            return ISIZEOF(i8_t) + ISIZEOF(u8_t);
        case -TYPE_I16:
            return ISIZEOF(i8_t) + ISIZEOF(i16_t);
        case -TYPE_I32:
        case -TYPE_DATE:
        case -TYPE_TIME:
            return ISIZEOF(i8_t) + ISIZEOF(i32_t);
        case -TYPE_I64:
        case -TYPE_TIMESTAMP:
            return ISIZEOF(i8_t) + ISIZEOF(i64_t);
        case -TYPE_F64:
            return ISIZEOF(i8_t) + ISIZEOF(f64_t);
        case -TYPE_SYMBOL:
            return ISIZEOF(i8_t) + SYMBOL_STRLEN(obj->i64) + 1;
        case -TYPE_C8:
            return ISIZEOF(i8_t) + ISIZEOF(c8_t);
        case -TYPE_GUID:
            return ISIZEOF(i8_t) + ISIZEOF(guid_t);
        case TYPE_GUID:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(guid_t);
        case TYPE_B8:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(i8_t);
        case TYPE_U8:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(u8_t);
        case TYPE_I16:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(i16_t);
        case TYPE_I32:
        case TYPE_DATE:
        case TYPE_TIME:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(i32_t);
        case TYPE_I64:
        case TYPE_TIMESTAMP:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(i64_t);
        case TYPE_F64:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(f64_t);
        case TYPE_C8:
            return ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t) + obj->len * ISIZEOF(c8_t);
        case TYPE_SYMBOL:
            l = obj->len;
            size = ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t);
            for (i = 0; i < l; i++)
                size += SYMBOL_STRLEN(AS_SYMBOL(obj)[i]) + 1;
            return size;
        case TYPE_LIST:
            l = obj->len;
            size = ISIZEOF(i8_t) + 1 + ISIZEOF(u32_t);
            for (i = 0; i < l; i++)
                size += raykx_size_obj(AS_LIST(obj)[i]);
            return size;
        case TYPE_TABLE:
        case TYPE_DICT:
            return ISIZEOF(i8_t) + raykx_size_obj(AS_LIST(obj)[0]) + raykx_size_obj(AS_LIST(obj)[1]);
        case TYPE_ERR:
            return ISIZEOF(i8_t) + AS_ERROR(obj)->msg->len + 1;
        case TYPE_NULL:
            return ISIZEOF(i8_t) + 1 + sizeof(i32_t);
        // // case TYPE_LAMBDA:
        // //     return ISIZEOF(i8_t) + size_obj(AS_LAMBDA(obj)->args) + size_obj(AS_LAMBDA(obj)->body);
        // case TYPE_UNARY:
        // case TYPE_BINARY:
        // case TYPE_VARY:
        //     return ISIZEOF(i8_t) + strlen(env_get_internal_name(obj)) + 1;
        // case TYPE_NULL:
        //     return ISIZEOF(i8_t);
        // case TYPE_ERR:
        //     return ISIZEOF(i8_t) + ISIZEOF(i8_t) + size_obj(AS_ERROR(obj)->msg);
        default:
            return 0;
    }
}

// Table mapping RayKX types to KDB+ types
static const i8_t raykx_type_to_k_table[128] = {
    [TYPE_TIMESTAMP] = KP,  // Timestamp
    [TYPE_I64] = KJ,        // Long
    [TYPE_F64] = KF,        // Float
    [TYPE_I32] = KI,        // Int
    [TYPE_I16] = KH,        // Short
    [TYPE_U8] = KG,         // Byte
    [TYPE_B8] = KB,         // Boolean
    [TYPE_C8] = KC,         // Char
    [TYPE_SYMBOL] = KS,     // Symbol
    [TYPE_GUID] = UU,       // GUID
    [TYPE_DATE] = KD,       // Date
    [TYPE_TIME] = KT,       // Time
    [TYPE_LIST] = 0,        // List
    [TYPE_TABLE] = XT,      // Table
    [TYPE_DICT] = XD,       // Dictionary
    [TYPE_NULL] = 0,        // Null
    [TYPE_ERR] = -128,      // Error
};

#define RAYKX_TYPE_TO_K(t) (raykx_type_to_k_table[(t < 0 ? -t : t)] * (t < 0 ? -1 : 1))

#define RAYKX_SER_ATOM(b, o, t)          \
    ({                                   \
        memcpy(b, &o->t, sizeof(t##_t)); \
        ISIZEOF(i8_t) + ISIZEOF(t##_t);  \
    })

#define RAYKX_SER_VEC(b, o, t)                 \
    ({                                         \
        b[0] = 0;                              \
        b++;                                   \
        l = (i32_t)obj->len;                   \
        memcpy(b, &l, sizeof(i32_t));          \
        b += sizeof(i32_t);                    \
        n = obj->len * ISIZEOF(t##_t);         \
        memcpy(b, o->raw, n);                  \
        ISIZEOF(i8_t) + 1 + sizeof(i32_t) + n; \
    })

i64_t raykx_ser_obj(u8_t *buf, obj_p obj) {
    i32_t i, n, l;
    u8_t *b;
    str_p str;

    buf[0] = RAYKX_TYPE_TO_K(obj->type);
    buf++;
    b = buf;

    switch (obj->type) {
        case -TYPE_B8:
            return RAYKX_SER_ATOM(b, obj, u8);
        case -TYPE_U8:
            return RAYKX_SER_ATOM(b, obj, u8);
        case -TYPE_C8:
            return RAYKX_SER_ATOM(b, obj, c8);
        case -TYPE_I16:
            return RAYKX_SER_ATOM(b, obj, i16);
        case -TYPE_I32:
        case -TYPE_DATE:
        case -TYPE_TIME:
            return RAYKX_SER_ATOM(b, obj, i32);
        case -TYPE_I64:
        case -TYPE_TIMESTAMP:
            return RAYKX_SER_ATOM(b, obj, i64);
        case -TYPE_F64:
            return RAYKX_SER_ATOM(b, obj, f64);
        case -TYPE_SYMBOL:
            str = str_from_symbol(obj->i64);
            l = SYMBOL_STRLEN(obj->i64) + 1;
            memcpy(buf, str, l);
            return ISIZEOF(i8_t) + l;
        case -TYPE_GUID:
            memcpy(buf, obj->raw, sizeof(guid_t));
            return ISIZEOF(i8_t) + ISIZEOF(guid_t);
        case TYPE_C8:
        case TYPE_B8:
        case TYPE_U8:
            return RAYKX_SER_VEC(buf, obj, c8);
        case TYPE_I16:
            return RAYKX_SER_VEC(buf, obj, i16);
        case TYPE_I32:
        case TYPE_DATE:
        case TYPE_TIME:
            return RAYKX_SER_VEC(buf, obj, i32);
        case TYPE_I64:
        case TYPE_TIMESTAMP:
            return RAYKX_SER_VEC(buf, obj, i64);
        case TYPE_F64:
            return RAYKX_SER_VEC(buf, obj, f64);
        case TYPE_SYMBOL:
            buf[0] = 0;  // attrs
            buf++;
            l = (i32_t)obj->len;
            memcpy(buf, &l, sizeof(i32_t));
            buf += sizeof(i32_t);
            b = buf;
            for (i = 0, n = 0; i < obj->len; i++) {
                str = str_from_symbol(AS_SYMBOL(obj)[i]);
                n = SYMBOL_STRLEN(AS_SYMBOL(obj)[i]) + 1;
                memcpy(b, str, n);
                b += n;
            }
            return ISIZEOF(i8_t) + 1 + sizeof(i32_t) + (b - buf);
        case TYPE_GUID:
            buf[0] = 0;  // attrs
            buf++;
            l = (i32_t)obj->len;
            memcpy(buf, &l, sizeof(i32_t));
            buf += sizeof(i32_t);
            n = obj->len * ISIZEOF(guid_t);
            memcpy(buf, obj->raw, n);
            return ISIZEOF(i8_t) + 1 + sizeof(i32_t) + n;
        case TYPE_NULL:
            buf[0] = 0;  // attrs
            buf++;
            memset(buf, 0, sizeof(i32_t));
            return ISIZEOF(i8_t) + 1 + ISIZEOF(i32_t);
        case TYPE_LIST:
            buf[0] = 0;  // attrs
            buf++;
            l = (i32_t)obj->len;
            memcpy(buf, &l, sizeof(i32_t));
            buf += sizeof(i32_t);
            b = buf;
            for (i = 0; i < obj->len; i++)
                b += raykx_ser_obj(b, AS_LIST(obj)[i]);
            return ISIZEOF(i8_t) + 1 + sizeof(i32_t) + (b - buf);
        case TYPE_TABLE:
        case TYPE_DICT:
            b += raykx_ser_obj(b, AS_LIST(obj)[0]);
            b += raykx_ser_obj(b, AS_LIST(obj)[1]);
            return ISIZEOF(i8_t) + (b - buf);
        case TYPE_ERR:
            l = AS_ERROR(obj)->msg->len;
            if (l == 0) {
                buf[0] = 0;
                return ISIZEOF(i8_t) + 1;
            }

            if (AS_C8(AS_ERROR(obj)->msg)[l - 1] == '\0')
                l--;

            memcpy(buf, AS_C8(AS_ERROR(obj)->msg), l);
            buf += l;
            return ISIZEOF(i8_t) + l + 1;
        default:
            return 0;
    }
}

obj_p raykx_load_obj(u8_t *buf, i64_t *len) {
    i64_t l;
    obj_p k, v, obj;
    i8_t type;

    if (*len == 0)
        return NULL_OBJ;

    type = *buf;
    buf++;
    len--;

    switch (type) {
        case -KB:
            obj = b8(buf[0]);
            buf++;
            len--;
            return obj;
        case -KC:
            obj = c8(buf[0]);
            buf++;
            len--;
            return obj;
        case -KG:
            obj = u8(buf[0]);
            buf++;
            len--;
            return obj;
        case -KH:
            obj = i16(0);
            memcpy(&obj->i16, buf, sizeof(i16_t));
            buf += sizeof(i16_t);
            len -= sizeof(i16_t);
            return obj;
        case -KI:
            obj = i32(0);
            memcpy(&obj->i32, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            return obj;
        case -KJ:
            obj = i64(0);
            memcpy(&obj->i64, buf, sizeof(i64_t));
            buf += sizeof(i64_t);
            len -= sizeof(i64_t);
            return obj;
        case -KS:
            l = strlen((lit_p)buf);
            obj = symbol((lit_p)buf, l);
            buf += l + 1;
            len -= l + 1;
            return obj;
        case -KF:
            obj = f64(0);
            memcpy(&obj->f64, buf, sizeof(f64_t));
            buf += sizeof(f64_t);
            len -= sizeof(f64_t);
            return obj;
        case -KZ:
            obj = NULL_OBJ;
            return obj;
        case KC:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = C8(l);
            memcpy(obj->raw, buf, l);
            buf += l;
            len -= l;
            return obj;
        case KG:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = U8(l);
            memcpy(obj->raw, buf, l);
            buf += l;
            len -= l;
            return obj;
        case KH:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = I16(l);
            memcpy(obj->raw, buf, l * ISIZEOF(i16_t));
            buf += l;
            len -= l;
            return obj;
        case KI:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = I32(l);
            memcpy(obj->raw, buf, l * ISIZEOF(i32_t));
            buf += l;
            len -= l;
            return obj;
        case KJ:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = I64(l);
            memcpy(obj->raw, buf, l * ISIZEOF(i64_t));
            buf += l;
            len -= l;
            return obj;
        case KF:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = F64(l);
            memcpy(obj->raw, buf, l * ISIZEOF(f64_t));
            buf += l;
            len -= l;
            return obj;
        case KT:
            buf++;  // attrs
            memcpy(&l, buf, sizeof(i32_t));
            buf += sizeof(i32_t);
            len -= sizeof(i32_t);
            obj = TIME(l);
            memcpy(obj->raw, buf, l * ISIZEOF(i32_t));
            buf += l;
            len -= l;
            return obj;
        case XD:
            l = *len;
            k = raykx_load_obj(buf, len);
            buf += l - *len;
            v = raykx_load_obj(buf, len);
            return dict(k, v);
        default:
            return NULL_OBJ;
    }
}

obj_p raykx_des_obj(u8_t *buf, i64_t len) {
    i64_t l;
    raykx_header_p header;

    // Check if buffer is large enough to contain a header
    if (len < ISIZEOF(struct raykx_header_t))
        return error_str(ERR_IO, "raykx_des_obj: buffer too small to contain header");

    header = (raykx_header_p)buf;
    LOG_DEBUG("Deserializing object of size %lld", header->size);

    // Check for reasonable size values
    if (header->size > 1000000000)  // 1GB max size
        return error_str(ERR_IO, "raykx_des_obj: unreasonable size in header, possible corruption");

    // Check for reasonable size values
    if (header->size != len)
        return error_str(ERR_IO, "raykx_des_obj: corrupted data in a buffer");

    buf += sizeof(struct raykx_header_t);
    l = header->size;

    return raykx_load_obj(buf, &l);
}
