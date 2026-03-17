#define _POSIX_C_SOURCE 200809L
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Parser state ────────────────────────────────────── */

typedef struct {
    const char* src;
    int         pos;
    int         len;
} JsonParser;

static void jp_skip_ws(JsonParser* p) {
    while (p->pos < p->len && isspace((unsigned char)p->src[p->pos]))
        p->pos++;
}

static int jp_peek(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len) return 0;
    return (unsigned char)p->src[p->pos];
}

static int jp_consume(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len) return 0;
    return (unsigned char)p->src[p->pos++];
}

static IlmaValue jp_parse_value(JsonParser* p);

/* ── String parsing ──────────────────────────────────── */

static IlmaValue jp_parse_string(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len || p->src[p->pos] != '"')
        return ilma_empty_val();
    p->pos++;  /* skip opening " */

    /* estimate buffer size */
    int start = p->pos;
    char* buf = malloc(p->len - start + 4);
    int bi = 0;

    while (p->pos < p->len && p->src[p->pos] != '"') {
        if (p->src[p->pos] == '\\') {
            p->pos++;
            if (p->pos >= p->len) break;
            switch (p->src[p->pos]) {
                case '"':  buf[bi++] = '"';  break;
                case '\\': buf[bi++] = '\\'; break;
                case '/':  buf[bi++] = '/';  break;
                case 'n':  buf[bi++] = '\n'; break;
                case 'r':  buf[bi++] = '\r'; break;
                case 't':  buf[bi++] = '\t'; break;
                case 'b':  buf[bi++] = '\b'; break;
                case 'f':  buf[bi++] = '\f'; break;
                case 'u': {
                    /* Unicode escape: \uXXXX -> UTF-8 */
                    if (p->pos + 4 < p->len) {
                        char hex[5] = {0};
                        memcpy(hex, p->src + p->pos + 1, 4);
                        unsigned int cp = (unsigned int)strtol(hex, NULL, 16);
                        p->pos += 4;
                        if (cp < 0x80) {
                            buf[bi++] = (char)cp;
                        } else if (cp < 0x800) {
                            buf[bi++] = (char)(0xC0 | (cp >> 6));
                            buf[bi++] = (char)(0x80 | (cp & 0x3F));
                        } else {
                            buf[bi++] = (char)(0xE0 | (cp >> 12));
                            buf[bi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                            buf[bi++] = (char)(0x80 | (cp & 0x3F));
                        }
                    }
                    break;
                }
                default:
                    buf[bi++] = p->src[p->pos];
                    break;
            }
        } else {
            buf[bi++] = p->src[p->pos];
        }
        p->pos++;
    }
    if (p->pos < p->len) p->pos++;  /* skip closing " */

    buf[bi] = '\0';
    IlmaValue result = ilma_text(buf);
    free(buf);
    return result;
}

/* ── Number parsing ──────────────────────────────────── */

static IlmaValue jp_parse_number(JsonParser* p) {
    jp_skip_ws(p);
    int start = p->pos;
    int is_float = 0;

    if (p->pos < p->len && p->src[p->pos] == '-') p->pos++;
    while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    if (p->pos < p->len && p->src[p->pos] == '.') {
        is_float = 1;
        p->pos++;
        while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    }
    if (p->pos < p->len && (p->src[p->pos] == 'e' || p->src[p->pos] == 'E')) {
        is_float = 1;
        p->pos++;
        if (p->pos < p->len && (p->src[p->pos] == '+' || p->src[p->pos] == '-')) p->pos++;
        while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) p->pos++;
    }

    char num_buf[64];
    int num_len = p->pos - start;
    if (num_len >= 63) num_len = 63;
    memcpy(num_buf, p->src + start, num_len);
    num_buf[num_len] = '\0';

    if (is_float) {
        return ilma_decimal(atof(num_buf));
    } else {
        return ilma_whole((int64_t)atoll(num_buf));
    }
}

/* ── Array parsing ───────────────────────────────────── */

