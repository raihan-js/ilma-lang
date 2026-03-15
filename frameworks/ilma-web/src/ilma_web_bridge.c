/*
 * ilma_web_bridge.c — Bridge between the ILMA runtime and IlmaWeb HTTP
 *
 * Translates IlmaValue types to/from raw C strings and HTTP responses.
 * The JSON serialiser handles all ILMA types recursively: text, whole,
 * decimal, truth (yes/no -> true/false), empty (null), bag (array),
 * and notebook (object).
 */

#include "ilma_web_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Global state ─────────────────────────────────────────── */

IlmaHttpServer   ilma_web_server;
IlmaHttpRequest* ilma_web_current_request = NULL;

/* ═════════════════════════════════════════════════════════════
 *  Server lifecycle
 * ═════════════════════════════════════════════════════════════ */

void ilma_web_init(int port) {
    ilma_http_init(&ilma_web_server, port);
}

void ilma_web_add_route(const char* path, IlmaRouteHandler handler) {
    ilma_http_route(&ilma_web_server, "*", path, handler);
}

void ilma_web_start(void) {
    ilma_http_start(&ilma_web_server);
}

/* ═════════════════════════════════════════════════════════════
 *  Request data access  (return IlmaValue)
 * ═════════════════════════════════════════════════════════════ */

IlmaValue ilma_web_query(const char* key) {
    if (!ilma_web_current_request) return ilma_empty_val();
    const char* val = ilma_http_query_get(ilma_web_current_request, key);
    if (val) return ilma_text(val);
    return ilma_empty_val();
}

IlmaValue ilma_web_body_text(void) {
    if (!ilma_web_current_request) return ilma_empty_val();
    return ilma_text(ilma_web_current_request->body);
}

IlmaValue ilma_web_header(const char* key) {
    if (!ilma_web_current_request) return ilma_empty_val();
    const char* val = ilma_http_header_get(ilma_web_current_request, key);
    if (val) return ilma_text(val);
    return ilma_empty_val();
}

IlmaValue ilma_web_method(void) {
    if (!ilma_web_current_request) return ilma_empty_val();
    return ilma_text(ilma_web_current_request->method);
}

IlmaValue ilma_web_path(void) {
    if (!ilma_web_current_request) return ilma_empty_val();
    return ilma_text(ilma_web_current_request->path);
}

/* ═════════════════════════════════════════════════════════════
 *  JSON serialisation of IlmaValue
 * ═════════════════════════════════════════════════════════════ */

/*
 * We build JSON into a dynamically-grown buffer.  The helpers below
 * append to *buf (realloc as needed) and update *len / *cap.
 */

static void json_append(char** buf, size_t* len, size_t* cap,
                         const char* str) {
    size_t slen = strlen(str);
    while (*len + slen + 1 > *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }
    memcpy(*buf + *len, str, slen);
    *len += slen;
    (*buf)[*len] = '\0';
}

static void json_append_char(char** buf, size_t* len, size_t* cap, char c) {
    if (*len + 2 > *cap) {
        *cap *= 2;
        *buf = realloc(*buf, *cap);
    }
    (*buf)[*len] = c;
    (*len)++;
    (*buf)[*len] = '\0';
}

/* Escape a C string for JSON (handles \, ", \n, \r, \t) */
static void json_append_escaped(char** buf, size_t* len, size_t* cap,
                                 const char* s) {
    json_append_char(buf, len, cap, '"');
    for (; *s; s++) {
        switch (*s) {
            case '"':  json_append(buf, len, cap, "\\\""); break;
            case '\\': json_append(buf, len, cap, "\\\\"); break;
            case '\n': json_append(buf, len, cap, "\\n");  break;
            case '\r': json_append(buf, len, cap, "\\r");  break;
            case '\t': json_append(buf, len, cap, "\\t");  break;
            default:   json_append_char(buf, len, cap, *s); break;
        }
    }
    json_append_char(buf, len, cap, '"');
}

