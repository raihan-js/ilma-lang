# IlmaWeb

**Build a web server in 5 lines of ILMA.**

```ilma
use web

web.route("/"):
    give back web.html("<h1>Hello from IlmaWeb!</h1>")

web.start(port: 3000)
```

IlmaWeb is the first web framework written for [ILMA](https://github.com/raihan/ilma) — a programming language designed for children that compiles to C. Your ILMA web application becomes a native binary: no interpreter, no VM, no runtime overhead.

---

## Features

- **Routes** — Register handlers for any path with `web.route("/path")`
- **Query parameters** — Access `?key=value` pairs with `web.query("key")`
- **JSON APIs** — Return notebooks as JSON with `web.json(data)`
- **HTML pages** — Serve HTML with `web.html("<h1>Hello</h1>")`
- **Plain text** — Return raw text with `web.text("hello")`
- **Request headers** — Read any header with `web.header("Content-Type")`
- **Request body** — Read POST bodies with `web.body()`
- **Zero dependencies** — Pure POSIX C. No libuv, no OpenSSL, no frameworks.
- **Compiles to native** — Your web server is a single binary.

---

## Hello World

### 1. Write your app (`app.ilma`)

```ilma
use web

web.route("/"):
    give back web.html("<h1>Assalamu Alaikum!</h1>")

web.route("/greet"):
    remember name = web.query("name")
    if name is empty:
        remember name = "friend"
    give back web.html("<h1>Hello, " + name + "!</h1>")

web.route("/api/status"):
    remember data = notebook["status": "running", "version": "0.1.0"]
    give back web.json(data)

web.start(port: 3000)
```

### 2. Build and run

```bash
cd frameworks/ilma-web
make build
make run
```

### 3. Visit

Open [http://localhost:3000](http://localhost:3000) in your browser.

---

## Installation

IlmaWeb is included in the ILMA source tree. No separate installation needed.

```
ilma/
├── src/
│   └── runtime/
│       ├── ilma_runtime.h
│       └── ilma_runtime.c
└── frameworks/
    └── ilma-web/
        ├── src/
        │   ├── ilma_http.h        # HTTP server
        │   ├── ilma_http.c
        │   ├── ilma_web_bridge.h   # ILMA ↔ HTTP bridge
        │   ├── ilma_web_bridge.c
        │   ├── ilma_db.h           # SQLite integration (optional)
        │   └── ilma_db.c
        ├── stdlib/
        │   └── web.ilma            # ILMA API reference
        ├── examples/
        │   ├── hello_world/
        │   │   ├── app.ilma            # Source (what you write)
        │   │   └── app_generated.c     # Generated C (what the compiler emits)
        │   └── todo_app/
        │       ├── app.ilma            # Todo app source
        │       └── app_generated.c     # Generated C for the todo app
        ├── Makefile
        └── README.md
```

---

## Route Handlers

### Basic route

```ilma
web.route("/about"):
    give back web.html("<h1>About us</h1>")
```

### Query parameters

```ilma
# GET /search?q=ilma
web.route("/search"):
    remember query = web.query("q")
    give back web.html("<p>You searched for: " + query + "</p>")
```

### Reading headers

```ilma
web.route("/debug"):
    remember ct = web.header("User-Agent")
    give back web.text("Your browser: " + ct)
```

### Request method and path

```ilma
web.route("/info"):
    remember m = web.method()
    remember p = web.path()
    give back web.text("Method: " + m + ", Path: " + p)
```

---

## JSON APIs

Return any ILMA value as JSON. Notebooks become objects, bags become arrays.

```ilma
web.route("/api/users"):
    remember users = bag[]
    bag.add(users, notebook["name": "Zayd", "age": 10])
    bag.add(users, notebook["name": "Maryam", "age": 8])
    give back web.json(users)
```

**Response:**

```json
[{"name":"Zayd","age":10},{"name":"Maryam","age":8}]
```

### Type mapping

| ILMA type   | JSON output       |
|-------------|-------------------|
| `text`      | `"string"`        |
| `whole`     | `42`              |
| `decimal`   | `3.14`            |
| `truth yes` | `true`            |
| `truth no`  | `false`           |
| `empty`     | `null`            |
| `bag`       | `[...]`           |
| `notebook`  | `{...}`           |

---

## HTML Responses

```ilma
web.route("/page"):
    remember title = "My Page"
    give back web.html("<html><head><title>" + title + "</title></head><body><h1>" + title + "</h1></body></html>")
```

---

## Express.js vs IlmaWeb

Side-by-side comparison — the same 3-route app in both frameworks:

### Express.js (JavaScript)

```javascript
const express = require('express');
const app = express();

app.get('/', (req, res) => {
    res.send('<h1>Hello!</h1>');
});

app.get('/greet', (req, res) => {
    const name = req.query.name || 'friend';
    res.send(`<h1>Hello, ${name}!</h1>`);
});

app.get('/api/status', (req, res) => {
    res.json({ status: 'running', version: '0.1.0' });
});

app.listen(3000);
```

### IlmaWeb (ILMA)

```ilma
use web

web.route("/"):
    give back web.html("<h1>Hello!</h1>")

web.route("/greet"):
    remember name = web.query("name")
    if name is empty:
        remember name = "friend"
    give back web.html("<h1>Hello, " + name + "!</h1>")

web.route("/api/status"):
    remember data = notebook["status": "running", "version": "0.1.0"]
    give back web.json(data)

web.start(port: 3000)
```

The ILMA version compiles to a **native binary** — no Node.js, no V8, no `node_modules`.

---

## Todo App

A more complete example — a full CRUD-style todo list with in-memory storage, HTML forms, and a JSON API.

### Source (`examples/todo_app/app.ilma`)

```ilma
use web

# In-memory todo storage
remember todos = bag[]
remember next_id = 1

# Home page — HTML with todo list
web.route("/"):
    remember html = "<html><head><title>ILMA Todo App</title></head><body>"
    html = html + "<h1>ILMA Todo List</h1>"
    html = html + "<form action='/todos/add' method='POST'>"
    html = html + "<input name='text' placeholder='New task...' required>"
    html = html + "<button type='submit'>Add</button></form><ul>"
    for each todo in todos:
        html = html + "<li>" + todo[text] + "</li>"
    html = html + "</ul><p><a href='/about'>About</a></p></body></html>"
    give back web.html(html)

# API: list all todos as JSON
web.route("/todos"):
    give back web.json(todos)

# Add a new todo
web.route("/todos/add"):
    remember text = web.query("text")
    if text is empty:
        text = "Untitled task"
    remember todo = notebook[id: next_id, text: text, done: no]
    todos.add(todo)
    next_id = next_id + 1
    give back web.html("<html><body><p>Added!</p><a href='/'>Back</a></body></html>")

# About page
web.route("/about"):
    give back web.html("<html><body><h1>About</h1><p>Built with IlmaWeb framework.</p><a href='/'>Home</a></body></html>")

# Health check
web.route("/health"):
    give back web.json(notebook[status: "ok", language: "ILMA"])

web.start(port: 3000)
```

### Routes

| Path          | Method | Description                     |
|---------------|--------|---------------------------------|
| `/`           | GET    | HTML page with todo list + form |
| `/todos`      | GET    | JSON array of all todos         |
| `/todos/add`  | POST   | Add a new todo from form data   |
| `/about`      | GET    | About page                      |
| `/health`     | GET    | JSON health check               |

### Build and run

```bash
cd frameworks/ilma-web
make build
# Or compile manually:
gcc -O2 -Wall -std=c11 -D_POSIX_C_SOURCE=200809L \
    -o build/todo_app \
    examples/todo_app/app_generated.c \
    src/ilma_http.c src/ilma_web_bridge.c \
    ../../src/runtime/ilma_runtime.c \
    -Isrc -I../../src/runtime -lm

./build/todo_app
```

Visit [http://localhost:3000](http://localhost:3000) to see the todo app.

---

## Database Support

IlmaWeb includes optional SQLite integration for persistent storage. Compile with `-DILMA_HAS_SQLITE -lsqlite3` to enable it.

### API

```ilma
# Open a database
remember db = db.open("app.db")

# Execute statements (CREATE, INSERT, UPDATE, DELETE)
db.exec(db, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT)")
db.exec(db, "INSERT INTO users (name) VALUES ('Zayd')")

# Query rows (SELECT) — returns a bag of notebooks
remember rows = db.query(db, "SELECT * FROM users")
for each row in rows:
    say row[name]

# Close the database
db.close(db)
```

### Without SQLite

If compiled without `-DILMA_HAS_SQLITE`, all database functions return an error message explaining how to enable SQLite support.

---

## Architecture

```
                        IlmaWeb compilation pipeline
  ┌─────────────┐     ┌────────┐     ┌────────┐     ┌──────────┐
  │  app.ilma   │────>│ Lexer  │────>│ Parser │────>│   AST    │
  │ (your code) │     │        │     │        │     │          │
  └─────────────┘     └────────┘     └────────┘     └────┬─────┘
                                                         │
                                                         v
                                                    ┌──────────┐
                                                    │ Codegen  │
                                                    │ (emit C) │
                                                    └────┬─────┘
                                                         │
                    ┌────────────────────────────────────┐│
                    │                                    ││
                    v                                    v│
             ┌──────────────┐  ┌──────────────────┐  ┌───v──────────┐
             │ ilma_http.c  │  │ ilma_runtime.c   │  │ app_gen.c    │
             │ (HTTP server)│  │ (ILMA runtime)   │  │ (your routes)│
             └──────┬───────┘  └────────┬─────────┘  └──────┬───────┘
                    │                   │                    │
                    └───────────┬───────┘────────────────────┘
                                │
                                v
                         ┌─────────────┐
                         │  GCC / link │
                         └──────┬──────┘
                                │
                                v
                        ┌───────────────┐
                        │ Native binary │  <-- your web server
                        │  (< 100 KB)   │
                        └───────────────┘
```

### How it works internally

1. **You write** `app.ilma` using the `web` module (`web.route`, `web.query`, `web.html`, etc.)

2. **The ILMA compiler** parses your code and generates C source (`app_generated.c`) that calls the bridge functions:
   - `web.route("/path")` becomes `ilma_web_add_route("/path", handler_fn)`
   - `web.query("key")` becomes `ilma_web_query("key")` (returns `IlmaValue`)
   - `web.html(val)` becomes `ilma_web_html_response(val)` (returns `IlmaHttpResponse`)
   - `web.json(val)` becomes `ilma_web_json_response(val)` (serialises IlmaValue to JSON)
   - `web.start(port: N)` becomes `ilma_web_init(N); ilma_web_start();`

3. **GCC compiles** the generated C together with `ilma_http.c` (the HTTP server), `ilma_web_bridge.c` (the bridge), and `ilma_runtime.c` (the ILMA runtime) into a single native binary.

4. **The binary runs** a POSIX TCP socket server that parses HTTP requests, matches routes, calls your handler functions, and sends back HTTP responses.

---

## License

Part of the ILMA project.
