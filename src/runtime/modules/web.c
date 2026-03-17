#define _POSIX_C_SOURCE 200809L
#include "web.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef int socklen_t;
#  define CLOSESOCK(s) closesocket(s)
   static void net_init(void) {
       WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
   }
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
   typedef int SOCKET;
#  define INVALID_SOCKET (-1)
#  define CLOSESOCK(s)   close(s)
   static void net_init(void) {}
#endif

/* ── Limits ──────────────────────────────────────────────── */
#define WEB_MAX_HEADERS   64
#define WEB_MAX_QUERY     64
#define WEB_MAX_PATH      2048
#define WEB_MAX_BODY      (256 * 1024)   /* 256 KB */
#define WEB_RECV_BUF      (16 * 1024)    /* 16 KB receive buffer */

/* ── Parsed request ──────────────────────────────────────── */
typedef struct {
    char key[256];
    char val[1024];
} WebPair;

typedef struct {
    char     method[16];
    char     path[WEB_MAX_PATH];
    WebPair  query[WEB_MAX_QUERY];
    int      query_count;
    WebPair  headers[WEB_MAX_HEADERS];
    int      header_count;
    char*    body;
    int      body_len;
} WebRequest;

/* ── Response content-type tag ───────────────────────────── */
typedef enum {
    CT_HTML,
    CT_JSON,
    CT_TEXT
} WebCType;

/* ── Module-level globals ────────────────────────────────── */
static SOCKET     g_listen_fd  = INVALID_SOCKET;
static SOCKET     g_client_fd  = INVALID_SOCKET;
static int        g_port       = 0;
static WebRequest g_req;
static WebCType   g_ctype      = CT_HTML;
static int        g_status     = 200;

/* ── String helpers ──────────────────────────────────────── */

static void str_lower(char* s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* URL-decode src into dst (dst must be at least strlen(src)+1 bytes) */
static void url_decode(const char* src, char* dst, size_t dst_sz) {
    size_t i = 0;
    while (*src && i + 1 < dst_sz) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], 0 };
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

/* ── Query-string parser ─────────────────────────────────── */

