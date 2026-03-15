/*
 * ilma_http.c — Minimal HTTP/1.1 server implementation for IlmaWeb
 *
 * Pure POSIX C, single-threaded, under 400 lines.
 * Handles: TCP socket creation, request parsing (method, path, query
 * string, headers, body), route matching, response formatting.
 */

#include "ilma_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ── URL-decode a percent-encoded string in place ─────────── */

static void url_decode(char* dst, const char* src, size_t dst_size) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di < dst_size - 1; si++) {
        if (src[si] == '%' && src[si + 1] && src[si + 2]) {
            char hex[3] = { src[si + 1], src[si + 2], '\0' };
            dst[di++] = (char)strtol(hex, NULL, 16);
            si += 2;
        } else if (src[si] == '+') {
            dst[di++] = ' ';
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
}

/* ── Parse query string into key-value pairs ──────────────── */

static void parse_query_string(IlmaHttpRequest* req) {
    if (req->query_string[0] == '\0') return;

    char buf[512];
    strncpy(buf, req->query_string, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* pair = strtok(buf, "&");
    while (pair && req->query_count < ILMA_HTTP_MAX_QUERY) {
        char* eq = strchr(pair, '=');
        if (eq) {
            *eq = '\0';
            url_decode(req->query[req->query_count].key, pair,
                       sizeof(req->query[req->query_count].key));
            url_decode(req->query[req->query_count].value, eq + 1,
                       sizeof(req->query[req->query_count].value));
        } else {
            url_decode(req->query[req->query_count].key, pair,
                       sizeof(req->query[req->query_count].key));
            req->query[req->query_count].value[0] = '\0';
        }
        req->query_count++;
        pair = strtok(NULL, "&");
    }
}

/* ── Parse a raw HTTP request buffer ──────────────────────── */

static int parse_request(const char* raw, size_t raw_len, IlmaHttpRequest* req) {
    memset(req, 0, sizeof(*req));

    /* --- request line ------------------------------------------------ */
    const char* line_end = strstr(raw, "\r\n");
    if (!line_end) return -1;

    /* method */
    const char* p = raw;
    size_t i = 0;
    while (p < line_end && *p != ' ' && i < sizeof(req->method) - 1)
        req->method[i++] = *p++;
    req->method[i] = '\0';
    if (*p == ' ') p++;

    /* path (and optional query string) */
    i = 0;
    while (p < line_end && *p != ' ' && *p != '?' && i < sizeof(req->path) - 1)
        req->path[i++] = *p++;
    req->path[i] = '\0';

    if (*p == '?') {
        p++; /* skip '?' */
        i = 0;
        while (p < line_end && *p != ' ' && i < sizeof(req->query_string) - 1)
            req->query_string[i++] = *p++;
        req->query_string[i] = '\0';
    }

    /* --- headers ----------------------------------------------------- */
    const char* header_start = line_end + 2; /* skip \r\n */
    const char* headers_end  = strstr(header_start, "\r\n\r\n");
    if (!headers_end) return -1;

    const char* hp = header_start;
    while (hp < headers_end && req->header_count < ILMA_HTTP_MAX_HEADERS) {
        const char* he = strstr(hp, "\r\n");
        if (!he || he > headers_end) break;

        const char* colon = memchr(hp, ':', (size_t)(he - hp));
        if (colon) {
            size_t klen = (size_t)(colon - hp);
            if (klen >= sizeof(req->headers[0].key))
                klen = sizeof(req->headers[0].key) - 1;
            memcpy(req->headers[req->header_count].key, hp, klen);
            req->headers[req->header_count].key[klen] = '\0';

            const char* vp = colon + 1;
            while (vp < he && *vp == ' ') vp++;
            size_t vlen = (size_t)(he - vp);
            if (vlen >= sizeof(req->headers[0].value))
                vlen = sizeof(req->headers[0].value) - 1;
            memcpy(req->headers[req->header_count].value, vp, vlen);
            req->headers[req->header_count].value[vlen] = '\0';

            req->header_count++;
        }
        hp = he + 2;
    }

    /* --- Content-Length & body ---------------------------------------- */
    const char* cl = ilma_http_header_get(req, "Content-Length");
    if (cl) {
        req->content_length = atoi(cl);
    }

    const char* body_start = headers_end + 4; /* skip \r\n\r\n */
    size_t body_available = raw_len - (size_t)(body_start - raw);
    size_t to_copy = (size_t)req->content_length;
    if (to_copy > body_available) to_copy = body_available;
    if (to_copy >= sizeof(req->body)) to_copy = sizeof(req->body) - 1;
    if (to_copy > 0) memcpy(req->body, body_start, to_copy);
    req->body[to_copy] = '\0';

    /* --- parse query params ------------------------------------------ */
    parse_query_string(req);

    return 0;
}

/* ── Status code → reason phrase ──────────────────────────── */

static const char* status_phrase(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

/* ── Build and send the HTTP response ─────────────────────── */

static void send_response(int client_fd, IlmaHttpResponse* res) {
    char header_buf[1024];
    int hlen = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        res->status, status_phrase(res->status),
        res->content_type,
        res->body_length);

    ssize_t w = write(client_fd, header_buf, (size_t)hlen);
    (void)w;
    if (res->body_length > 0) {
        w = write(client_fd, res->body, (size_t)res->body_length);
        (void)w;
    }
}

/* ── Default 404 page ─────────────────────────────────────── */

static IlmaHttpResponse not_found_response(const char* path) {
    IlmaHttpResponse res;
    memset(&res, 0, sizeof(res));
    res.status = 404;
    strncpy(res.content_type, "text/html", sizeof(res.content_type));
    res.body_length = snprintf(res.body, sizeof(res.body),
        "<html><head><title>404 — Not Found</title>"
        "<style>body{font-family:sans-serif;text-align:center;padding:60px;}"
        "h1{font-size:3em;margin-bottom:0;}p{color:#555;}</style></head>"
        "<body><h1>404</h1><p>The path <code>%s</code> was not found on this "
        "server.</p><hr><p><em>IlmaWeb</em></p></body></html>", path);
    return res;
}

/* ── Match a route against the registered table ───────────── */

static IlmaRoute* match_route(IlmaHttpServer* server, IlmaHttpRequest* req) {
    for (int i = 0; i < server->route_count; i++) {
        IlmaRoute* r = &server->routes[i];
        /* Match path; method "*" matches any method */
        if (strcmp(r->path, req->path) == 0) {
            if (r->method[0] == '*' || strcmp(r->method, req->method) == 0) {
                return r;
            }
        }
    }
    return NULL;
}

/* ── Handle a single client connection ────────────────────── */

static void handle_client(IlmaHttpServer* server, int client_fd) {
    char buffer[ILMA_HTTP_BUFFER_SIZE];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    IlmaHttpRequest req;
    if (parse_request(buffer, (size_t)n, &req) < 0) {
        close(client_fd);
        return;
    }

    IlmaHttpResponse res;
    IlmaRoute* route = match_route(server, &req);
    if (route) {
        res = route->handler(&req);
    } else {
        res = not_found_response(req.path);
    }

    /* Log the request */
    printf("%s %s -> %d\n", req.method, req.path, res.status);
    fflush(stdout);

    send_response(client_fd, &res);
    close(client_fd);
}

/* ═════════════════════════════════════════════════════════════
 *  Public API
 * ═════════════════════════════════════════════════════════════ */

void ilma_http_init(IlmaHttpServer* server, int port) {
    memset(server, 0, sizeof(*server));
    server->port = port;
}

void ilma_http_route(IlmaHttpServer* server, const char* method,
                     const char* path, IlmaRouteHandler handler) {
    if (server->route_count >= ILMA_HTTP_MAX_ROUTES) {
        fprintf(stderr, "IlmaWeb: too many routes (max %d)\n",
                ILMA_HTTP_MAX_ROUTES);
        return;
    }
    IlmaRoute* r = &server->routes[server->route_count++];
    strncpy(r->method, method, sizeof(r->method) - 1);
    strncpy(r->path, path, sizeof(r->path) - 1);
    r->handler = handler;
}

void ilma_http_start(IlmaHttpServer* server) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("IlmaWeb: socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)server->port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("IlmaWeb: bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 128) < 0) {
        perror("IlmaWeb: listen");
        close(server_fd);
        exit(1);
    }

    printf("IlmaWeb server running on http://localhost:%d\n", server->port);
    fflush(stdout);

    /* Accept loop */
    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("IlmaWeb: accept");
            continue;
        }
        handle_client(server, client_fd);
    }
}