static IlmaValue jp_parse_array(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len || p->src[p->pos] != '[') return ilma_empty_val();
    p->pos++;  /* skip [ */

    IlmaValue bag = ilma_bag_new();

    jp_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == ']') {
        p->pos++;
        return bag;
    }

    while (p->pos < p->len) {
        IlmaValue item = jp_parse_value(p);
        ilma_bag_add(bag.as_bag, item);

        jp_skip_ws(p);
        if (p->pos >= p->len) break;
        if (p->src[p->pos] == ']') { p->pos++; break; }
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        break;
    }

    return bag;
}

/* ── Object parsing ──────────────────────────────────── */

static IlmaValue jp_parse_object(JsonParser* p) {
    jp_skip_ws(p);
    if (p->pos >= p->len || p->src[p->pos] != '{') return ilma_empty_val();
    p->pos++;  /* skip { */

    IlmaValue nb = ilma_notebook_new();

    jp_skip_ws(p);
    if (p->pos < p->len && p->src[p->pos] == '}') {
        p->pos++;
        return nb;
    }

    while (p->pos < p->len) {
        jp_skip_ws(p);
        if (p->src[p->pos] != '"') break;

        IlmaValue key_val = jp_parse_string(p);
        char* key = key_val.type == ILMA_TEXT ? key_val.as_text : (char*)"";

        jp_skip_ws(p);
        if (p->pos < p->len && p->src[p->pos] == ':') p->pos++;

        IlmaValue val = jp_parse_value(p);
        ilma_notebook_set(nb.as_notebook, key, val);

        jp_skip_ws(p);
        if (p->pos >= p->len) break;
        if (p->src[p->pos] == '}') { p->pos++; break; }
        if (p->src[p->pos] == ',') { p->pos++; continue; }
        break;
    }

    return nb;
}

/* ── Value parser (dispatches to above) ─────────────── */

static IlmaValue jp_parse_value(JsonParser* p) {
    int c = jp_peek(p);
    if (c == 0) return ilma_empty_val();

    if (c == '"') return jp_parse_string(p);
    if (c == '[') return jp_parse_array(p);
    if (c == '{') return jp_parse_object(p);
    if (c == '-' || isdigit(c)) return jp_parse_number(p);

    /* Literals: true, false, null */
    if (strncmp(p->src + p->pos, "true", 4) == 0)  { p->pos += 4; return ilma_yes(); }
    if (strncmp(p->src + p->pos, "false", 5) == 0) { p->pos += 5; return ilma_no();  }
    if (strncmp(p->src + p->pos, "null", 4) == 0)  { p->pos += 4; return ilma_empty_val(); }

    return ilma_empty_val();
}

/* ── Public: parse ───────────────────────────────────── */

IlmaValue ilma_json_parse(IlmaValue json_text) {
    if (json_text.type != ILMA_TEXT || !json_text.as_text)
        return ilma_empty_val();

    JsonParser p;
    p.src = json_text.as_text;
    p.len = (int)strlen(json_text.as_text);
    p.pos = 0;

    return jp_parse_value(&p);
}

/* ── Stringify helpers ───────────────────────────────── */

typedef struct {
    char* buf;
    int   pos;
    int   cap;
} StrBuf;

static void sb_init(StrBuf* sb) {
    sb->cap = 256;
    sb->buf = malloc(sb->cap);
    sb->pos = 0;
    sb->buf[0] = '\0';
}

static void sb_append(StrBuf* sb, const char* s) {
    int len = (int)strlen(s);
    while (sb->pos + len + 1 >= sb->cap) {
        sb->cap *= 2;
        sb->buf = realloc(sb->buf, sb->cap);
    }
    memcpy(sb->buf + sb->pos, s, len);
    sb->pos += len;
    sb->buf[sb->pos] = '\0';
}

static void sb_appendc(StrBuf* sb, char c) {
    if (sb->pos + 2 >= sb->cap) {
        sb->cap *= 2;
        sb->buf = realloc(sb->buf, sb->cap);
    }
    sb->buf[sb->pos++] = c;
    sb->buf[sb->pos] = '\0';
}

