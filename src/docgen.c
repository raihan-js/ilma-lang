#include "docgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_DOC_ITEMS 256

typedef struct {
    char* name;
    char* kind;   /* "recipe" or "blueprint" */
    char* doc;    /* accumulated doc comment */
} DocItem;

static DocItem g_items[MAX_DOC_ITEMS];
static int     g_count = 0;

static void strip_newline(char* s) {
    size_t l = strlen(s);
    while (l > 0 && (s[l-1] == '\n' || s[l-1] == '\r')) s[--l] = '\0';
}

int ilma_doc_generate(const char* source, const char* filename, const char* output_path) {
    g_count = 0;

    /* Split source into lines */
    char* copy = strdup(source);
    char* lines[8192];
    int nlines = 0;
    char* ptr = copy;
    while (*ptr && nlines < 8191) {
        lines[nlines++] = ptr;
        char* nl = strchr(ptr, '\n');
        if (nl) { *nl = '\0'; ptr = nl + 1; }
        else break;
    }

    /* Scan for doc comments + recipe/blueprint */
    char doc_buf[4096] = "";
    int in_doc = 0;

    for (int i = 0; i < nlines && g_count < MAX_DOC_ITEMS; i++) {
        char* line = lines[i];
        strip_newline(line);
        /* Trim leading spaces */
        while (*line == ' ' || *line == '\t') line++;

        if (strncmp(line, "###", 3) == 0) {
            /* Doc comment line */
            const char* text = line + 3;
            while (*text == ' ') text++;
            if (in_doc) {
                strncat(doc_buf, " ", sizeof(doc_buf) - strlen(doc_buf) - 1);
            } else {
                doc_buf[0] = '\0';
                in_doc = 1;
            }
            strncat(doc_buf, text, sizeof(doc_buf) - strlen(doc_buf) - 1);
        } else if (in_doc) {
            /* Check if this is a recipe or blueprint line */
            const char* kind = NULL;
            const char* name_start = NULL;
            if (strncmp(line, "recipe ", 7) == 0) {
                kind = "recipe";
                name_start = line + 7;
            } else if (strncmp(line, "blueprint ", 10) == 0) {
                kind = "blueprint";
                name_start = line + 10;
            }

            if (kind && name_start) {
                /* Extract name (up to first non-ident char) */
                char name[256] = "";
                int j = 0;
                while (name_start[j] && (isalnum((unsigned char)name_start[j]) || name_start[j] == '_') && j < 255) {
                    name[j] = name_start[j];
                    j++;
                }
                name[j] = '\0';

                g_items[g_count].name = strdup(name);
                g_items[g_count].kind = strdup(kind);
                g_items[g_count].doc  = strdup(doc_buf);
                g_count++;
            }
            in_doc = 0;
            doc_buf[0] = '\0';
        }
    }

    free(copy);

    /* Generate HTML */
    FILE* out = stdout;
    int write_to_file = output_path && strlen(output_path) > 0;
    if (write_to_file) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "ilma doc: cannot write to '%s'\n", output_path);
            return 1;
        }
    }

    fprintf(out, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(out, "<meta charset='utf-8'>\n");
    fprintf(out, "<title>ILMA Docs &mdash; %s</title>\n", filename);
    fprintf(out, "<style>\n");
    fprintf(out, "body{font-family:sans-serif;max-width:800px;margin:2rem auto;padding:0 1rem;}\n");
    fprintf(out, "h1{color:#2d6a4f;}h2{color:#1b4332;border-bottom:2px solid #95d5b2;padding-bottom:4px;}\n");
    fprintf(out, ".kind{background:#d8f3dc;color:#1b4332;padding:2px 8px;border-radius:4px;font-size:.85em;}\n");
    fprintf(out, ".doc{color:#444;margin:.5rem 0;}\n");
    fprintf(out, "</style>\n</head>\n<body>\n");
    fprintf(out, "<h1>%s</h1>\n", filename);

    if (g_count == 0) {
        fprintf(out, "<p>No documented items found. Add <code>###</code> doc comments above recipe and blueprint definitions.</p>\n");
    }

    for (int i = 0; i < g_count; i++) {
        fprintf(out, "<div style='margin:1.5rem 0;'>\n");
        fprintf(out, "<h2><span class='kind'>%s</span> %s</h2>\n",
                g_items[i].kind, g_items[i].name);
        fprintf(out, "<p class='doc'>%s</p>\n", g_items[i].doc);
        fprintf(out, "</div>\n");
        free(g_items[i].name);
        free(g_items[i].kind);
        free(g_items[i].doc);
    }

    fprintf(out, "</body>\n</html>\n");

    if (write_to_file) fclose(out);

    return 0;
}