/* ── Request helpers ──────────────────────────────────────── */

const char* ilma_http_query_get(IlmaHttpRequest* req, const char* key) {
    for (int i = 0; i < req->query_count; i++) {
        if (strcmp(req->query[i].key, key) == 0) {
            return req->query[i].value;
        }
    }
    return NULL;
}

const char* ilma_http_header_get(IlmaHttpRequest* req, const char* key) {
    for (int i = 0; i < req->header_count; i++) {
        /* Case-insensitive comparison */
        if (strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

/* ── Response builders ────────────────────────────────────── */

IlmaHttpResponse ilma_http_html(int status, const char* html) {
    IlmaHttpResponse res;
    memset(&res, 0, sizeof(res));
    res.status = status;
    strncpy(res.content_type, "text/html; charset=utf-8",
            sizeof(res.content_type));
    res.body_length = snprintf(res.body, sizeof(res.body), "%s", html);
    return res;
}

IlmaHttpResponse ilma_http_json(int status, const char* json) {
    IlmaHttpResponse res;
    memset(&res, 0, sizeof(res));
    res.status = status;
    strncpy(res.content_type, "application/json",
            sizeof(res.content_type));
    res.body_length = snprintf(res.body, sizeof(res.body), "%s", json);
    return res;
}

IlmaHttpResponse ilma_http_text(int status, const char* text) {
    IlmaHttpResponse res;
    memset(&res, 0, sizeof(res));
    res.status = status;
    strncpy(res.content_type, "text/plain; charset=utf-8",
            sizeof(res.content_type));
    res.body_length = snprintf(res.body, sizeof(res.body), "%s", text);
    return res;
}

IlmaHttpResponse ilma_http_redirect(const char* url) {
    IlmaHttpResponse res;
    memset(&res, 0, sizeof(res));
    res.status = 302;
    strncpy(res.content_type, "text/html; charset=utf-8",
            sizeof(res.content_type));
    res.body_length = snprintf(res.body, sizeof(res.body),
        "<html><body>Redirecting to <a href=\"%s\">%s</a>...</body></html>",
        url, url);
    /* We embed the Location in the Content-Type trick?  No — we need a
     * real Location header.  Override send_response to handle 302 by
     * storing the URL in the body and writing a custom header.
     * Actually, let's just stuff "Location: url\r\n" before Content-Type
     * by using a small trick: we write it into content_type so the
     * generic send_response emits it.  BETTER: just handle it cleanly.
     * We'll encode location in content_type with a newline. */
    snprintf(res.content_type, sizeof(res.content_type),
             "text/html; charset=utf-8\r\nLocation: %s", url);
    return res;
}
