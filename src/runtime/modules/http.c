#define _POSIX_C_SOURCE 200809L
#include "http.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   typedef int socklen_t;
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
   typedef int SOCKET;
#  define INVALID_SOCKET (-1)
#  define closesocket(s) close(s)
#endif

#define HTTP_MAX_RESPONSE (1024 * 1024)  /* 1 MB */
#define HTTP_MAX_REDIRECTS 5
#define HTTP_TIMEOUT_SEC  10

/* ── URL parsing ─────────────────────────────────────── */

typedef struct {
    char scheme[8];   /* http or https */
    char host[256];
    char path[1024];
    int  port;
} ParsedUrl;

static int parse_url(const char* url, ParsedUrl* out) {
    memset(out, 0, sizeof(*out));
    out->port = 80;

    if (strncmp(url, "https://", 8) == 0) {
        strncpy(out->scheme, "https", sizeof(out->scheme) - 1);
        url += 8;
        out->port = 443;
    } else if (strncmp(url, "http://", 7) == 0) {
        strncpy(out->scheme, "http", sizeof(out->scheme) - 1);
        url += 7;
    } else {
        strncpy(out->scheme, "http", sizeof(out->scheme) - 1);
    }

    /* Extract host[:port] and path */
    const char* slash = strchr(url, '/');
    const char* host_end = slash ? slash : url + strlen(url);

    /* Check for port in host */
    const char* colon = strchr(url, ':');
    if (colon && colon < host_end) {
        int host_len = (int)(colon - url);
        if (host_len >= (int)sizeof(out->host)) host_len = sizeof(out->host) - 1;
        strncpy(out->host, url, host_len);
        out->host[host_len] = '\0';
        out->port = atoi(colon + 1);
    } else {
        int host_len = (int)(host_end - url);
        if (host_len >= (int)sizeof(out->host)) host_len = sizeof(out->host) - 1;
        strncpy(out->host, url, host_len);
        out->host[host_len] = '\0';
    }

    if (slash) {
        strncpy(out->path, slash, sizeof(out->path) - 1);
    } else {
        strcpy(out->path, "/");
    }

    return (out->host[0] != '\0') ? 1 : 0;
}

/* ── Low-level HTTP request ──────────────────────────── */

typedef struct {
    int    status;
    char*  body;
    int    body_len;
    char*  headers;   /* raw headers string */
    char*  location;  /* redirect URL if any */
} HttpResponse;

static void http_response_free(HttpResponse* r) {
    free(r->body);
    free(r->headers);
    free(r->location);
    memset(r, 0, sizeof(*r));
}

static int http_do_request(const char* host, int port, const char* method,
                            const char* path, const char* req_body,
                            const char* content_type, HttpResponse* resp) {
    memset(resp, 0, sizeof(*resp));

    /* Resolve host */
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int gai = getaddrinfo(host, port_str, &hints, &res);
    if (gai != 0 || !res) {
        return -1;
    }

    /* Connect */
    SOCKET sock = INVALID_SOCKET;
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock == INVALID_SOCKET) continue;

#ifndef _WIN32
        /* Set timeout */
        struct timeval tv = { HTTP_TIMEOUT_SEC, 0 };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

        if (connect(sock, ai->ai_addr, (socklen_t)ai->ai_addrlen) == 0) {
            break;
        }
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    freeaddrinfo(res);

    if (sock == INVALID_SOCKET) return -1;

    /* Build request */
    char req[4096];
    int req_len = 0;
    int body_len = req_body ? (int)strlen(req_body) : 0;

    req_len += snprintf(req + req_len, sizeof(req) - req_len,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: ILMA/0.6.0\r\n"
        "Connection: close\r\n"
        "Accept: */*\r\n",
        method, path, host);

    if (body_len > 0) {
        req_len += snprintf(req + req_len, sizeof(req) - req_len,
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n",
            content_type ? content_type : "application/json",
            body_len);
    }
    req_len += snprintf(req + req_len, sizeof(req) - req_len, "\r\n");

    /* Send headers */
    if (send(sock, req, req_len, 0) < 0) {
        closesocket(sock);
        return -1;
    }
    /* Send body if any */
    if (body_len > 0) {
        if (send(sock, req_body, body_len, 0) < 0) {
            closesocket(sock);
            return -1;
        }
    }

    /* Read response */
    char* buf = malloc(HTTP_MAX_RESPONSE + 1);
    if (!buf) { closesocket(sock); return -1; }
    int total = 0;

    while (total < HTTP_MAX_RESPONSE) {
        int n = (int)recv(sock, buf + total, HTTP_MAX_RESPONSE - total, 0);
        if (n <= 0) break;
        total += n;
    }
    closesocket(sock);
    buf[total] = '\0';

    /* Parse response */
    char* header_end = strstr(buf, "\r\n\r\n");
    if (!header_end) {
        free(buf);
        return -1;
    }

    /* Status line: HTTP/1.1 200 OK */
    resp->status = 200;
    if (strncmp(buf, "HTTP/", 5) == 0) {
        const char* sp = strchr(buf, ' ');
        if (sp) resp->status = atoi(sp + 1);
    }

    /* Store headers */
    int hdr_len = (int)(header_end - buf);
    resp->headers = malloc(hdr_len + 1);
    memcpy(resp->headers, buf, hdr_len);
    resp->headers[hdr_len] = '\0';

    /* Look for Location header (redirect) */
    char* loc = strstr(resp->headers, "\r\nLocation: ");
    if (loc) {
        loc += 12; /* skip "\r\nLocation: " */
        char* loc_end = strstr(loc, "\r\n");
        int loc_len = loc_end ? (int)(loc_end - loc) : (int)strlen(loc);
        resp->location = malloc(loc_len + 1);
        memcpy(resp->location, loc, loc_len);
        resp->location[loc_len] = '\0';
    }

    /* Body */
    char* body_start = header_end + 4;
    resp->body_len = total - (int)(body_start - buf);
    if (resp->body_len > 0) {
        resp->body = malloc(resp->body_len + 1);
        memcpy(resp->body, body_start, resp->body_len);
        resp->body[resp->body_len] = '\0';
    } else {
        resp->body = strdup("");
    }

    free(buf);
    return 0;
}

