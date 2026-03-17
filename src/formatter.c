#include "formatter.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple output buffer */
typedef struct {
    char*  buf;
    size_t len;
    size_t cap;
} OutBuf;

static void ob_init(OutBuf* ob) {
    ob->cap = 4096;
    ob->buf = malloc(ob->cap);
    ob->len = 0;
    ob->buf[0] = '\0';
}

static void ob_push(OutBuf* ob, const char* s) {
    size_t sl = strlen(s);
    while (ob->len + sl + 1 >= ob->cap) {
        ob->cap *= 2;
        ob->buf = realloc(ob->buf, ob->cap);
    }
    memcpy(ob->buf + ob->len, s, sl);
    ob->len += sl;
    ob->buf[ob->len] = '\0';
}

static void ob_push_char(OutBuf* ob, char c) {
    char tmp[2] = {c, '\0'};
    ob_push(ob, tmp);
}

char* ilma_fmt_source(const char* source) {
    Lexer lex;
    lexer_init(&lex, source);
    lexer_tokenize(&lex);

    OutBuf ob;
    ob_init(&ob);

    int indent = 0;
    int last_was_newline = 1;

    for (int i = 0; i < lex.token_count; i++) {
        Token* t = &lex.tokens[i];
        Token* prev = i > 0 ? &lex.tokens[i-1] : NULL;
        Token* next = (i+1 < lex.token_count) ? &lex.tokens[i+1] : NULL;
        (void)next;

        if (t->type == TOK_INDENT) {
            indent++;
            continue;
        }
        if (t->type == TOK_DEDENT) {
            if (indent > 0) indent--;
            continue;
        }
        if (t->type == TOK_NEWLINE) {
            if (!last_was_newline) {
                ob_push_char(&ob, '\n');
                last_was_newline = 1;
            }
            continue;
        }
        if (t->type == TOK_EOF) break;

        /* Emit indent if at line start */
        if (last_was_newline) {
            for (int j = 0; j < indent * 4; j++) ob_push_char(&ob, ' ');
            last_was_newline = 0;
        } else {
            /* Space before token based on context */
            int need_space = 1;
            if (prev) {
                /* No space before colon */
                if (t->type == TOK_COLON) need_space = 0;
                /* No space after opening paren/bracket */
                if (prev->type == TOK_LPAREN || prev->type == TOK_LBRACKET) need_space = 0;
                /* No space before closing paren/bracket */
                if (t->type == TOK_RPAREN || t->type == TOK_RBRACKET) need_space = 0;
                /* No space before comma */
                if (t->type == TOK_COMMA) need_space = 0;
                /* No space before dot or after dot */
                if (t->type == TOK_DOT || prev->type == TOK_DOT) need_space = 0;
            }
            if (need_space) ob_push_char(&ob, ' ');
        }

        /* Emit the token */
        switch (t->type) {
            case TOK_SAY:        ob_push(&ob, "say"); break;
            case TOK_ASK:        ob_push(&ob, "ask"); break;
            case TOK_REMEMBER:   ob_push(&ob, "remember"); break;
            case TOK_IF:         ob_push(&ob, "if"); break;
            case TOK_OTHERWISE:  ob_push(&ob, "otherwise"); break;
            case TOK_REPEAT:     ob_push(&ob, "repeat"); break;
            case TOK_KEEP:       ob_push(&ob, "keep"); break;
            case TOK_GOING:      ob_push(&ob, "going"); break;
            case TOK_WHILE:      ob_push(&ob, "while"); break;
            case TOK_FOR:        ob_push(&ob, "for"); break;
            case TOK_EACH:       ob_push(&ob, "each"); break;
            case TOK_IN:         ob_push(&ob, "in"); break;
            case TOK_RECIPE:     ob_push(&ob, "recipe"); break;
            case TOK_GIVE:       ob_push(&ob, "give"); break;
            case TOK_BACK:       ob_push(&ob, "back"); break;
            case TOK_BLUEPRINT:  ob_push(&ob, "blueprint"); break;
            case TOK_CREATE:     ob_push(&ob, "create"); break;
            case TOK_ME:         ob_push(&ob, "me"); break;
            case TOK_COMES:      ob_push(&ob, "comes"); break;
            case TOK_FROM:       ob_push(&ob, "from"); break;
            case TOK_COMES_FROM: ob_push(&ob, "comes_from"); break;
            case TOK_USE:        ob_push(&ob, "use"); break;
            case TOK_SHOUT:      ob_push(&ob, "shout"); break;
            case TOK_TRY:        ob_push(&ob, "try"); break;
            case TOK_WHEN:       ob_push(&ob, "when"); break;
            case TOK_WRONG:      ob_push(&ob, "wrong"); break;
            case TOK_YES:        ob_push(&ob, "yes"); break;
            case TOK_NO:         ob_push(&ob, "no"); break;
            case TOK_EMPTY:      ob_push(&ob, "empty"); break;
            case TOK_AND:        ob_push(&ob, "and"); break;
            case TOK_OR:         ob_push(&ob, "or"); break;
            case TOK_NOT:        ob_push(&ob, "not"); break;
            case TOK_IS:         ob_push(&ob, "is"); break;
            case TOK_IS_NOT:     ob_push(&ob, "is not"); break;
            case TOK_BAG:        ob_push(&ob, "bag"); break;
            case TOK_NOTEBOOK:   ob_push(&ob, "notebook"); break;
            case TOK_CHECK:      ob_push(&ob, "check"); break;
            case TOK_TEST:       ob_push(&ob, "test"); break;
            case TOK_ASSERT:     ob_push(&ob, "assert"); break;
            case TOK_RUN:        ob_push(&ob, "run"); break;
            case TOK_WAIT:       ob_push(&ob, "wait"); break;
            case TOK_TYPE_WHOLE:    ob_push(&ob, "whole"); break;
            case TOK_TYPE_DECIMAL:  ob_push(&ob, "decimal"); break;
            case TOK_TYPE_TEXT:     ob_push(&ob, "text"); break;
            case TOK_TYPE_TRUTH:    ob_push(&ob, "truth"); break;
            case TOK_TYPE_ANYTHING: ob_push(&ob, "anything"); break;
            case TOK_PLUS:       ob_push(&ob, "+"); break;
            case TOK_MINUS:      ob_push(&ob, "-"); break;
            case TOK_STAR:       ob_push(&ob, "*"); break;
            case TOK_SLASH:      ob_push(&ob, "/"); break;
            case TOK_PERCENT:    ob_push(&ob, "%"); break;
            case TOK_EQ:         ob_push(&ob, "=="); break;
            case TOK_NEQ:        ob_push(&ob, "!="); break;
            case TOK_LT:         ob_push(&ob, "<"); break;
            case TOK_GT:         ob_push(&ob, ">"); break;
            case TOK_LEQ:        ob_push(&ob, "<="); break;
            case TOK_GEQ:        ob_push(&ob, ">="); break;
            case TOK_ASSIGN:     ob_push(&ob, "="); break;
            case TOK_COLON:      ob_push(&ob, ":"); break;
            case TOK_DOT:        ob_push(&ob, "."); break;
            case TOK_DOTDOT:     ob_push(&ob, ".."); break;
            case TOK_COMMA:      ob_push(&ob, ","); break;
            case TOK_LPAREN:     ob_push(&ob, "("); break;
            case TOK_RPAREN:     ob_push(&ob, ")"); break;
            case TOK_LBRACKET:   ob_push(&ob, "["); break;
            case TOK_RBRACKET:   ob_push(&ob, "]"); break;
            case TOK_INT_LIT:    ob_push(&ob, t->value); break;
            case TOK_FLOAT_LIT:  ob_push(&ob, t->value); break;
            case TOK_IDENT:      ob_push(&ob, t->value); break;
            case TOK_STRING_LIT:
            case TOK_STRING_INTERP: {
                ob_push_char(&ob, '"');
                ob_push(&ob, t->value);
                ob_push_char(&ob, '"');
                break;
            }
            default: break;
        }
    }

    lexer_free(&lex);
    return ob.buf;
}

