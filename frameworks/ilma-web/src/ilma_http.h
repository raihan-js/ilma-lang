/*
 * ilma_http.h — Minimal HTTP server for the IlmaWeb framework
 *
 * This provides a lightweight, single-threaded HTTP/1.1 server built on
 * POSIX sockets.  The ILMA code generator emits calls to these functions
 * (through the bridge layer) so that ILMA programs can serve web pages
 * and JSON APIs as native binaries.
 */

#ifndef ILMA_HTTP_H
#define ILMA_HTTP_H

#include <stddef.h>

#define ILMA_HTTP_MAX_ROUTES    64
#define ILMA_HTTP_MAX_HEADERS   32
#define ILMA_HTTP_MAX_QUERY     32
#define ILMA_HTTP_BUFFER_SIZE   8192

typedef struct {
    char method[8];          /* GET, POST, PUT, DELETE, etc. */
    char path[256];          /* /path                        */
    char query_string[512];  /* raw query string             */
    char body[4096];         /* request body                 */
    int  content_length;
    struct {
        char key[64];
        char value[256];
    } headers[ILMA_HTTP_MAX_HEADERS];
    int header_count;
    struct {
        char key[64];
        char value[256];
    } query[ILMA_HTTP_MAX_QUERY];
    int query_count;
} IlmaHttpRequest;

typedef struct {
    int  status;
    char content_type[64];
    char body[65536];
    int  body_length;
} IlmaHttpResponse;

/* Route handler function type */
typedef IlmaHttpResponse (*IlmaRouteHandler)(IlmaHttpRequest* req);

typedef struct {
    char method[8];
    char path[256];
    IlmaRouteHandler handler;
} IlmaRoute;

typedef struct {
    IlmaRoute routes[ILMA_HTTP_MAX_ROUTES];
    int route_count;
    int port;
} IlmaHttpServer;

/* ── Server lifecycle ─────────────────────────────────────── */
void ilma_http_init(IlmaHttpServer* server, int port);
void ilma_http_route(IlmaHttpServer* server, const char* method,
                     const char* path, IlmaRouteHandler handler);
void ilma_http_start(IlmaHttpServer* server);

/* ── Request helpers ──────────────────────────────────────── */
const char* ilma_http_query_get(IlmaHttpRequest* req, const char* key);
const char* ilma_http_header_get(IlmaHttpRequest* req, const char* key);

/* ── Response builders ────────────────────────────────────── */
IlmaHttpResponse ilma_http_html(int status, const char* html);
IlmaHttpResponse ilma_http_json(int status, const char* json);
IlmaHttpResponse ilma_http_text(int status, const char* text);
IlmaHttpResponse ilma_http_redirect(const char* url);

#endif /* ILMA_HTTP_H */