static void sb_append_str_escaped(StrBuf* sb, const char* s) {
    sb_appendc(sb, '"');
    while (*s) {
        switch (*s) {
            case '"':  sb_append(sb, "\\\""); break;
            case '\\': sb_append(sb, "\\\\"); break;
            case '\n': sb_append(sb, "\\n");  break;
            case '\r': sb_append(sb, "\\r");  break;
            case '\t': sb_append(sb, "\\t");  break;
            default:   sb_appendc(sb, *s);    break;
        }
        s++;
    }
    sb_appendc(sb, '"');
}

static void stringify_value(StrBuf* sb, IlmaValue v, int indent, int cur_depth);

static void append_indent(StrBuf* sb, int indent, int depth) {
    if (indent <= 0) return;
    sb_appendc(sb, '\n');
    for (int i = 0; i < indent * depth; i++) sb_appendc(sb, ' ');
}

static void stringify_value(StrBuf* sb, IlmaValue v, int indent, int cur_depth) {
    switch (v.type) {
        case ILMA_EMPTY:
            sb_append(sb, "null");
            break;
        case ILMA_TRUTH:
            sb_append(sb, v.as_truth ? "true" : "false");
            break;
        case ILMA_WHOLE: {
            char num[32];
            snprintf(num, sizeof(num), "%lld", (long long)v.as_whole);
            sb_append(sb, num);
            break;
        }
        case ILMA_DECIMAL: {
            char num[64];
            /* Avoid scientific notation for reasonable values */
            snprintf(num, sizeof(num), "%g", v.as_decimal);
            sb_append(sb, num);
            break;
        }
        case ILMA_TEXT:
            sb_append_str_escaped(sb, v.as_text ? v.as_text : "");
            break;
        case ILMA_BAG: {
            sb_appendc(sb, '[');
            if (v.as_bag) {
                for (int i = 0; i < v.as_bag->count; i++) {
                    if (i > 0) {
                        sb_appendc(sb, ',');
                        if (indent > 0) sb_appendc(sb, ' ');
                    }
                    if (indent > 0) append_indent(sb, indent, cur_depth + 1);
                    stringify_value(sb, v.as_bag->items[i], indent, cur_depth + 1);
                }
                if (indent > 0 && v.as_bag->count > 0) append_indent(sb, indent, cur_depth);
            }
            sb_appendc(sb, ']');
            break;
        }
        case ILMA_NOTEBOOK: {
            sb_appendc(sb, '{');
            if (v.as_notebook) {
                int first = 1;
                for (int i = 0; i < v.as_notebook->capacity; i++) {
                    if (!v.as_notebook->entries[i].used) continue;
                    if (!first) {
                        sb_appendc(sb, ',');
                        if (indent > 0) sb_appendc(sb, ' ');
                    }
                    if (indent > 0) append_indent(sb, indent, cur_depth + 1);
                    sb_append_str_escaped(sb, v.as_notebook->entries[i].key);
                    sb_appendc(sb, ':');
                    if (indent > 0) sb_appendc(sb, ' ');
                    stringify_value(sb, v.as_notebook->entries[i].value, indent, cur_depth + 1);
                    first = 0;
                }
                if (indent > 0 && !first) append_indent(sb, indent, cur_depth);
            }
            sb_appendc(sb, '}');
            break;
        }
        case ILMA_OBJECT:
            sb_append(sb, "{}");
            break;
        default:
            sb_append(sb, "null");
            break;
    }
}

IlmaValue ilma_json_stringify(IlmaValue value) {
    StrBuf sb;
    sb_init(&sb);
    stringify_value(&sb, value, 0, 0);
    IlmaValue result = ilma_text(sb.buf);
    free(sb.buf);
    return result;
}

IlmaValue ilma_json_stringify_pretty(IlmaValue value, IlmaValue indent_size) {
    int indent = 2;
    if (indent_size.type == ILMA_WHOLE) indent = (int)indent_size.as_whole;
    if (indent < 1) indent = 2;
    StrBuf sb;
    sb_init(&sb);
    stringify_value(&sb, value, indent, 0);
    IlmaValue result = ilma_text(sb.buf);
    free(sb.buf);
    return result;
}
