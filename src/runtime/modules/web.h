#ifndef ILMA_WEB_H
#define ILMA_WEB_H

#include "../ilma_runtime.h"

/*
 * ILMA web module — event-loop HTTP server
 *
 * Usage in ILMA:
 *   use web
 *   web.listen(3000)
 *   keep going while web.accept():
 *       remember path = web.path()
 *       if path is "/":
 *           web.send(web.html("<h1>Hello!</h1>"))
 *       otherwise:
 *           web.send(web.html("<h1>404 Not Found</h1>"))
 */

/* ── Server lifecycle ──────────────────────────────────── */

/* Bind and listen on the given port. Prints startup message. */
IlmaValue ilma_web_listen(IlmaValue port);

/* Accept next connection and parse the HTTP request.
 * Returns ilma_yes() on success, ilma_no() on error/shutdown. */
IlmaValue ilma_web_accept(void);

/* ── Request accessors ─────────────────────────────────── */

/* Return the request path, e.g. "/about" */
IlmaValue ilma_web_path(void);

/* Return the HTTP method, e.g. "GET" or "POST" */
IlmaValue ilma_web_method(void);

/* Return a query-string parameter value, e.g. web.query("name")
 * Returns empty if the key is not present. */
IlmaValue ilma_web_query(IlmaValue key);

/* Return the raw request body (for POST/PUT) */
IlmaValue ilma_web_body(void);

/* Return a request header value by name (case-insensitive) */
IlmaValue ilma_web_header(IlmaValue name);

/* ── Response builders ─────────────────────────────────── */

/* Wrap content as an HTML response (Content-Type: text/html) */
IlmaValue ilma_web_html(IlmaValue content);

/* Serialize data (notebook or bag) and wrap as JSON response */
IlmaValue ilma_web_json(IlmaValue data);

/* Wrap content as a plain-text response */
IlmaValue ilma_web_text(IlmaValue content);

/* ── Send ──────────────────────────────────────────────── */

/* Send the response to the current client and close the connection. */
IlmaValue ilma_web_send(IlmaValue response);

/* Set HTTP status code for the next web.send() call (default 200).
 * e.g. web.status(404) before web.send(web.html("Not found")) */
IlmaValue ilma_web_status(IlmaValue code);

/* Convenience: send a 302 redirect to the given URL */
IlmaValue ilma_web_redirect(IlmaValue url);

#endif /* ILMA_WEB_H */
