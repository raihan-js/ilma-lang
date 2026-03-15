#include "ilma_lsp.h"
#include "../lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ── Document cache ────────────────────────────────────────────── */

#define LSP_MAX_DOCS 32

static struct {
    char  uri[512];
    char* content;
} lsp_docs[LSP_MAX_DOCS];

static int lsp_doc_count = 0;

static int lsp_find_doc(const char* uri) {
    for (int i = 0; i < lsp_doc_count; i++) {
        if (strcmp(lsp_docs[i].uri, uri) == 0)
            return i;
    }
    return -1;
}

static void lsp_store_doc(const char* uri, const char* content) {
    int idx = lsp_find_doc(uri);
    if (idx < 0) {
        if (lsp_doc_count >= LSP_MAX_DOCS) return; /* full */
        idx = lsp_doc_count++;
        strncpy(lsp_docs[idx].uri, uri, sizeof(lsp_docs[idx].uri) - 1);
        lsp_docs[idx].uri[sizeof(lsp_docs[idx].uri) - 1] = '\0';
    } else {
        free(lsp_docs[idx].content);
    }
    lsp_docs[idx].content = strdup(content);
}

static void lsp_remove_doc(const char* uri) {
    int idx = lsp_find_doc(uri);
    if (idx < 0) return;
    free(lsp_docs[idx].content);
    lsp_docs[idx].content = NULL;
    /* Shift remaining docs down */
    for (int i = idx; i < lsp_doc_count - 1; i++) {
        lsp_docs[i] = lsp_docs[i + 1];
    }
    lsp_doc_count--;
}

/* ── Minimal JSON helpers ──────────────────────────────────────── */

/*
 * Extract a string value for a given key from a JSON object.
 * Returns a malloc'd string or NULL. Handles escaped quotes.
 * Searches for "key":"value" pattern.
 */
static char* json_get_string(const char* json, const char* key) {
    /* Build the search pattern: "key" */
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = strstr(json, pattern);
    if (!p) return NULL;

    p += strlen(pattern);

    /* Skip whitespace and colon */
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    if (*p != '"') return NULL;
    p++; /* skip opening quote */

    /* Collect characters until unescaped closing quote */
    size_t cap = 256;
    size_t len = 0;
    char* buf = malloc(cap);

    while (*p && !(*p == '"' && (len == 0 || buf[len-1] != '\\'))) {
        if (len + 2 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        /* Handle escape sequences */
        if (*p == '\\' && *(p+1)) {
            p++; /* skip backslash */
            switch (*p) {
                case '"':  buf[len++] = '"';  break;
                case '\\': buf[len++] = '\\'; break;
                case 'n':  buf[len++] = '\n'; break;
                case 't':  buf[len++] = '\t'; break;
                case 'r':  buf[len++] = '\r'; break;
                default:   buf[len++] = *p;   break;
            }
        } else {
            buf[len++] = *p;
        }
        p++;
    }
    buf[len] = '\0';
    return buf;
}

/*
 * Extract an integer value for a given key.
 * Returns the value; sets *found to 1 if found, 0 otherwise.
 */
static int json_get_int(const char* json, const char* key, int* found) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = strstr(json, pattern);
    if (!p) { if (found) *found = 0; return 0; }

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') { if (found) *found = 0; return 0; }
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    if (found) *found = 1;
    return atoi(p);
}

/*
 * Extract a nested JSON object value for a given key.
 * Returns a malloc'd string containing the object (with braces) or NULL.
 * Handles nested braces.
 */
static char* json_get_object(const char* json, const char* key) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char* p = strstr(json, pattern);
    if (!p) return NULL;

    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    if (*p != '{') return NULL;

    /* Find matching closing brace */
    int depth = 0;
    const char* start = p;
    int in_string = 0;

    while (*p) {
        if (*p == '\\' && in_string) {
            p++; /* skip escaped char */
            if (*p) p++;
            continue;
        }
        if (*p == '"') in_string = !in_string;
        if (!in_string) {
            if (*p == '{') depth++;
            if (*p == '}') { depth--; if (depth == 0) { p++; break; } }
        }
        p++;
    }

    size_t len = (size_t)(p - start);
    char* result = malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

