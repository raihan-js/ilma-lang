/*
 * app_generated.c — C code that the ILMA compiler would produce for the
 *                   todo_app/app.ilma example.
 *
 * This is the reference implementation showing how the ILMA code generator
 * maps the Todo App's web.route / web.query / web.html / web.json API to C.
 *
 * Compilation (handled by the Makefile):
 *   gcc -O2 -Wall -std=c11 -D_POSIX_C_SOURCE=200809L \
 *       -o build/todo_app \
 *       app_generated.c \
 *       ../../src/ilma_http.c \
 *       ../../src/ilma_web_bridge.c \
 *       ../../../../src/runtime/ilma_runtime.c \
 *       -I../../src -I../../../../src/runtime -lm
 */

#include "ilma_web_bridge.h"
#include "ilma_runtime.h"

#include <string.h>

/* ═════════════════════════════════════════════════════════════
 *  Global state — in-memory todo storage
 * ═════════════════════════════════════════════════════════════ */

static IlmaValue todos;       /* bag[] of notebook entries */
static int64_t   next_id = 1;

/* ═════════════════════════════════════════════════════════════
 *  Route handlers — one function per web.route(...) block
 * ═════════════════════════════════════════════════════════════ */

/*
 * web.route("/"):
 *     Build HTML with the todo list and a form to add new todos.
 */
static IlmaHttpResponse route_home(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    /* Build the HTML string piece by piece */
    IlmaValue html = ilma_text(
        "<html><head><title>ILMA Todo App</title></head><body>"
        "<h1>ILMA Todo List</h1>"
        "<form action='/todos/add' method='POST'>"
        "<input name='text' placeholder='New task...' required>"
        "<button type='submit'>Add</button></form><ul>"
    );

    /* for each todo in todos: html = html + "<li>" + todo[text] + "</li>" */
    if (todos.type == ILMA_BAG && todos.as_bag) {
        for (int i = 0; i < todos.as_bag->count; i++) {
            IlmaValue todo = todos.as_bag->items[i];
            IlmaValue text_val = ilma_notebook_get(todo.as_notebook, "text");
            html = ilma_concat(html, ilma_text("<li>"));
            html = ilma_concat(html, text_val);
            html = ilma_concat(html, ilma_text("</li>"));
        }
    }

    html = ilma_concat(html, ilma_text(
        "</ul><p><a href='/about'>About</a></p></body></html>"
    ));

    return ilma_web_html_response(html);
}

/*
 * web.route("/todos"):
 *     give back web.json(todos)
 */
static IlmaHttpResponse route_todos_list(IlmaHttpRequest* req) {
    ilma_web_current_request = req;
    return ilma_web_json_response(todos);
}

/*
 * web.route("/todos/add"):
 *     remember text = web.query("text")
 *     if text is empty: text = "Untitled task"
 *     remember todo = notebook[id: next_id, text: text, done: no]
 *     todos.add(todo)
 *     next_id = next_id + 1
 *     give back web.html("<html>...<p>Added!</p>...</html>")
 */
static IlmaHttpResponse route_todos_add(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    /* For POST requests, also parse form body for the "text" field */
    IlmaValue text_val = ilma_web_query("text");

    /* If not in query string, try to find it in the POST body */
    if (ilma_is_empty(text_val) && req->body[0] != '\0') {
        /* Simple form body parsing: look for text= */
        const char* body = req->body;
        const char* found = strstr(body, "text=");
        if (found) {
            found += 5; /* skip "text=" */
            char buf[256];
            size_t i = 0;
            while (found[i] && found[i] != '&' && i < sizeof(buf) - 1) {
                if (found[i] == '+') {
                    buf[i] = ' ';
                } else {
                    buf[i] = found[i];
                }
                i++;
            }
            buf[i] = '\0';
            if (i > 0) {
                text_val = ilma_text(buf);
            }
        }
    }

    /* if text is empty: text = "Untitled task" */
    if (ilma_is_empty(text_val)) {
        text_val = ilma_text("Untitled task");
    }

    /* remember todo = notebook[id: next_id, text: text, done: no] */
    IlmaValue todo = ilma_notebook_new();
    ilma_notebook_set(todo.as_notebook, "id", ilma_whole(next_id));
    ilma_notebook_set(todo.as_notebook, "text", text_val);
    ilma_notebook_set(todo.as_notebook, "done", ilma_no());

    /* todos.add(todo) */
    ilma_bag_add(todos.as_bag, todo);

    /* next_id = next_id + 1 */
    next_id++;

    /* give back web.html(...) */
    IlmaValue html = ilma_text(
        "<html><body><p>Added!</p><a href='/'>Back</a></body></html>"
    );
    return ilma_web_html_response(html);
}

/*
 * web.route("/about"):
 *     give back web.html("<html>...<h1>About</h1>...</html>")
 */
static IlmaHttpResponse route_about(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    IlmaValue html = ilma_text(
        "<html><body><h1>About</h1>"
        "<p>Built with IlmaWeb framework.</p>"
        "<a href='/'>Home</a></body></html>"
    );
    return ilma_web_html_response(html);
}

/*
 * web.route("/health"):
 *     give back web.json(notebook[status: "ok", language: "ILMA"])
 */
static IlmaHttpResponse route_health(IlmaHttpRequest* req) {
    ilma_web_current_request = req;

    IlmaValue data = ilma_notebook_new();
    ilma_notebook_set(data.as_notebook, "status",   ilma_text("ok"));
    ilma_notebook_set(data.as_notebook, "language", ilma_text("ILMA"));

    return ilma_web_json_response(data);
}

/* ═════════════════════════════════════════════════════════════
 *  main — initialise state, register routes, start the server
 * ═════════════════════════════════════════════════════════════ */

int main(void) {
    /* remember todos = bag[] */
    todos = ilma_bag_new();

    /* remember next_id = 1 */
    next_id = 1;

    /* web.start(port: 3000) — initialise on port 3000 */
    ilma_web_init(3000);

    /* Register all routes */
    ilma_web_add_route("/",          route_home);
    ilma_web_add_route("/todos",     route_todos_list);
    ilma_web_add_route("/todos/add", route_todos_add);
    ilma_web_add_route("/about",     route_about);
    ilma_web_add_route("/health",    route_health);

    /* Start the server (blocks forever) */
    ilma_web_start();

    return 0;
}