/* Forward declaration for recursive serialisation */
static void ilma_value_to_json(IlmaValue v, char** buf, size_t* len,
                                size_t* cap);

static void ilma_value_to_json(IlmaValue v, char** buf, size_t* len,
                                size_t* cap) {
    char tmp[64];
    switch (v.type) {
        case ILMA_TEXT:
            json_append_escaped(buf, len, cap, v.as_text ? v.as_text : "");
            break;

        case ILMA_WHOLE:
            snprintf(tmp, sizeof(tmp), "%ld", (long)v.as_whole);
            json_append(buf, len, cap, tmp);
            break;

        case ILMA_DECIMAL:
            snprintf(tmp, sizeof(tmp), "%g", v.as_decimal);
            json_append(buf, len, cap, tmp);
            break;

        case ILMA_TRUTH:
            json_append(buf, len, cap, v.as_truth ? "true" : "false");
            break;

        case ILMA_EMPTY:
            json_append(buf, len, cap, "null");
            break;

        case ILMA_BAG:
            json_append_char(buf, len, cap, '[');
            if (v.as_bag) {
                for (int i = 0; i < v.as_bag->count; i++) {
                    if (i > 0) json_append_char(buf, len, cap, ',');
                    ilma_value_to_json(v.as_bag->items[i], buf, len, cap);
                }
            }
            json_append_char(buf, len, cap, ']');
            break;

        case ILMA_NOTEBOOK:
            json_append_char(buf, len, cap, '{');
            if (v.as_notebook) {
                int first = 1;
                for (int i = 0; i < v.as_notebook->capacity; i++) {
                    if (!v.as_notebook->entries[i].used) continue;
                    if (!first) json_append_char(buf, len, cap, ',');
                    first = 0;
                    json_append_escaped(buf, len, cap,
                                        v.as_notebook->entries[i].key);
                    json_append_char(buf, len, cap, ':');
                    ilma_value_to_json(v.as_notebook->entries[i].value,
                                       buf, len, cap);
                }
            }
            json_append_char(buf, len, cap, '}');
            break;

        case ILMA_OBJECT:
            /* Serialize object fields as JSON object */
            json_append_char(buf, len, cap, '{');
            if (v.as_object && v.as_object->fields) {
                int first = 1;
                IlmaBook* f = v.as_object->fields;
                for (int i = 0; i < f->capacity; i++) {
                    if (!f->entries[i].used) continue;
                    if (!first) json_append_char(buf, len, cap, ',');
                    first = 0;
                    json_append_escaped(buf, len, cap, f->entries[i].key);
                    json_append_char(buf, len, cap, ':');
                    ilma_value_to_json(f->entries[i].value, buf, len, cap);
                }
            }
            json_append_char(buf, len, cap, '}');
            break;

        default:
            json_append(buf, len, cap, "null");
            break;
    }
}

/*
 * Convert an IlmaValue to a newly-allocated JSON string.
 * Caller must free() the result.
 */
static char* ilma_to_json_string(IlmaValue v) {
    size_t cap = 256;
    size_t len = 0;
    char*  buf = malloc(cap);
    buf[0] = '\0';
    ilma_value_to_json(v, &buf, &len, &cap);
    return buf;
}

/* ═════════════════════════════════════════════════════════════
 *  Response builders  (return IlmaHttpResponse)
 * ═════════════════════════════════════════════════════════════ */

IlmaHttpResponse ilma_web_html_response(IlmaValue html_val) {
    char* s = ilma_to_string(html_val);
    IlmaHttpResponse res = ilma_http_html(200, s);
    free(s);
    return res;
}

IlmaHttpResponse ilma_web_json_response(IlmaValue data) {
    char* json = ilma_to_json_string(data);
    IlmaHttpResponse res = ilma_http_json(200, json);
    free(json);
    return res;
}

IlmaHttpResponse ilma_web_text_response(IlmaValue text_val) {
    char* s = ilma_to_string(text_val);
    IlmaHttpResponse res = ilma_http_text(200, s);
    free(s);
    return res;
}