/*
 * Escape a string for JSON output.
 * Returns a malloc'd string with special characters escaped.
 */
static char* json_escape(const char* s) {
    if (!s) return strdup("");

    size_t cap = strlen(s) * 2 + 1;
    char* buf = malloc(cap);
    size_t len = 0;

    while (*s) {
        if (len + 8 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        switch (*s) {
            case '"':  buf[len++] = '\\'; buf[len++] = '"';  break;
            case '\\': buf[len++] = '\\'; buf[len++] = '\\'; break;
            case '\n': buf[len++] = '\\'; buf[len++] = 'n';  break;
            case '\r': buf[len++] = '\\'; buf[len++] = 'r';  break;
            case '\t': buf[len++] = '\\'; buf[len++] = 't';  break;
            default:   buf[len++] = *s;                       break;
        }
        s++;
    }
    buf[len] = '\0';
    return buf;
}

/* ── LSP I/O ───────────────────────────────────────────────────── */

char* lsp_read_message(void) {
    /* Read headers until blank line */
    int content_length = -1;
    char header_line[1024];

    while (1) {
        if (!fgets(header_line, sizeof(header_line), stdin))
            return NULL;

        /* Check for Content-Length header */
        if (strncmp(header_line, "Content-Length:", 15) == 0) {
            content_length = atoi(header_line + 15);
        }

        /* Blank line (just \r\n or \n) marks end of headers */
        if (strcmp(header_line, "\r\n") == 0 || strcmp(header_line, "\n") == 0)
            break;
    }

    if (content_length <= 0)
        return NULL;

    /* Read exactly content_length bytes */
    char* body = malloc(content_length + 1);
    size_t bytes_read = fread(body, 1, content_length, stdin);
    body[bytes_read] = '\0';

    if ((int)bytes_read < content_length) {
        /* Partial read; still return what we got */
        body[bytes_read] = '\0';
    }

    return body;
}

void lsp_send_response(int id, const char* result_json) {
    char* body = NULL;
    int body_len;

    if (result_json) {
        body_len = snprintf(NULL, 0,
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, result_json);
        body = malloc(body_len + 1);
        snprintf(body, body_len + 1,
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}", id, result_json);
    } else {
        body_len = snprintf(NULL, 0,
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}", id);
        body = malloc(body_len + 1);
        snprintf(body, body_len + 1,
            "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":null}", id);
    }

    fprintf(stdout, "Content-Length: %d\r\n\r\n%s", body_len, body);
    fflush(stdout);
    free(body);
}

void lsp_send_notification(const char* method, const char* params_json) {
    char* body = NULL;
    int body_len;

    if (params_json) {
        body_len = snprintf(NULL, 0,
            "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}",
            method, params_json);
        body = malloc(body_len + 1);
        snprintf(body, body_len + 1,
            "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}",
            method, params_json);
    } else {
        body_len = snprintf(NULL, 0,
            "{\"jsonrpc\":\"2.0\",\"method\":\"%s\"}", method);
        body = malloc(body_len + 1);
        snprintf(body, body_len + 1,
            "{\"jsonrpc\":\"2.0\",\"method\":\"%s\"}", method);
    }

    fprintf(stdout, "Content-Length: %d\r\n\r\n%s", body_len, body);
    fflush(stdout);
    free(body);
}

void lsp_send_diagnostics(const char* uri, const char* diagnostics_json) {
    char* escaped_uri = json_escape(uri);

    char* params = NULL;
    int params_len = snprintf(NULL, 0,
        "{\"uri\":\"%s\",\"diagnostics\":%s}",
        escaped_uri, diagnostics_json);
    params = malloc(params_len + 1);
    snprintf(params, params_len + 1,
        "{\"uri\":\"%s\",\"diagnostics\":%s}",
        escaped_uri, diagnostics_json);

    lsp_send_notification("textDocument/publishDiagnostics", params);
    free(params);
    free(escaped_uri);
}

/* ── Keyword documentation ─────────────────────────────────────── */

const char* lsp_get_keyword_docs(const char* word) {
    if (!word) return NULL;

    if (strcmp(word, "say") == 0)
        return "**say** — Print a value to the screen\n\n```ilma\nsay \"Hello!\"\nsay 2 + 2\n```";
    if (strcmp(word, "remember") == 0)
        return "**remember** — Create a variable\n\n```ilma\nremember name = \"Yusuf\"\nremember age = 12\n```";
    if (strcmp(word, "recipe") == 0)
        return "**recipe** — Define a function\n\n```ilma\nrecipe greet(name):\n    say \"Salaam, \" + name\n```";
    if (strcmp(word, "blueprint") == 0)
        return "**blueprint** — Define a class\n\n```ilma\nblueprint Animal:\n    create(name):\n        me.name = name\n```";
    if (strcmp(word, "give") == 0)
        return "**give back** — Return a value from a recipe\n\n```ilma\nrecipe double(n):\n    give back n * 2\n```";
    if (strcmp(word, "bag") == 0)
        return "**bag** — An ordered list\n\n```ilma\nremember fruits = bag[\"dates\", \"mango\"]\n```";
    if (strcmp(word, "notebook") == 0)
        return "**notebook** — A key-value dictionary\n\n```ilma\nremember person = notebook[name: \"Yusuf\", age: 12]\n```";
    if (strcmp(word, "repeat") == 0)
        return "**repeat** — Loop a fixed number of times\n\n```ilma\nrepeat 5:\n    say \"SubhanAllah\"\n```";
    if (strcmp(word, "for") == 0)
        return "**for each** — Iterate over a collection\n\n```ilma\nfor each item in fruits:\n    say item\n```";
    if (strcmp(word, "if") == 0)
        return "**if** — Conditional execution\n\n```ilma\nif age >= 18:\n    say \"Adult\"\notherwise:\n    say \"Child\"\n```";
    if (strcmp(word, "try") == 0)
        return "**try / when wrong** — Error handling\n\n```ilma\ntry:\n    risky_thing()\nwhen wrong:\n    say \"Something went wrong\"\n```";
    if (strcmp(word, "use") == 0)
        return "**use** — Import a module\n\n```ilma\nuse finance\nremember z = finance.zakat(5000, 595)\n```";
    if (strcmp(word, "yes") == 0)
        return "**yes** — Boolean true value";
    if (strcmp(word, "no") == 0)
        return "**no** — Boolean false value";
    if (strcmp(word, "empty") == 0)
        return "**empty** — Null/nothing value";
    if (strcmp(word, "me") == 0)
        return "**me** — Reference to the current object inside a blueprint";
    if (strcmp(word, "shout") == 0)
        return "**shout** — Throw an error\n\n```ilma\nshout \"Something went wrong!\"\n```";
    if (strcmp(word, "ask") == 0)
        return "**ask** — Read input from the user\n\n```ilma\nremember name = ask \"What is your name? \"\n```";
    if (strcmp(word, "otherwise") == 0)
        return "**otherwise** — Else branch of an if statement\n\n```ilma\nif age >= 18:\n    say \"Adult\"\notherwise:\n    say \"Child\"\n```";
    if (strcmp(word, "keep") == 0)
        return "**keep going while** — Loop while a condition is true\n\n```ilma\nkeep going while x < 10:\n    say x\n    remember x = x + 1\n```";
    if (strcmp(word, "each") == 0)
        return "**for each** — Iterate over a collection\n\n```ilma\nfor each item in fruits:\n    say item\n```";
    if (strcmp(word, "in") == 0)
        return "**in** — Used with for each loops\n\n```ilma\nfor each item in fruits:\n    say item\n```";
    if (strcmp(word, "back") == 0)
        return "**give back** — Return a value from a recipe\n\n```ilma\nrecipe double(n):\n    give back n * 2\n```";
    if (strcmp(word, "create") == 0)
        return "**create** — Constructor inside a blueprint\n\n```ilma\nblueprint Animal:\n    create(name):\n        me.name = name\n```";
    if (strcmp(word, "when") == 0)
        return "**when wrong** — Catch errors from a try block\n\n```ilma\ntry:\n    risky_thing()\nwhen wrong:\n    say \"Something went wrong\"\n```";

    return NULL;
}

/* ── Diagnostics (token-level validation) ──────────────────────── */

/*
 * Validate a document by running the lexer in a subprocess.
 * The lexer calls exit(1) on errors, so we fork to avoid killing the LSP.
 */
static void lsp_validate_document(const char* uri, const char* content) {
    /*
     * MVP approach: run the lexer in-process, but redirect stderr to
     * capture error messages, and fork so exit(1) doesn't kill us.
     */
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        /* Can't create pipe — send empty diagnostics */
        lsp_send_diagnostics(uri, "[]");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        /* Fork failed — send empty diagnostics */
        close(pipefd[0]);
        close(pipefd[1]);
        lsp_send_diagnostics(uri, "[]");
        return;
    }

    if (pid == 0) {
        /* Child process: run lexer, stderr goes to pipe */
        close(pipefd[0]); /* close read end */
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        Lexer lexer;
        lexer_init(&lexer, content);
        lexer_tokenize(&lexer);
        lexer_free(&lexer);

        /* If we get here, lexer succeeded */
        _exit(0);
    }

    /* Parent process: read child's stderr */
    close(pipefd[1]); /* close write end */

    char error_buf[4096];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(pipefd[0], error_buf + total,
                     sizeof(error_buf) - 1 - total)) > 0) {
        total += n;
    }
    error_buf[total] = '\0';
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && total > 0) {
        /* Lexer failed — extract line number from error message */
        int error_line = 0;
        const char* line_str = strstr(error_buf, "line ");
        if (line_str) {
            error_line = atoi(line_str + 5);
        }
        if (error_line > 0) error_line--; /* LSP lines are 0-based */

        /* Escape the error message for JSON */
        char* escaped_msg = json_escape(error_buf);
        /* Strip trailing newline from escaped message */
        size_t elen = strlen(escaped_msg);
        while (elen > 0 && (escaped_msg[elen-1] == 'n' && elen >= 2 && escaped_msg[elen-2] == '\\')) {
            escaped_msg[elen-2] = '\0';
            elen -= 2;
        }

        char diag[8192];
        snprintf(diag, sizeof(diag),
            "[{\"range\":{\"start\":{\"line\":%d,\"character\":0},"
            "\"end\":{\"line\":%d,\"character\":999}},"
            "\"severity\":1,"
            "\"source\":\"ilma\","
            "\"message\":\"%s\"}]",
            error_line, error_line, escaped_msg);

        lsp_send_diagnostics(uri, diag);
        free(escaped_msg);
    } else {
        /* No errors — clear diagnostics */
        lsp_send_diagnostics(uri, "[]");
    }
}

