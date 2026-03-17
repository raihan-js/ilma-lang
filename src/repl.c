#define _POSIX_C_SOURCE 200809L
#include "repl.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#endif

#define ILMA_VERSION    "0.5.0"
#define REPL_BUF_SIZE   16384
#define HISTORY_MAX     10

/* ── Ring-buffer history ──────────────────────────────── */
static char* g_history[HISTORY_MAX];
static int   g_hist_count = 0;
static int   g_hist_start = 0;   /* index of oldest entry */

static void history_add(const char* line) {
    if (g_hist_count < HISTORY_MAX) {
        g_history[g_hist_count++] = strdup(line);
    } else {
        free(g_history[g_hist_start]);
        g_history[g_hist_start] = strdup(line);
        g_hist_start = (g_hist_start + 1) % HISTORY_MAX;
    }
}

static void history_print(void) {
    int total = g_hist_count;
    for (int i = 0; i < total; i++) {
        int idx = (g_hist_start + i) % HISTORY_MAX;
        printf("  %2d  %s\n", i + 1, g_history[idx]);
    }
    if (total == 0) printf("  (no history yet)\n");
}

/* ── SIGINT handler ───────────────────────────────────── */
static volatile int g_sigint = 0;

static void sigint_handler(int sig) {
    (void)sig;
    g_sigint = 1;
}

/* ── Welcome banner ───────────────────────────────────── */
static void print_welcome(void) {
    printf("\033[1;32m");  /* bold green */
    printf("╔══════════════════════════════════════╗\n");
    printf("║  ILMA %-6s — ilma-lang.dev         ║\n", ILMA_VERSION);
    printf("║  Type 'help' for examples, 'quit'   ║\n");
    printf("║  to exit. Ctrl+C also works.        ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("\033[0m\n");
}

static void print_help(void) {
    printf("\nExamples:\n\n");
    printf("  say \"Hello, World!\"\n");
    printf("  remember name = ask \"Your name: \"\n");
    printf("  repeat 3: say \"SubhanAllah\"\n");
    printf("  recipe double(n): give back n * 2\n");
    printf("  use finance\n\n");
    printf("Commands: quit, exit, help, clear, reset, history\n\n");
}

/* ── Line reading (readline or plain) ────────────────── */
static char* read_line_repl(const char* prompt) {
#ifdef HAVE_READLINE
    char* line = readline(prompt);
    if (line && *line) add_history(line);
    return line;   /* caller must free() */
#else
    printf("%s", prompt);
    fflush(stdout);
    char* buf = malloc(REPL_BUF_SIZE);
    if (!buf) return NULL;
    if (!fgets(buf, REPL_BUF_SIZE, stdin)) {
        free(buf);
        return NULL;
    }
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return buf;
#endif
}

/* Returns 1 if line ends with ':' (block opener) */
static int is_block_opener(const char* line) {
    int i = (int)strlen(line) - 1;
    while (i >= 0 && (line[i] == ' ' || line[i] == '\t' ||
                       line[i] == '\n' || line[i] == '\r'))
        i--;
    return (i >= 0 && line[i] == ':');
}

/* ── Public entry point ───────────────────────────────── */
void repl_start(void) {
    signal(SIGINT, sigint_handler);
    print_welcome();

    Evaluator ev;
    evaluator_init(&ev);
    ev.capture_output = 0;
    ev.repl_mode      = 1;   /* auto-print bare expressions */

    char input_buf[REPL_BUF_SIZE * 4];

    while (1) {
        /* Check for pending Ctrl+C */
        if (g_sigint) {
            g_sigint = 0;
            printf("\n(Interrupt — type 'quit' to exit)\n");
        }

        char* line = read_line_repl("ilma> ");

        /* EOF (Ctrl+D) */
        if (!line) {
            printf("\nGoodbye!\n");
            break;
        }

        /* Trim trailing whitespace for command checking */
        char cmd[256];
        strncpy(cmd, line, sizeof(cmd) - 1);
        cmd[sizeof(cmd) - 1] = '\0';
        int clen = (int)strlen(cmd);
        while (clen > 0 && (cmd[clen-1] == ' ' || cmd[clen-1] == '\t')) {
            cmd[--clen] = '\0';
        }

        /* ── Special REPL commands ────────────────────── */
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            free(line);
            printf("Goodbye!\n");
            break;
        }
        if (strcmp(cmd, "help") == 0) {
            print_help();
            free(line);
            continue;
        }
        if (strcmp(cmd, "clear") == 0) {
            printf("\033[2J\033[H");
            fflush(stdout);
            free(line);
            continue;
        }
        if (strcmp(cmd, "reset") == 0) {
            evaluator_free(&ev);
            evaluator_init(&ev);
            ev.capture_output = 0;
            ev.repl_mode      = 1;
            printf("Environment reset.\n");
            free(line);
            continue;
        }
        if (strcmp(cmd, "history") == 0) {
            history_print();
            free(line);
            continue;
        }

        /* Skip blank lines */
        if (strlen(cmd) == 0) {
            free(line);
            continue;
        }

        /* ── Build multi-line input ───────────────────── */
        strncpy(input_buf, line, REPL_BUF_SIZE * 4 - 1);
        input_buf[REPL_BUF_SIZE * 4 - 1] = '\0';
        history_add(line);
        free(line);
        line = NULL;

        if (is_block_opener(input_buf)) {
            /* Collect indented continuation lines */
            while (1) {
                char* cont = read_line_repl("...> ");
                if (!cont) { break; }

                /* Empty line or reduced indent = end of block */
                int empty_cont = (strlen(cont) == 0);
                strncat(input_buf, "\n",
                        sizeof(input_buf) - strlen(input_buf) - 1);
                strncat(input_buf, cont,
                        sizeof(input_buf) - strlen(input_buf) - 1);

                int cont_is_block = is_block_opener(cont);
                free(cont);

                if (empty_cont) break;
                if (!cont_is_block) {
                    /* peek: if next line starts at base level, stop */
                    char* peek = read_line_repl("...> ");
                    if (!peek || strlen(peek) == 0) {
                        free(peek);
                        break;
                    }
                    if (peek[0] != ' ' && peek[0] != '\t') {
                        /* Non-indented — end block, but include as next stmt */
                        strncat(input_buf, "\n",
                                sizeof(input_buf) - strlen(input_buf) - 1);
                        strncat(input_buf, peek,
                                sizeof(input_buf) - strlen(input_buf) - 1);
                        free(peek);
                        break;
                    }
                    strncat(input_buf, "\n",
                            sizeof(input_buf) - strlen(input_buf) - 1);
                    strncat(input_buf, peek,
                            sizeof(input_buf) - strlen(input_buf) - 1);
                    free(peek);
                }
            }
        }

        /* ── Lex → Parse → Evaluate ─────────────────── */
        Lexer lexer;
        lexer_init(&lexer, input_buf);
        lexer_tokenize(&lexer);

        Parser parser;
        parser_init(&parser, lexer.tokens, lexer.token_count);
        ASTNode* program = parser_parse(&parser);

        if (program) {
            evaluator_run(&ev, program);
            /* Do NOT free program — evaluator may hold recipe/blueprint refs */
        }

        lexer_free(&lexer);
    }

    evaluator_free(&ev);

    /* Free history ring buffer */
    for (int i = 0; i < g_hist_count; i++) {
        free(g_history[(g_hist_start + i) % HISTORY_MAX]);
    }
}
