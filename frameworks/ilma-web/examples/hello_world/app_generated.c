/*
 * app_generated.c — C code that the ILMA compiler would produce for app.ilma
 *
 * This is the reference implementation showing how the ILMA code generator
 * maps the web.route / web.query / web.html / web.json API to C calls.
 *
 * Compilation (handled by the Makefile):
 *   gcc -O2 -Wall -std=c11 -D_POSIX_C_SOURCE=200809L \
 *       -o build/hello_world \
 *       app_generated.c \
 *       ../../src/ilma_http.c \
 *       ../../src/ilma_web_bridge.c \
 *       ../../../../src/runtime/ilma_runtime.c \
 *       -I../../src -I../../../../src/runtime -lm
 */

#include "ilma_web_bridge.h"
#include "ilma_runtime.h"

/* ═════════════════════════════════════════════════════════════
 *  Route handlers — one function per web.route(...) block
 * ═════════════════════════════════════════════════════════════ */

/*
 * web.route("/"):
 *     give back web.html("<html>...</html>")
 */
static IlmaHttpResponse route_home(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    IlmaValue html = ilma_text(
        "<html><body>"
        "<h1>Welcome to IlmaWeb!</h1>"
        "<p>Built entirely in ILMA.</p>"
        "<ul>"
        "<li><a href='/about'>About</a></li>"
        "<li><a href='/greet?name=World'>Greet</a></li>"
        "<li><a href='/api/status'>API Status</a></li>"
        "<li><a href='/calculate?a=5&b=3&op=add'>Calculator</a></li>"
        "</ul>"
        "</body></html>"
    );
    return ilma_web_html_response(html);
}

/*
 * web.route("/about"):
 *     give back web.html("<html>...</html>")
 */
static IlmaHttpResponse route_about(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    IlmaValue html = ilma_text(
        "<html><body>"
        "<h1>About IlmaWeb</h1>"
        "<p>IlmaWeb is the first web framework written in ILMA, "
        "a programming language built for children that compiles to C.</p>"
        "<a href='/'>Back home</a>"
        "</body></html>"
    );
    return ilma_web_html_response(html);
}

/*
 * web.route("/greet"):
 *     remember name = web.query("name")
 *     if name is empty:
 *         remember name = "friend"
 *     give back web.html("<html>...<h1>Assalamu Alaikum, " + name + "!</h1>...</html>")
 */
static IlmaHttpResponse route_greet(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    /* remember name = web.query("name") */
    IlmaValue name = ilma_web_query("name");

    /* if name is empty: remember name = "friend" */
    if (ilma_is_empty(name)) {
        name = ilma_text("friend");
    }

    /* give back web.html("<html>...<h1>Assalamu Alaikum, " + name + "!</h1>...</html>") */
    IlmaValue html = ilma_concat(
        ilma_text("<html><body><h1>Assalamu Alaikum, "),
        ilma_concat(
            name,
            ilma_text(
                "!</h1>"
                "<p>Welcome to IlmaWeb.</p>"
                "<a href='/'>Back home</a>"
                "</body></html>"
            )
        )
    );
    return ilma_web_html_response(html);
}

/*
 * web.route("/api/status"):
 *     remember status = notebook["server": "IlmaWeb", "version": "0.1.0",
 *                                "status": "running",  "language": "ILMA"]
 *     give back web.json(status)
 */
static IlmaHttpResponse route_api_status(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    /* remember status = notebook[...] */
    IlmaValue status = ilma_notebook_new();
    ilma_notebook_set(status.as_notebook, "server",   ilma_text("IlmaWeb"));
    ilma_notebook_set(status.as_notebook, "version",  ilma_text("0.1.0"));
    ilma_notebook_set(status.as_notebook, "status",   ilma_text("running"));
    ilma_notebook_set(status.as_notebook, "language", ilma_text("ILMA"));

    /* give back web.json(status) */
    return ilma_web_json_response(status);
}

/*
 * web.route("/calculate"):
 *     remember a_str = web.query("a")
 *     remember b_str = web.query("b")
 *     remember op    = web.query("op")
 *     remember result = notebook["a": a_str, "b": b_str,
 *                                "operation": op,
 *                                "result": "use ?a=5&b=3&op=add"]
 *     give back web.json(result)
 */
static IlmaHttpResponse route_calculate(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    IlmaValue a_str = ilma_web_query("a");
    IlmaValue b_str = ilma_web_query("b");
    IlmaValue op    = ilma_web_query("op");

    IlmaValue result = ilma_notebook_new();
    ilma_notebook_set(result.as_notebook, "a",         a_str);
    ilma_notebook_set(result.as_notebook, "b",         b_str);
    ilma_notebook_set(result.as_notebook, "operation", op);
    ilma_notebook_set(result.as_notebook, "result",
                      ilma_text("use ?a=5&b=3&op=add"));

    return ilma_web_json_response(result);
}

/* ═════════════════════════════════════════════════════════════
 *  main — register routes and start the server
 * ═════════════════════════════════════════════════════════════ */

int main(void) {
    /* web.start(port: 3000) — initialise on port 3000 */
    ilma_web_init(3000);

    /* Register all routes */
    ilma_web_add_route("/",          route_home);
    ilma_web_add_route("/about",     route_about);
    ilma_web_add_route("/greet",     route_greet);
    ilma_web_add_route("/api/status", route_api_status);
    ilma_web_add_route("/calculate", route_calculate);

    /* Start the server (blocks forever) */
    ilma_web_start();

    return 0;
}
