#include "repl.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ILMA_VERSION "0.5.0"
#define REPL_BUF_SIZE 8192

static void print_welcome(void) {
    printf("ILMA %s — ilma-lang.dev\n", ILMA_VERSION);
    printf("Type ILMA code and press Enter. Type 'quit' to exit.\n");
    printf("Type 'help' to see examples.\n");
    printf("Note: Syntax errors will exit the REPL in this version.\n\n");
}

static void print_help(void) {
    printf("\nQuick examples:\n\n");
    printf("  say \"Bismillah\"                    # print text\n");
    printf("  remember x = 42                    # create a variable\n");
    printf("  say \"Value is {x}\"                 # string interpolation\n");
    printf("  recipe double(n):                  # define a function\n");
    printf("      give back n * 2\n");
    printf("  say double(21)                     # call it: 42\n");
    printf("\nCommands: quit, exit, help, clear, reset\n\n");
}

static int is_block_opener(const char* line) {
    /* Check if line ends with : (ignoring trailing whitespace and comments) */
    int len = strlen(line);
    int i = len - 1;
    while (i >= 0 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r'))
        i--;
    return (i >= 0 && line[i] == ':');
}

static char* read_line(const char* prompt) {
    printf("%s", prompt);
    fflush(stdout);

    char* buf = malloc(REPL_BUF_SIZE);
    if (!buf) return NULL;
    if (!fgets(buf, REPL_BUF_SIZE, stdin)) {
        free(buf);
        return NULL;
    }
    /* Remove trailing newline */
    int len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return buf;
}

void repl_start(void) {
    print_welcome();

    Evaluator ev;
    evaluator_init(&ev);
    ev.capture_output = 0; /* Print directly to stdout */

    char input_buf[REPL_BUF_SIZE * 4];

    while (1) {
        char* line = read_line("ilma> ");
        if (!line) {
            printf("\n");
            break; /* EOF / Ctrl+D */
        }

        /* Special commands */
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            free(line);
            break;
        }
        if (strcmp(line, "help") == 0) {
            print_help();
            free(line);
            continue;
        }
        if (strcmp(line, "clear") == 0) {
            printf("\033[2J\033[H"); /* ANSI clear screen */
            free(line);
            continue;
        }
        if (strcmp(line, "reset") == 0) {
            evaluator_free(&ev);
            evaluator_init(&ev);
            ev.capture_output = 0;
            printf("Environment reset.\n");
            free(line);
            continue;
        }

        /* Skip empty lines */
        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        /* Start building the input */
        strncpy(input_buf, line, sizeof(input_buf) - 1);
        input_buf[sizeof(input_buf) - 1] = '\0';

        /* If line ends with :, enter multi-line mode */
        if (is_block_opener(line)) {
            free(line);
            line = NULL;
            while (1) {
                char* cont = read_line("...> ");
                if (!cont || strlen(cont) == 0) {
                    free(cont);
                    break;
                }
                strncat(input_buf, "\n", sizeof(input_buf) - strlen(input_buf) - 1);
                strncat(input_buf, cont, sizeof(input_buf) - strlen(input_buf) - 1);

                /* Check if this continuation line also ends with : */
                int ends_with_colon = is_block_opener(cont);
                free(cont);

                if (!ends_with_colon) {
                    /* Peek at the next line */
                    cont = read_line("...> ");
                    if (!cont || strlen(cont) == 0) {
                        free(cont);
                        break;
                    }
                    /* If it starts with spaces/tabs, it's still part of the block */
                    if (cont[0] == ' ' || cont[0] == '\t') {
                        strncat(input_buf, "\n", sizeof(input_buf) - strlen(input_buf) - 1);
                        strncat(input_buf, cont, sizeof(input_buf) - strlen(input_buf) - 1);
                        free(cont);
                    } else {
                        /* Not indented — end of block, but include as new statement */
                        strncat(input_buf, "\n", sizeof(input_buf) - strlen(input_buf) - 1);
                        strncat(input_buf, cont, sizeof(input_buf) - strlen(input_buf) - 1);
                        free(cont);
                        break;
                    }
                }
            }
        } else {
            free(line);
            line = NULL;
        }

        /* Lex and parse */
        Lexer lexer;
        lexer_init(&lexer, input_buf);
        lexer_tokenize(&lexer);

        Parser parser;
        parser_init(&parser, lexer.tokens, lexer.token_count);
        ASTNode* program = parser_parse(&parser);

        if (program) {
            evaluator_run(&ev, program);
            /* Don't free the program — the evaluator may reference recipe/blueprint nodes */
        }

        lexer_free(&lexer);
    }

    evaluator_free(&ev);
    printf("Goodbye!\n");
}