/* ── Build IlmaValue response notebook ──────────────── */

static IlmaValue build_response(HttpResponse* resp) {
    IlmaValue nb = ilma_notebook_new();
    ilma_notebook_set(nb.as_notebook, "status",
                      ilma_whole(resp->status));
    ilma_notebook_set(nb.as_notebook, "body",
                      ilma_text(resp->body ? resp->body : ""));

    /* Basic headers notebook */
    IlmaValue hdrs = ilma_notebook_new();
    if (resp->headers) {
        char* hdr_copy = strdup(resp->headers);
        char* line = strtok(hdr_copy, "\r\n");
        while (line) {
            char* colon_pos = strchr(line, ':');
            if (colon_pos) {
                *colon_pos = '\0';
                const char* val = colon_pos + 1;
                while (*val == ' ') val++;
                /* Lowercase the header name */
                for (char* c = line; *c; c++) {
                    if (*c >= 'A' && *c <= 'Z') *c += 32;
                }
                ilma_notebook_set(hdrs.as_notebook, line, ilma_text(val));
            }
            line = strtok(NULL, "\r\n");
        }
        free(hdr_copy);
    }
    ilma_notebook_set(nb.as_notebook, "headers", hdrs);
    return nb;
}

/* ── Public API ──────────────────────────────────────── */

IlmaValue ilma_http_get(IlmaValue url_val) {
    if (url_val.type != ILMA_TEXT || !url_val.as_text)
        return ilma_empty_val();

    char url_buf[2048];
    strncpy(url_buf, url_val.as_text, sizeof(url_buf) - 1);
    url_buf[sizeof(url_buf) - 1] = '\0';

    for (int redirect = 0; redirect < HTTP_MAX_REDIRECTS; redirect++) {
        ParsedUrl pu;
        if (!parse_url(url_buf, &pu)) return ilma_empty_val();

        /* HTTPS: fall back to system curl if available */
        if (strcmp(pu.scheme, "https") == 0) {
            char cmd[2200];
            snprintf(cmd, sizeof(cmd),
                     "curl -s -L --max-time %d '%s' 2>/dev/null",
                     HTTP_TIMEOUT_SEC, url_buf);
            FILE* f = popen(cmd, "r");
            if (!f) return ilma_empty_val();
            char* body = malloc(HTTP_MAX_RESPONSE + 1);
            int n = (int)fread(body, 1, HTTP_MAX_RESPONSE, f);
            pclose(f);
            body[n] = '\0';
            IlmaValue nb = ilma_notebook_new();
            ilma_notebook_set(nb.as_notebook, "status", ilma_whole(200));
            IlmaValue bv = ilma_text(body);
            free(body);
            ilma_notebook_set(nb.as_notebook, "body", bv);
            ilma_notebook_set(nb.as_notebook, "headers", ilma_notebook_new());
            return nb;
        }

        HttpResponse resp;
        int rc = http_do_request(pu.host, pu.port, "GET", pu.path,
                                 NULL, NULL, &resp);
        if (rc != 0) {
            return ilma_empty_val();
        }

        /* Follow redirects */
        if ((resp.status == 301 || resp.status == 302 || resp.status == 307)
            && resp.location) {
            strncpy(url_buf, resp.location, sizeof(url_buf) - 1);
            http_response_free(&resp);
            continue;
        }

        IlmaValue result = build_response(&resp);
        http_response_free(&resp);
        return result;
    }
    return ilma_empty_val();
}

IlmaValue ilma_http_post(IlmaValue url_val, IlmaValue body_val,
                         IlmaValue content_type_val) {
    if (url_val.type != ILMA_TEXT || !url_val.as_text)
        return ilma_empty_val();

    ParsedUrl pu;
    if (!parse_url(url_val.as_text, &pu)) return ilma_empty_val();

    const char* body_str = (body_val.type == ILMA_TEXT && body_val.as_text)
                           ? body_val.as_text : "";
    const char* ct_str = (content_type_val.type == ILMA_TEXT && content_type_val.as_text)
                         ? content_type_val.as_text : "application/json";

    HttpResponse resp;
    int rc = http_do_request(pu.host, pu.port, "POST", pu.path,
                             body_str, ct_str, &resp);
    if (rc != 0) return ilma_empty_val();

    IlmaValue result = build_response(&resp);
    http_response_free(&resp);
    return result;
}

IlmaValue ilma_http_get_json(IlmaValue url_val) {
    IlmaValue resp = ilma_http_get(url_val);
    if (resp.type != ILMA_NOTEBOOK) return ilma_empty_val();
    IlmaValue body = ilma_notebook_get(resp.as_notebook, "body");
    return ilma_json_parse(body);
}

IlmaValue ilma_http_post_json(IlmaValue url_val, IlmaValue data) {
    IlmaValue json_str = ilma_json_stringify(data);
    IlmaValue ct = ilma_text("application/json");
    IlmaValue resp = ilma_http_post(url_val, json_str, ct);
    if (resp.type != ILMA_NOTEBOOK) return ilma_empty_val();
    IlmaValue body = ilma_notebook_get(resp.as_notebook, "body");
    return ilma_json_parse(body);
}