int ilma_fmt_file(const char* filename, int check_only) {
    /* Read file */
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "ilma fmt: cannot open '%s'\n", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* src = malloc(sz + 1);
    fread(src, 1, sz, f);
    src[sz] = '\0';
    fclose(f);

    char* formatted = ilma_fmt_source(src);
    free(src);

    if (!formatted) return 1;

    if (check_only) {
        /* Re-read and compare */
        FILE* f2 = fopen(filename, "rb");
        if (!f2) { free(formatted); return 1; }
        fseek(f2, 0, SEEK_END);
        long sz2 = ftell(f2);
        fseek(f2, 0, SEEK_SET);
        char* orig = malloc(sz2 + 1);
        fread(orig, 1, sz2, f2);
        orig[sz2] = '\0';
        fclose(f2);
        int same = (strcmp(orig, formatted) == 0);
        free(orig);
        free(formatted);
        if (!same) {
            printf("%s: would reformat\n", filename);
            return 1;
        }
        return 0;
    }

    /* Write back */
    FILE* out = fopen(filename, "wb");
    if (!out) {
        fprintf(stderr, "ilma fmt: cannot write '%s'\n", filename);
        free(formatted);
        return 1;
    }
    fwrite(formatted, 1, strlen(formatted), out);
    fclose(out);
    free(formatted);
    printf("Formatted: %s\n", filename);
    return 0;
}