static void parse_query(const char* qs, WebPair* pairs, int* count, int max) {
    *count = 0;
    if (!qs || !*qs) return;
    char buf[WEB_MAX_PATH];
    strncpy(buf, qs, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char* tok = buf;
    while (tok && *tok && *count < max) {
        char* amp = strchr(tok, '&');
        if (amp) *amp = '\0';
        char* eq = strchr(tok, '=');
        if (eq) {
            *eq = '\0';
            url_decode(tok, pairs[*count].key, sizeof(pairs[*count].key));
            url_decode(eq + 1, pairs[*count].val, sizeof(pairs[*count].val));
        } else {
            url_decode(tok, pairs[*count].key, sizeof(pairs[*count].key));
            pairs[*count].val[0] = '\0';
        }
        (*count)++;
        tok = amp ? amp + 1 : NULL;
    }
}

/* ── HTTP request parser ─────────────────────────────────── */

static int parse_request(const char* raw, int raw_len) {
    (void)raw_len;
    memset(&g_req, 0, sizeof(g_req));

    /* Request line */
    const char* p = raw;
    const char* end = p + raw_len;

    /* Method */
    const char* sp = memchr(p, ' ', (size_t)(end - p));
    if (!sp) return 0;
    int mlen = (int)(sp - p);
    if (mlen >= (int)sizeof(g_req.method)) mlen = (int)sizeof(g_req.method) - 1;
    memcpy(g_req.method, p, (size_t)mlen);
    g_req.method[mlen] = '\0';
    p = sp + 1;

    /* Path + query */
    sp = memchr(p, ' ', (size_t)(end - p));
    if (!sp) return 0;
    int full_len = (int)(sp - p);
    char full_path[WEB_MAX_PATH];
    if (full_len >= WEB_MAX_PATH) full_len = WEB_MAX_PATH - 1;
    memcpy(full_path, p, (size_t)full_len);
    full_path[full_len] = '\0';
    p = sp + 1;

    /* Split path and query string */
    char* qmark = strchr(full_path, '?');
    if (qmark) {
        *qmark = '\0';
        url_decode(full_path, g_req.path, sizeof(g_req.path));
        parse_query(qmark + 1, g_req.query, &g_req.query_count, WEB_MAX_QUERY);
    } else {
        url_decode(full_path, g_req.path, sizeof(g_req.path));
    }

    /* Skip rest of request line */
    const char* crlf = strstr(p, "\r\n");
    if (!crlf) return 0;
    p = crlf + 2;

    /* Headers */
    while (p < end - 1) {
        if (p[0] == '\r' && p[1] == '\n') { p += 2; break; } /* blank line */
        const char* nl = strstr(p, "\r\n");
        if (!nl) break;
        const char* colon = memchr(p, ':', (size_t)(nl - p));
        if (colon && g_req.header_count < WEB_MAX_HEADERS) {
            int klen = (int)(colon - p);
            int vlen = (int)(nl - colon - 2); /* skip ": " */
            const char* vstart = colon + 1;
            while (vstart < nl && *vstart == ' ') vstart++;
            vlen = (int)(nl - vstart);
            if (klen >= (int)sizeof(g_req.headers[0].key)) klen = (int)sizeof(g_req.headers[0].key) - 1;
            if (vlen >= (int)sizeof(g_req.headers[0].val)) vlen = (int)sizeof(g_req.headers[0].val) - 1;
            memcpy(g_req.headers[g_req.header_count].key, p, (size_t)klen);
            g_req.headers[g_req.header_count].key[klen] = '\0';
            str_lower(g_req.headers[g_req.header_count].key);
            memcpy(g_req.headers[g_req.header_count].val, vstart, (size_t)vlen);
            g_req.headers[g_req.header_count].val[vlen] = '\0';
            g_req.header_count++;
        }
        p = nl + 2;
    }

    /* Body */
    int remaining = (int)(end - p);
    if (remaining > 0) {
        int blen = remaining < WEB_MAX_BODY ? remaining : WEB_MAX_BODY;
        g_req.body = malloc((size_t)blen + 1);
        if (g_req.body) {
            memcpy(g_req.body, p, (size_t)blen);
            g_req.body[blen] = '\0';
            g_req.body_len = blen;
        }
    }

    return 1;
}

/* ── Read a full HTTP request from the socket ─────────────── */

static char* recv_request(SOCKET fd, int* out_len) {
    char*  buf  = malloc(WEB_RECV_BUF);
    int    cap  = WEB_RECV_BUF;
    int    used = 0;
    if (!buf) return NULL;

    while (1) {
        if (used + 1 >= cap) {
            cap *= 2;
            char* nb = realloc(buf, (size_t)cap);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        int n = (int)recv(fd, buf + used, (size_t)(cap - used - 1), 0);
        if (n <= 0) break;
        used += n;
        buf[used] = '\0';
        /* Check if headers are fully received */
        if (strstr(buf, "\r\n\r\n")) {
            /* Check Content-Length to see if body is complete */
            const char* cl = strstr(buf, "Content-Length:");
            if (!cl) cl = strstr(buf, "content-length:");
            if (cl) {
                int clen = atoi(cl + 15);
                const char* body_start = strstr(buf, "\r\n\r\n");
                if (body_start) {
                    int header_end = (int)(body_start - buf) + 4;
                    if (used - header_end >= clen) break;
                }
            } else {
                break; /* no body expected */
            }
        }
    }
    *out_len = used;
    return buf;
}

/* ── JSON serializer ──────────────────────────────────────── */

static void json_append_str(char** out, int* len, int* cap, const char* s) {
    while (*s) {
        if (*len + 8 >= *cap) {
            *cap *= 2;
            *out = realloc(*out, (size_t)*cap);
        }
        unsigned char c = (unsigned char)*s++;
        if (c == '"')       { (*out)[(*len)++] = '\\'; (*out)[(*len)++] = '"'; }
        else if (c == '\\') { (*out)[(*len)++] = '\\'; (*out)[(*len)++] = '\\'; }
        else if (c == '\n') { (*out)[(*len)++] = '\\'; (*out)[(*len)++] = 'n'; }
        else if (c == '\r') { (*out)[(*len)++] = '\\'; (*out)[(*len)++] = 'r'; }
        else if (c == '\t') { (*out)[(*len)++] = '\\'; (*out)[(*len)++] = 't'; }
        else                { (*out)[(*len)++] = (char)c; }
    }
}

static void json_append(char** out, int* len, int* cap, const char* s) {
    int slen = (int)strlen(s);
    while (*len + slen + 4 >= *cap) {
        *cap *= 2;
        *out = realloc(*out, (size_t)*cap);
    }
    memcpy(*out + *len, s, (size_t)slen);
    *len += slen;
}

static void json_serialize(IlmaValue val, char** out, int* len, int* cap);

static void json_serialize(IlmaValue val, char** out, int* len, int* cap) {
    char numbuf[64];
    switch (val.type) {
        case ILMA_TEXT:
            json_append(out, len, cap, "\"");
            json_append_str(out, len, cap, val.as_text ? val.as_text : "");
            json_append(out, len, cap, "\"");
            break;
        case ILMA_WHOLE:
            snprintf(numbuf, sizeof(numbuf), "%lld", (long long)val.as_whole);
            json_append(out, len, cap, numbuf);
            break;
        case ILMA_DECIMAL:
            snprintf(numbuf, sizeof(numbuf), "%.10g", val.as_decimal);
            json_append(out, len, cap, numbuf);
            break;
        case ILMA_TRUTH:
            json_append(out, len, cap, val.as_truth ? "true" : "false");
            break;
        case ILMA_EMPTY:
            json_append(out, len, cap, "null");
            break;
        case ILMA_BAG:
            json_append(out, len, cap, "[");
            if (val.as_bag) {
                for (int i = 0; i < val.as_bag->count; i++) {
                    if (i > 0) json_append(out, len, cap, ",");
                    json_serialize(val.as_bag->items[i], out, len, cap);
                }
            }
            json_append(out, len, cap, "]");
            break;
        case ILMA_NOTEBOOK:
            json_append(out, len, cap, "{");
            if (val.as_notebook) {
                int first = 1;
                for (int i = 0; i < val.as_notebook->capacity; i++) {
                    if (!val.as_notebook->entries[i].used) continue;
                    if (!first) json_append(out, len, cap, ",");
                    first = 0;
                    json_append(out, len, cap, "\"");
                    json_append_str(out, len, cap, val.as_notebook->entries[i].key);
                    json_append(out, len, cap, "\":");
                    json_serialize(val.as_notebook->entries[i].value, out, len, cap);
                }
            }
            json_append(out, len, cap, "}");
            break;
        case ILMA_OBJECT:
            /* Serialize object fields as JSON object */
            json_append(out, len, cap, "{");
            if (val.as_object && val.as_object->fields) {
                int first = 1;
                IlmaBook* b = val.as_object->fields;
                for (int i = 0; i < b->capacity; i++) {
                    if (!b->entries[i].used) continue;
                    if (!first) json_append(out, len, cap, ",");
                    first = 0;
                    json_append(out, len, cap, "\"");
                    json_append_str(out, len, cap, b->entries[i].key);
                    json_append(out, len, cap, "\":");
                    json_serialize(b->entries[i].value, out, len, cap);
                }
            }
            json_append(out, len, cap, "}");
            break;
    }
}

static char* ilma_value_to_json(IlmaValue val) {
    int   cap = 256;
    int   len = 0;
    char* out = malloc((size_t)cap);
    if (!out) return strdup("null");
    json_serialize(val, &out, &len, &cap);
    out[len] = '\0';
    return out;
}

/* ── Send HTTP response ──────────────────────────────────── */

static void send_response(SOCKET fd, int status, const char* ctype,
                          const char* body, int blen) {
    const char* status_text = "OK";
    if      (status == 201) status_text = "Created";
    else if (status == 204) status_text = "No Content";
    else if (status == 301) status_text = "Moved Permanently";
    else if (status == 302) status_text = "Found";
    else if (status == 400) status_text = "Bad Request";
    else if (status == 401) status_text = "Unauthorized";
    else if (status == 403) status_text = "Forbidden";
    else if (status == 404) status_text = "Not Found";
    else if (status == 405) status_text = "Method Not Allowed";
    else if (status == 500) status_text = "Internal Server Error";

    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, ctype, blen);
    send(fd, header, (size_t)hlen, 0);
    if (blen > 0) send(fd, body, (size_t)blen, 0);
}

/* ═══════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════ */

IlmaValue ilma_web_listen(IlmaValue port) {
    net_init();

    int p = (port.type == ILMA_WHOLE) ? (int)port.as_whole :
            (port.type == ILMA_DECIMAL) ? (int)port.as_decimal : 3000;
    g_port = p;

    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_fd == INVALID_SOCKET) {
        fprintf(stderr, "web: socket() failed: %s\n", strerror(errno));
        return ilma_empty_val();
    }

    int yes = 1;
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)p);

    if (bind(g_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "web: bind() failed on port %d: %s\n", p, strerror(errno));
        CLOSESOCK(g_listen_fd);
        g_listen_fd = INVALID_SOCKET;
        return ilma_empty_val();
    }

    if (listen(g_listen_fd, 10) < 0) {
        fprintf(stderr, "web: listen() failed: %s\n", strerror(errno));
        CLOSESOCK(g_listen_fd);
        g_listen_fd = INVALID_SOCKET;
        return ilma_empty_val();
    }

    printf("ILMA web server running on http://localhost:%d\n", p);
    fflush(stdout);
    return ilma_empty_val();
}

