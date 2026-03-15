/*
 * ilma_web_bridge.h — Bridge between the ILMA runtime and the HTTP server
 *
 * The ILMA code generator emits calls to these functions.  They translate
 * between IlmaValue (the runtime's universal value type) and the raw C
 * types used by the HTTP layer.
 */

#ifndef ILMA_WEB_BRIDGE_H
#define ILMA_WEB_BRIDGE_H

#include "ilma_http.h"
#include "../../src/runtime/ilma_runtime.h"

/* ── Global state ─────────────────────────────────────────── */
extern IlmaHttpServer     ilma_web_server;
extern IlmaHttpRequest*   ilma_web_current_request;

/* ── Server lifecycle (called from generated main()) ──────── */
void ilma_web_init(int port);
void ilma_web_add_route(const char* path, IlmaRouteHandler handler);
void ilma_web_start(void);

/* ── Request data access (return IlmaValue) ───────────────── */
IlmaValue ilma_web_query(const char* key);
IlmaValue ilma_web_body_text(void);
IlmaValue ilma_web_header(const char* key);
IlmaValue ilma_web_method(void);
IlmaValue ilma_web_path(void);

/* ── Response builders (return IlmaHttpResponse) ──────────── */
IlmaHttpResponse ilma_web_html_response(IlmaValue html_val);
IlmaHttpResponse ilma_web_json_response(IlmaValue data);
IlmaHttpResponse ilma_web_text_response(IlmaValue text_val);

#endif /* ILMA_WEB_BRIDGE_H */
