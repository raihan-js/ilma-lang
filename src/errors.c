#define _POSIX_C_SOURCE 200809L
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ── ANSI colour helpers ──────────────────────────────── */
static int use_color(void) {
    return isatty(fileno(stdout));
}

#define COL_RED     "\033[1;31m"
#define COL_CYAN    "\033[1;36m"
#define COL_YELLOW  "\033[1;33m"
#define COL_BOLD    "\033[1m"
#define COL_RESET   "\033[0m"

static void emit(const char* ansi, const char* text) {
    if (use_color()) fputs(ansi, stdout);
    fputs(text, stdout);
    if (use_color()) fputs(COL_RESET, stdout);
}

/* ── Public API ───────────────────────────────────────── */

void ilma_error(SourceMap* map, int line, int col,
                const char* message, const char* hint,
                const char* example) {
    /* "error: <message>" */
    if (use_color()) fputs(COL_RED, stdout);
    fputs("error: ", stdout);
    if (use_color()) fputs(COL_BOLD, stdout);
    fputs(message ? message : "(unknown error)", stdout);
    if (use_color()) fputs(COL_RESET, stdout);
    fputc('\n', stdout);

    /* " --> filename:line:col" */
    if (use_color()) fputs(COL_CYAN, stdout);
    printf(" --> %s:%d:%d\n",
           map && map->filename ? map->filename : "<input>",
           line, col);
    if (use_color()) fputs(COL_RESET, stdout);

    /* Source line */
    if (map && line >= 1 && line <= map->line_count) {
        const char* src_line = map->source_lines[line - 1];
        printf("%4d | %s\n", line, src_line ? src_line : "");
        /* Caret line */
        printf("     | ");
        int spaces = (col > 1) ? col - 1 : 0;
        for (int i = 0; i < spaces; i++) fputc(' ', stdout);
        if (use_color()) fputs(COL_RED, stdout);
        fputc('^', stdout);
        if (hint) { fputc(' ', stdout); fputs(hint, stdout); }
        if (use_color()) fputs(COL_RESET, stdout);
        fputc('\n', stdout);
    } else if (hint) {
        /* No source map — just show hint */
        printf("hint: %s\n", hint);
    }

    /* Example block */
    if (example) {
        printf("example:\n");
        /* Indent each line of the example by 2 spaces */
        const char* p = example;
        while (*p) {
            printf("  ");
            while (*p && *p != '\n') { fputc(*p++, stdout); }
            fputc('\n', stdout);
            if (*p == '\n') p++;
        }
    }

    fputc('\n', stdout);
}

void ilma_warning(SourceMap* map, int line, int col,
                  const char* message, const char* hint) {
    if (use_color()) fputs(COL_YELLOW, stdout);
    fputs("warning: ", stdout);
    if (use_color()) fputs(COL_BOLD, stdout);
    fputs(message ? message : "(warning)", stdout);
    if (use_color()) fputs(COL_RESET, stdout);
    fputc('\n', stdout);

    if (use_color()) fputs(COL_CYAN, stdout);
    printf(" --> %s:%d:%d\n",
           map && map->filename ? map->filename : "<input>",
           line, col);
    if (use_color()) fputs(COL_RESET, stdout);

    if (map && line >= 1 && line <= map->line_count) {
        const char* src_line = map->source_lines[line - 1];
        printf("%4d | %s\n", line, src_line ? src_line : "");
        if (hint) {
            printf("     | ");
            printf("hint: %s\n", hint);
        }
    }
    fputc('\n', stdout);
}

/* ── SourceMap helpers ────────────────────────────────── */

SourceMap sourcemap_build(const char* source, const char* filename) {
    SourceMap map;
    map.filename = filename ? filename : "<input>";
    map.source_lines = NULL;
    map.line_count = 0;

    if (!source) return map;

    /* Count lines */
    int count = 1;
    for (const char* p = source; *p; p++) {
        if (*p == '\n') count++;
    }

    map.source_lines = calloc(count, sizeof(char*));
    map.line_count = 0;

    /* Split into lines */
    const char* start = source;
    const char* p = source;
    int idx = 0;
    while (1) {
        if (*p == '\n' || *p == '\0') {
            int len = (int)(p - start);
            char* ln = malloc(len + 1);
            memcpy(ln, start, len);
            ln[len] = '\0';
            map.source_lines[idx++] = ln;
            map.line_count++;
            if (*p == '\0') break;
            start = p + 1;
        }
        p++;
    }

    return map;
}

void sourcemap_free(SourceMap* map) {
    if (!map) return;
    for (int i = 0; i < map->line_count; i++) {
        free((char*)map->source_lines[i]);
    }
    free(map->source_lines);
    map->source_lines = NULL;
    map->line_count = 0;
}