IlmaValue ilma_web_accept(void) {
    if (g_listen_fd == INVALID_SOCKET) return ilma_no();

    /* Close previous client */
    if (g_client_fd != INVALID_SOCKET) {
        CLOSESOCK(g_client_fd);
        g_client_fd = INVALID_SOCKET;
    }
    /* Free previous body */
    if (g_req.body) { free(g_req.body); g_req.body = NULL; }

    /* Reset response defaults */
    g_status = 200;
    g_ctype  = CT_HTML;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    g_client_fd = accept(g_listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (g_client_fd == INVALID_SOCKET) return ilma_no();

    int raw_len = 0;
    char* raw = recv_request(g_client_fd, &raw_len);
    if (!raw || raw_len == 0) {
        if (raw) free(raw);
        CLOSESOCK(g_client_fd);
        g_client_fd = INVALID_SOCKET;
        return ilma_web_accept(); /* skip bad requests silently */
    }

    parse_request(raw, raw_len);
    free(raw);
    return ilma_yes();
}

IlmaValue ilma_web_path(void) {
    return ilma_text(g_req.path[0] ? g_req.path : "/");
}

IlmaValue ilma_web_method(void) {
    return ilma_text(g_req.method[0] ? g_req.method : "GET");
}

IlmaValue ilma_web_query(IlmaValue key) {
    const char* k = (key.type == ILMA_TEXT && key.as_text) ? key.as_text : "";
    for (int i = 0; i < g_req.query_count; i++) {
        if (strcmp(g_req.query[i].key, k) == 0)
            return ilma_text(g_req.query[i].val);
    }
    return ilma_empty_val();
}

IlmaValue ilma_web_body(void) {
    if (g_req.body && g_req.body_len > 0)
        return ilma_text(g_req.body);
    return ilma_empty_val();
}

IlmaValue ilma_web_header(IlmaValue name) {
    if (name.type != ILMA_TEXT || !name.as_text) return ilma_empty_val();
    char lower_name[256];
    strncpy(lower_name, name.as_text, sizeof(lower_name) - 1);
    lower_name[sizeof(lower_name) - 1] = '\0';
    str_lower(lower_name);
    for (int i = 0; i < g_req.header_count; i++) {
        if (strcmp(g_req.headers[i].key, lower_name) == 0)
            return ilma_text(g_req.headers[i].val);
    }
    return ilma_empty_val();
}

IlmaValue ilma_web_html(IlmaValue content) {
    g_ctype = CT_HTML;
    if (content.type == ILMA_TEXT) return content;
    return ilma_text("");
}

IlmaValue ilma_web_json(IlmaValue data) {
    g_ctype = CT_JSON;
    char* json = ilma_value_to_json(data);
    IlmaValue result = ilma_text(json);
    free(json);
    return result;
}

IlmaValue ilma_web_text(IlmaValue content) {
    g_ctype = CT_TEXT;
    if (content.type == ILMA_TEXT) return content;
    return ilma_text("");
}

IlmaValue ilma_web_send(IlmaValue response) {
    if (g_client_fd == INVALID_SOCKET) return ilma_empty_val();

    const char* ctype_str;
    switch (g_ctype) {
        case CT_JSON: ctype_str = "application/json"; break;
        case CT_TEXT: ctype_str = "text/plain";       break;
        default:      ctype_str = "text/html";        break;
    }

    const char* body = "";
    int blen = 0;
    if (response.type == ILMA_TEXT && response.as_text) {
        body = response.as_text;
        blen = (int)strlen(body);
    }

    send_response(g_client_fd, g_status, ctype_str, body, blen);
    CLOSESOCK(g_client_fd);
    g_client_fd = INVALID_SOCKET;

    /* Reset for next request */
    g_status = 200;
    g_ctype  = CT_HTML;
    return ilma_empty_val();
}

IlmaValue ilma_web_status(IlmaValue code) {
    if (code.type == ILMA_WHOLE)   g_status = (int)code.as_whole;
    if (code.type == ILMA_DECIMAL) g_status = (int)code.as_decimal;
    return ilma_empty_val();
}

IlmaValue ilma_web_redirect(IlmaValue url) {
    if (g_client_fd == INVALID_SOCKET) return ilma_empty_val();
    const char* loc = (url.type == ILMA_TEXT && url.as_text) ? url.as_text : "/";
    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n",
        loc);
    send(g_client_fd, header, (size_t)hlen, 0);
    CLOSESOCK(g_client_fd);
    g_client_fd = INVALID_SOCKET;
    g_status = 200;
    return ilma_empty_val();
}