/* ── Hover support ─────────────────────────────────────────────── */

/*
 * Extract the word at a given line and character position from text.
 * Returns a malloc'd string or NULL.
 */
static char* get_word_at_position(const char* text, int line, int character) {
    if (!text) return NULL;

    /* Find the start of the requested line */
    const char* p = text;
    for (int i = 0; i < line && *p; i++) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }

    if (!*p) return NULL;

    /* Find the line end */
    const char* line_start = p;
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;

    int line_len = (int)(line_end - line_start);
    if (character >= line_len) return NULL;

    /* Find word boundaries around the character position */
    int start = character;
    int end = character;

    while (start > 0) {
        unsigned char c = (unsigned char)line_start[start - 1];
        if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c >= 0x80))
            break;
        start--;
    }
    while (end < line_len) {
        unsigned char c = (unsigned char)line_start[end];
        if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c >= 0x80))
            break;
        end++;
    }

    if (start == end) return NULL;

    int word_len = end - start;
    char* word = malloc(word_len + 1);
    memcpy(word, line_start + start, word_len);
    word[word_len] = '\0';
    return word;
}

/* ── Message dispatch ──────────────────────────────────────────── */

int lsp_process_message(const char* message) {
    char* method = json_get_string(message, "method");
    int id_found = 0;
    int id = json_get_int(message, "id", &id_found);

    if (!method) {
        free(method);
        return 0; /* ignore messages without method */
    }

    /* ── initialize ── */
    if (strcmp(method, "initialize") == 0) {
        lsp_send_response(id,
            "{\"capabilities\":{"
            "\"textDocumentSync\":1,"
            "\"hoverProvider\":true"
            "},"
            "\"serverInfo\":{"
            "\"name\":\"ilma-lsp\","
            "\"version\":\"0.3.0\""
            "}}");
        free(method);
        return 0;
    }

    /* ── initialized ── */
    if (strcmp(method, "initialized") == 0) {
        /* No response needed */
        free(method);
        return 0;
    }

    /* ── textDocument/didOpen ── */
    if (strcmp(method, "textDocument/didOpen") == 0) {
        char* td = json_get_object(message, "textDocument");
        if (td) {
            char* uri = json_get_string(td, "uri");
            char* text = json_get_string(td, "text");
            if (uri && text) {
                lsp_store_doc(uri, text);
                lsp_validate_document(uri, text);
            }
            free(uri);
            free(text);
            free(td);
        }
        free(method);
        return 0;
    }

    /* ── textDocument/didChange ── */
    if (strcmp(method, "textDocument/didChange") == 0) {
        /*
         * With textDocumentSync:1 (Full), the full text is sent in
         * contentChanges[0].text. We search for "text" inside the
         * contentChanges array.
         */
        char* td = json_get_object(message, "textDocument");
        char* uri = NULL;
        if (td) {
            uri = json_get_string(td, "uri");
            free(td);
        }

        /* Find the "text" field inside contentChanges */
        const char* cc = strstr(message, "contentChanges");
        char* text = NULL;
        if (cc) {
            text = json_get_string(cc, "text");
        }

        if (uri && text) {
            lsp_store_doc(uri, text);
            lsp_validate_document(uri, text);
        }
        free(uri);
        free(text);
        free(method);
        return 0;
    }

    /* ── textDocument/didClose ── */
    if (strcmp(method, "textDocument/didClose") == 0) {
        char* td = json_get_object(message, "textDocument");
        if (td) {
            char* uri = json_get_string(td, "uri");
            if (uri) {
                /* Clear diagnostics and remove from cache */
                lsp_send_diagnostics(uri, "[]");
                lsp_remove_doc(uri);
            }
            free(uri);
            free(td);
        }
        free(method);
        return 0;
    }

    /* ── textDocument/hover ── */
    if (strcmp(method, "textDocument/hover") == 0) {
        char* td = json_get_object(message, "textDocument");
        char* pos = json_get_object(message, "position");
        char* uri = NULL;
        int line = 0, character = 0;

        if (td) {
            uri = json_get_string(td, "uri");
            free(td);
        }
        if (pos) {
            int f;
            line = json_get_int(pos, "line", &f);
            character = json_get_int(pos, "character", &f);
            free(pos);
        }

        /* Look up document content */
        const char* content = NULL;
        if (uri) {
            int idx = lsp_find_doc(uri);
            if (idx >= 0)
                content = lsp_docs[idx].content;
        }

        /* Extract word at position */
        char* word = get_word_at_position(content, line, character);
        const char* docs = lsp_get_keyword_docs(word);

        if (docs) {
            char* escaped = json_escape(docs);
            char result[8192];
            snprintf(result, sizeof(result),
                "{\"contents\":{\"kind\":\"markdown\",\"value\":\"%s\"}}",
                escaped);
            lsp_send_response(id, result);
            free(escaped);
        } else {
            lsp_send_response(id, "null");
        }

        free(word);
        free(uri);
        free(method);
        return 0;
    }

    /* ── shutdown ── */
    if (strcmp(method, "shutdown") == 0) {
        lsp_send_response(id, "null");
        free(method);
        return 0;
    }

    /* ── exit ── */
    if (strcmp(method, "exit") == 0) {
        free(method);
        return 1;
    }

    /* Unknown method — ignore */
    free(method);
    return 0;
}

/* ── Main event loop ───────────────────────────────────────────── */

void lsp_run(void) {
    while (1) {
        char* msg = lsp_read_message();
        if (!msg) break;
        if (lsp_process_message(msg)) {
            free(msg);
            break;
        }
        free(msg);
    }

    /* Cleanup document cache */
    for (int i = 0; i < lsp_doc_count; i++) {
        free(lsp_docs[i].content);
        lsp_docs[i].content = NULL;
    }
    lsp_doc_count = 0;
}
