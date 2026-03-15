#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "pkg/ilma_pkg.h"

#define ILMA_VERSION "0.4.0"

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Oops! I couldn't open the file '%s'.\n"
                        "Make sure the file exists and the name is spelled correctly.\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(size + 1);
    if (!buf) { fprintf(stderr, "Oops! Out of memory.\n"); exit(1); }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

static void print_usage(void) {
    printf("ILMA %s — ilma-lang.dev\n\n", ILMA_VERSION);
    printf("Usage:\n");
    printf("  ilma <file.ilma>              Run an ILMA program\n");
    printf("  ilma --compile <file.ilma>    Compile to a binary\n");
    printf("  ilma --c <file.ilma>          Show the generated C code\n");
    printf("  ilma --tokens <file.ilma>     Show tokens (for debugging)\n");
    printf("  ilma --version                Show version\n");
    printf("  ilma --help                   Show this message\n\n");
    printf("Examples:\n");
    printf("  ilma hello.ilma\n");
    printf("  ilma --compile myapp.ilma\n\n");
}

static void print_tokens(Token* tokens, int count) {
    for (int i = 0; i < count; i++) {
        Token* t = &tokens[i];
        printf("%-4d:%-3d  %-18s", t->line, t->col, token_type_name(t->type));
        if (t->value) printf("  \"%s\"", t->value);
        printf("\n");
    }
}

/* Resolve the runtime path relative to the installed compiler binary */
static void find_runtime(char* out, size_t out_size) {
    /* 1. User-set ILMA_HOME env var */
    const char* home = getenv("ILMA_HOME");
    if (home) {
        snprintf(out, out_size, "%s/runtime", home);
        return;
    }

    /* 2. Relative to the compiler binary */
    char exe[1024] = {0};
#ifdef __linux__
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len > 0) {
        exe[len] = '\0';
        char* slash = strrchr(exe, '/');
        if (slash) {
            *slash = '\0';
            /* Binary in /usr/local/bin → runtime in /usr/local/lib/ilma/runtime */
            snprintf(out, out_size, "%s/../lib/ilma/runtime", exe);
            /* Check if it exists there */
            char probe[1024];
            snprintf(probe, sizeof(probe), "%s/ilma_runtime.h", out);
            if (access(probe, F_OK) == 0) return;
            /* Dev mode: binary in build/ → runtime in src/runtime/ */
            snprintf(out, out_size, "%s/../src/runtime", exe);
            snprintf(probe, sizeof(probe), "%s/ilma_runtime.h", out);
            if (access(probe, F_OK) == 0) return;
        }
    }
#elif defined(__APPLE__)
    extern int _NSGetExecutablePath(char*, uint32_t*);
    uint32_t size = sizeof(exe);
    if (_NSGetExecutablePath(exe, &size) == 0) {
        char* slash = strrchr(exe, '/');
        if (slash) {
            *slash = '\0';
            snprintf(out, out_size, "%s/../lib/ilma/runtime", exe);
            char probe[1024];
            snprintf(probe, sizeof(probe), "%s/ilma_runtime.h", out);
            if (access(probe, F_OK) == 0) return;
            snprintf(out, out_size, "%s/../src/runtime", exe);
            snprintf(probe, sizeof(probe), "%s/ilma_runtime.h", out);
            if (access(probe, F_OK) == 0) return;
        }
    }
#endif

    /* 3. Fallback: common install paths */
    const char* candidates[] = {
        "/usr/local/lib/ilma/runtime",
        "/usr/lib/ilma/runtime",
        "./src/runtime",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        char probe[1024];
        snprintf(probe, sizeof(probe), "%s/ilma_runtime.h", candidates[i]);
        if (access(probe, F_OK) == 0) {
            snprintf(out, out_size, "%s", candidates[i]);
            return;
        }
    }
    snprintf(out, out_size, "/usr/local/lib/ilma/runtime");
}

int main(int argc, char** argv) {
    if (argc < 2) { print_usage(); return 0; }

    /* Package manager commands */
    if (strcmp(argv[1], "get") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: ilma get <package-name>\n");
            return 1;
        }
        return ilma_pkg_install(argv[2]);
    }
    if (strcmp(argv[1], "packages") == 0) {
        if (argc >= 3 && strcmp(argv[2], "--available") == 0) {
            return ilma_pkg_list_available();
        }
        return ilma_pkg_list_installed();
    }

    int show_tokens  = 0;
    int show_c       = 0;
    int compile_only = 0;
    const char* filename = NULL;

    for (int i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "--tokens")  == 0) show_tokens  = 1;
        else if (strcmp(argv[i], "--c")       == 0) show_c       = 1;
        else if (strcmp(argv[i], "--compile") == 0) compile_only = 1;
        else if (strcmp(argv[i], "--version") == 0) {
            printf("ilma %s\n", ILMA_VERSION); return 0;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(); return 0;
        }
        else { filename = argv[i]; }
    }

    if (!filename) {
        fprintf(stderr, "Oops! You forgot to tell me which file to compile.\n"
                        "Try: ilma your_program.ilma\n");
        return 1;
    }

    /* Read and lex */
    char* source = read_file(filename);
    Lexer lexer;
    lexer_init(&lexer, source);
    lexer_tokenize(&lexer);

    if (show_tokens) {
        print_tokens(lexer.tokens, lexer.token_count);
        lexer_free(&lexer); free(source); return 0;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, lexer.tokens, lexer.token_count);
    ASTNode* program = parser_parse(&parser);

    /* Build base name (strip .ilma extension) */
    char base[4096];
    strncpy(base, filename, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    char* dot = strrchr(base, '.');
    if (dot && strcmp(dot, ".ilma") == 0) *dot = '\0';

    char c_file[4096];
    snprintf(c_file, sizeof(c_file), "%s.c", base);

    if (show_c) {
        CodeGen cg;
        codegen_init(&cg, stdout);
        codegen_generate(&cg, program);
    } else {
        /* Write generated C */
        FILE* cf = fopen(c_file, "w");
        if (!cf) {
            fprintf(stderr, "Oops! I couldn't create the file '%s'.\n", c_file);
            return 1;
        }
        CodeGen cg;
        codegen_init(&cg, cf);
        codegen_generate(&cg, program);
        fclose(cf);

        /* Find runtime */
        char runtime[2048];
        find_runtime(runtime, sizeof(runtime));

        /* Build module source file list */
        char module_files[4096] = "";
        for (int i = 0; i < cg.used_module_count; i++) {
            const char* mod = cg.used_modules[i];
            char mod_path[512];
            if (strcmp(mod, "time") == 0)
                snprintf(mod_path, sizeof(mod_path), " \"%s/modules/time_mod.c\"", runtime);
            else
                snprintf(mod_path, sizeof(mod_path), " \"%s/modules/%s.c\"", runtime, mod);
            strncat(module_files, mod_path, sizeof(module_files) - strlen(module_files) - 1);
        }

        /* Compile with GCC */
        char cmd[8192];
        snprintf(cmd, sizeof(cmd),
            "gcc -O2 -o \"%s\" \"%s\" \"%s/ilma_runtime.c\"%s -I\"%s\" -lm"
            " -Wall -Wno-unused-variable -Wno-unused-function 2>&1",
            base, c_file, runtime, module_files, runtime);

        int ret = system(cmd);
        remove(c_file);

        if (ret != 0) {
            fprintf(stderr, "\nOops! The C compiler had a problem. See the error above.\n");
            ast_free(program); lexer_free(&lexer); free(source);
            return 1;
        }

        if (!compile_only) {
            /* Run — handle both absolute and relative paths */
            char run[4096];
            if (base[0] == '/' || base[0] == '~') {
                snprintf(run, sizeof(run), "\"%s\"", base);
            } else {
                snprintf(run, sizeof(run), "\"./%s\"", base);
            }
            ret = system(run);
            remove(base);
        } else {
            printf("Compiled: %s\n", base);
        }
    }

    ast_free(program);
    lexer_free(&lexer);
    free(source);
    return 0;
}
