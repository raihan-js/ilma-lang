#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Helpers ───────────────────────────────────────────── */

static int is_utf8_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

static int is_unicode_letter_start(unsigned char c) {
    /* Accept any multi-byte UTF-8 start byte (Arabic, Urdu, etc.) */
    return c >= 0xC0;
}

static int is_ident_start(unsigned char c) {
    return isalpha(c) || c == '_' || is_unicode_letter_start(c);
}

static int is_ident_char(unsigned char c) {
    return isalnum(c) || c == '_' || is_unicode_letter_start(c) || is_utf8_continuation(c);
}

static char peek(Lexer* L) {
    if (L->pos >= L->length) return '\0';
    return L->source[L->pos];
}


static char advance(Lexer* L) {
    char c = L->source[L->pos];
    L->pos++;
    L->col++;
    return c;
}

static void emit_token(Lexer* L, TokenType type, char* value, int line, int col) {
    if (L->token_count >= L->token_capacity) {
        L->token_capacity *= 2;
        L->tokens = realloc(L->tokens, sizeof(Token) * L->token_capacity);
    }
    Token* t = &L->tokens[L->token_count++];
    t->type = type;
    t->value = value;
    t->line = line;
    t->col = col;
}

/* ── Token type names (for debugging) ─────────────────── */

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOK_SAY:        return "SAY";
        case TOK_ASK:        return "ASK";
        case TOK_REMEMBER:   return "REMEMBER";
        case TOK_IF:         return "IF";
        case TOK_OTHERWISE:  return "OTHERWISE";
        case TOK_REPEAT:     return "REPEAT";
        case TOK_KEEP:       return "KEEP";
        case TOK_GOING:      return "GOING";
        case TOK_WHILE:      return "WHILE";
        case TOK_FOR:        return "FOR";
        case TOK_EACH:       return "EACH";
        case TOK_IN:         return "IN";
        case TOK_RECIPE:     return "RECIPE";
        case TOK_GIVE:       return "GIVE";
        case TOK_BACK:       return "BACK";
        case TOK_BLUEPRINT:  return "BLUEPRINT";
        case TOK_CREATE:     return "CREATE";
        case TOK_ME:         return "ME";
        case TOK_COMES:      return "COMES";
        case TOK_FROM:       return "FROM";
        case TOK_COMES_FROM: return "COMES_FROM";
        case TOK_USE:        return "USE";
        case TOK_SHOUT:      return "SHOUT";
        case TOK_TRY:        return "TRY";
        case TOK_WHEN:       return "WHEN";
        case TOK_WRONG:      return "WRONG";
        case TOK_YES:        return "YES";
        case TOK_NO:         return "NO";
        case TOK_EMPTY:      return "EMPTY";
        case TOK_AND:        return "AND";
        case TOK_OR:         return "OR";
        case TOK_NOT:        return "NOT";
        case TOK_IS:         return "IS";
        case TOK_IS_NOT:     return "IS_NOT";
        case TOK_BAG:        return "BAG";
        case TOK_NOTEBOOK:   return "NOTEBOOK";
        case TOK_CHECK:      return "CHECK";
        case TOK_TYPE_WHOLE: return "TYPE_WHOLE";
        case TOK_TYPE_DECIMAL: return "TYPE_DECIMAL";
        case TOK_TYPE_TEXT:  return "TYPE_TEXT";
        case TOK_TYPE_TRUTH: return "TYPE_TRUTH";
        case TOK_TYPE_ANYTHING: return "TYPE_ANYTHING";
        case TOK_INT_LIT:    return "INT_LIT";
        case TOK_FLOAT_LIT:  return "FLOAT_LIT";
        case TOK_STRING_LIT:    return "STRING_LIT";
        case TOK_STRING_INTERP: return "STRING_INTERP";
        case TOK_IDENT:         return "IDENT";
        case TOK_PLUS:       return "PLUS";
        case TOK_MINUS:      return "MINUS";
        case TOK_STAR:       return "STAR";
        case TOK_SLASH:      return "SLASH";
        case TOK_PERCENT:    return "PERCENT";
        case TOK_EQ:         return "EQ";
        case TOK_NEQ:        return "NEQ";
        case TOK_LT:         return "LT";
        case TOK_GT:         return "GT";
        case TOK_LEQ:        return "LEQ";
        case TOK_GEQ:        return "GEQ";
        case TOK_ASSIGN:     return "ASSIGN";
        case TOK_COLON:      return "COLON";
        case TOK_DOT:        return "DOT";
        case TOK_DOTDOT:     return "DOTDOT";
        case TOK_COMMA:      return "COMMA";
        case TOK_LPAREN:     return "LPAREN";
        case TOK_RPAREN:     return "RPAREN";
        case TOK_LBRACKET:   return "LBRACKET";
        case TOK_RBRACKET:   return "RBRACKET";
        case TOK_INDENT:     return "INDENT";
        case TOK_DEDENT:     return "DEDENT";
        case TOK_NEWLINE:    return "NEWLINE";
        case TOK_EOF:        return "EOF";
        default:             return "UNKNOWN";
    }
}

/* ── Keyword lookup ───────────────────────────────────── */

typedef struct {
    const char* word;
    TokenType   type;
} Keyword;

static const Keyword keywords[] = {
    {"say",        TOK_SAY},
    {"ask",        TOK_ASK},
    {"remember",   TOK_REMEMBER},
    {"if",         TOK_IF},
    {"otherwise",  TOK_OTHERWISE},
    {"repeat",     TOK_REPEAT},
    {"keep",       TOK_KEEP},
    {"going",      TOK_GOING},
    {"while",      TOK_WHILE},
    {"for",        TOK_FOR},
    {"each",       TOK_EACH},
    {"in",         TOK_IN},
    {"recipe",     TOK_RECIPE},
    {"give",       TOK_GIVE},
    {"back",       TOK_BACK},
    {"blueprint",  TOK_BLUEPRINT},
    {"create",     TOK_CREATE},
    {"me",         TOK_ME},
    {"comes",      TOK_COMES},
    {"from",       TOK_FROM},
    {"comes_from", TOK_COMES_FROM},
    {"use",        TOK_USE},
    {"shout",      TOK_SHOUT},
    {"try",        TOK_TRY},
    {"when",       TOK_WHEN},
    {"wrong",      TOK_WRONG},
    {"yes",        TOK_YES},
    {"no",         TOK_NO},
    {"empty",      TOK_EMPTY},
    {"and",        TOK_AND},
    {"or",         TOK_OR},
    {"not",        TOK_NOT},
    {"is",         TOK_IS},
    {"bag",        TOK_BAG},
    {"notebook",   TOK_NOTEBOOK},
    {"check",      TOK_CHECK},
    {"whole",      TOK_TYPE_WHOLE},
    {"decimal",    TOK_TYPE_DECIMAL},
    {"text",       TOK_TYPE_TEXT},
    {"truth",      TOK_TYPE_TRUTH},
    {"anything",   TOK_TYPE_ANYTHING},
    {NULL, 0}
};

static TokenType find_keyword(const char* word) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(word, keywords[i].word) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENT;
}

/* ── Lexer implementation ─────────────────────────────── */

void lexer_init(Lexer* L, const char* source) {
    L->source = source;
    L->length = strlen(source);
    L->pos = 0;
    L->line = 1;
    L->col = 1;

    L->indent_stack[0] = 0;
    L->indent_top = 0;
    L->pending_dedents = 0;
    L->at_line_start = 1;

    L->token_capacity = 256;
    L->token_count = 0;
    L->tokens = malloc(sizeof(Token) * L->token_capacity);
}

static void lex_string(Lexer* L) {
    int start_line = L->line;
    int start_col = L->col;
    char quote = advance(L); /* consume opening quote */

    /* Check for triple quotes: """ or ''' */
    int is_triple = 0;
    if (L->pos + 1 < L->length &&
        L->source[L->pos] == quote && L->source[L->pos + 1] == quote) {
        advance(L); advance(L); /* consume second and third quotes */
        is_triple = 1;
    }

    size_t buf_cap = 64;
    size_t buf_len = 0;
    char* buf = malloc(buf_cap);

    if (is_triple) {
        /* Read until closing triple quotes */
        while (L->pos + 2 <= L->length) {
            if (L->pos + 2 < L->length &&
                L->source[L->pos] == quote &&
                L->source[L->pos + 1] == quote &&
                L->source[L->pos + 2] == quote) {
                advance(L); advance(L); advance(L); /* consume closing triple quotes */
                goto done_string;
            }
            char c = L->source[L->pos];
            if (c == '\n') { L->line++; L->col = 0; }
            advance(L);
            if (buf_len + 1 >= buf_cap) { buf_cap *= 2; buf = realloc(buf, buf_cap); }
            buf[buf_len++] = c;
        }
        fprintf(stderr, "Oops! Triple-quoted string starting on line %d was never closed.\n", start_line);
        exit(1);
    } else {
        /* Original single-quote string scanning */
        while (L->pos < L->length && L->source[L->pos] != quote) {
            char c = L->source[L->pos];
            if (c == '\\' && L->pos + 1 < L->length) {
                advance(L);
                char esc = advance(L);
                char actual;
                switch (esc) {
                    case 'n': actual = '\n'; break;
                    case 't': actual = '\t'; break;
                    case '\\': actual = '\\'; break;
                    case '"': actual = '"'; break;
                    case '\'': actual = '\''; break;
                    default: actual = esc; break;
                }
                if (buf_len + 1 >= buf_cap) { buf_cap *= 2; buf = realloc(buf, buf_cap); }
                buf[buf_len++] = actual;
            } else {
                if (c == '\n') { L->line++; L->col = 0; }
                advance(L);
                if (buf_len + 1 >= buf_cap) { buf_cap *= 2; buf = realloc(buf, buf_cap); }
                buf[buf_len++] = c;
            }
        }

        if (L->pos < L->length) {
            advance(L); /* consume closing quote */
        } else {
            fprintf(stderr, "Oops! You forgot to close a piece of text that starts on line %d.\n"
                            "Every piece of text needs a closing \" mark.\n", start_line);
            exit(1);
        }
    }

done_string:
    /* Check for string interpolation (contains unescaped {) */
    ;
    int has_interp = 0;
    for (size_t i = 0; i < buf_len; i++) {
        if (buf[i] == '{') { has_interp = 1; break; }
    }

    buf[buf_len] = '\0';
    emit_token(L, has_interp ? TOK_STRING_INTERP : TOK_STRING_LIT, buf, start_line, start_col);
}

static void lex_number(Lexer* L) {
    int start_line = L->line;
    int start_col = L->col;
    size_t start = L->pos;
    int is_float = 0;

    while (L->pos < L->length && isdigit((unsigned char)L->source[L->pos])) {
        advance(L);
    }

    if (L->pos < L->length && L->source[L->pos] == '.' &&
        L->pos + 1 < L->length && isdigit((unsigned char)L->source[L->pos + 1])) {
        is_float = 1;
        advance(L); /* consume '.' */
        while (L->pos < L->length && isdigit((unsigned char)L->source[L->pos])) {
            advance(L);
        }
    }

    size_t len = L->pos - start;
    char* val = malloc(len + 1);
    memcpy(val, L->source + start, len);
    val[len] = '\0';

    emit_token(L, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, val, start_line, start_col);
}

static void lex_identifier(Lexer* L) {
    int start_line = L->line;
    int start_col = L->col;
    size_t start = L->pos;

    while (L->pos < L->length && is_ident_char((unsigned char)L->source[L->pos])) {
        advance(L);
    }

    size_t len = L->pos - start;
    char* word = malloc(len + 1);
    memcpy(word, L->source + start, len);
    word[len] = '\0';

    TokenType type = find_keyword(word);

    /* Handle "is not" as a two-word keyword */
    if (type == TOK_IS) {
        /* Look ahead past whitespace for "not" */
        size_t saved_pos = L->pos;
        int saved_col = L->col;
        while (L->pos < L->length && L->source[L->pos] == ' ') {
            advance(L);
        }
        if (L->pos + 3 <= L->length &&
            strncmp(L->source + L->pos, "not", 3) == 0 &&
            (L->pos + 3 >= L->length || !is_ident_char((unsigned char)L->source[L->pos + 3]))) {
            /* Consume "not" */
            L->pos += 3;
            L->col += 3;
            free(word);
            word = strdup("is not");
            emit_token(L, TOK_IS_NOT, word, start_line, start_col);
            return;
        }
        /* Not "is not", restore position */
        L->pos = saved_pos;
        L->col = saved_col;
    }

    /* Handle "keep going while" — emit separate tokens */
    /* Handle "for each" — emit separate tokens */
    /* Handle "give back" — emit separate tokens */
    /* Handle "comes from" — emit separate tokens */
    /* Handle "when wrong" — emit separate tokens */
    /* These are all handled naturally as separate keyword tokens */

    if (type != TOK_IDENT) {
        emit_token(L, type, word, start_line, start_col);
    } else {
        emit_token(L, TOK_IDENT, word, start_line, start_col);
    }
}

static void handle_indentation(Lexer* L) {
    int indent = 0;
    while (L->pos < L->length && L->source[L->pos] == ' ') {
        indent++;
        advance(L);
    }
    /* Tabs count as 4 spaces */
    while (L->pos < L->length && L->source[L->pos] == '\t') {
        indent += 4;
        advance(L);
    }

    /* Skip blank lines and comment-only lines */
    if (L->pos >= L->length || L->source[L->pos] == '\n' || L->source[L->pos] == '#') {
        return;
    }

    int current_indent = L->indent_stack[L->indent_top];

    if (indent > current_indent) {
        L->indent_top++;
        L->indent_stack[L->indent_top] = indent;
        emit_token(L, TOK_INDENT, NULL, L->line, 1);
    } else if (indent < current_indent) {
        while (L->indent_top > 0 && L->indent_stack[L->indent_top] > indent) {
            L->indent_top--;
            emit_token(L, TOK_DEDENT, NULL, L->line, 1);
        }
        if (L->indent_stack[L->indent_top] != indent) {
            fprintf(stderr, "Oops! The spacing on line %d doesn't line up.\n"
                            "Make sure each block is indented the same amount (use 4 spaces).\n",
                            L->line);
            exit(1);
        }
    }
}

void lexer_tokenize(Lexer* L) {
    while (L->pos < L->length) {
        /* Handle indentation at line start */
        if (L->at_line_start) {
            L->at_line_start = 0;
            handle_indentation(L);
        }

        if (L->pos >= L->length) break;

        char c = L->source[L->pos];

        /* Skip spaces (not at line start) */
        if (c == ' ' || c == '\t') {
            advance(L);
            continue;
        }

        /* Newline */
        if (c == '\n') {
            /* Only emit newline if last token was not already a newline or indent/dedent */
            if (L->token_count > 0) {
                TokenType last = L->tokens[L->token_count - 1].type;
                if (last != TOK_NEWLINE && last != TOK_INDENT && last != TOK_DEDENT) {
                    emit_token(L, TOK_NEWLINE, NULL, L->line, L->col);
                }
            }
            advance(L);
            L->line++;
            L->col = 1;
            L->at_line_start = 1;
            continue;
        }

        /* Carriage return (Windows line endings) */
        if (c == '\r') {
            advance(L);
            continue;
        }

        /* Comments */
        if (c == '#') {
            while (L->pos < L->length && L->source[L->pos] != '\n') {
                advance(L);
            }
            continue;
        }

        /* Strings */
        if (c == '"' || c == '\'') {
            lex_string(L);
            continue;
        }

        /* Numbers */
        if (isdigit((unsigned char)c)) {
            lex_number(L);
            continue;
        }

        /* Raw strings with backticks (no escape processing, no interpolation) */
        if (c == '`') {
            int start_l = L->line, start_c = L->col;
            advance(L); /* consume opening backtick */
            size_t buf_cap = 64, buf_len = 0;
            char* buf = malloc(buf_cap);
            while (L->pos < L->length && L->source[L->pos] != '`') {
                char ch = L->source[L->pos];
                if (ch == '\n') { L->line++; L->col = 0; }
                advance(L);
                if (buf_len + 1 >= buf_cap) { buf_cap *= 2; buf = realloc(buf, buf_cap); }
                buf[buf_len++] = ch;
            }
            if (L->pos < L->length) advance(L); /* consume closing backtick */
            buf[buf_len] = '\0';
            emit_token(L, TOK_STRING_LIT, buf, start_l, start_c);
            continue;
        }

        /* Identifiers and keywords */
        if (is_ident_start((unsigned char)c)) {
            lex_identifier(L);
            continue;
        }

        /* Operators and punctuation */
        int tok_line = L->line;
        int tok_col = L->col;

        switch (c) {
            case '+': advance(L); emit_token(L, TOK_PLUS, NULL, tok_line, tok_col); break;
            case '-': advance(L); emit_token(L, TOK_MINUS, NULL, tok_line, tok_col); break;
            case '*': advance(L); emit_token(L, TOK_STAR, NULL, tok_line, tok_col); break;
            case '/': advance(L); emit_token(L, TOK_SLASH, NULL, tok_line, tok_col); break;
            case '%': advance(L); emit_token(L, TOK_PERCENT, NULL, tok_line, tok_col); break;
            case '.':
                advance(L);
                if (peek(L) == '.') {
                    advance(L);
                    emit_token(L, TOK_DOTDOT, NULL, tok_line, tok_col);
                } else {
                    emit_token(L, TOK_DOT, NULL, tok_line, tok_col);
                }
                break;
            case ',': advance(L); emit_token(L, TOK_COMMA, NULL, tok_line, tok_col); break;
            case '(': advance(L); emit_token(L, TOK_LPAREN, NULL, tok_line, tok_col); break;
            case ')': advance(L); emit_token(L, TOK_RPAREN, NULL, tok_line, tok_col); break;
            case '[': advance(L); emit_token(L, TOK_LBRACKET, NULL, tok_line, tok_col); break;
            case ']': advance(L); emit_token(L, TOK_RBRACKET, NULL, tok_line, tok_col); break;
            case ':': advance(L); emit_token(L, TOK_COLON, NULL, tok_line, tok_col); break;
            case '=':
                advance(L);
                if (peek(L) == '=') {
                    advance(L);
                    emit_token(L, TOK_EQ, NULL, tok_line, tok_col);
                } else {
                    emit_token(L, TOK_ASSIGN, NULL, tok_line, tok_col);
                }
                break;
            case '!':
                advance(L);
                if (peek(L) == '=') {
                    advance(L);
                    emit_token(L, TOK_NEQ, NULL, tok_line, tok_col);
                } else {
                    fprintf(stderr, "Oops! On line %d, I found a '!' but I don't know what to do with it.\n"
                                    "In ILMA, use 'not' instead of '!'.\n", tok_line);
                    exit(1);
                }
                break;
            case '<':
                advance(L);
                if (peek(L) == '=') {
                    advance(L);
                    emit_token(L, TOK_LEQ, NULL, tok_line, tok_col);
                } else {
                    emit_token(L, TOK_LT, NULL, tok_line, tok_col);
                }
                break;
            case '>':
                advance(L);
                if (peek(L) == '=') {
                    advance(L);
                    emit_token(L, TOK_GEQ, NULL, tok_line, tok_col);
                } else {
                    emit_token(L, TOK_GT, NULL, tok_line, tok_col);
                }
                break;
            default:
                fprintf(stderr, "Oops! On line %d, I found the character '%c' and I don't know what it means.\n"
                                "Check your code for any unusual characters.\n", tok_line, c);
                exit(1);
        }
    }

    /* Emit remaining DEDENT tokens */
    while (L->indent_top > 0) {
        L->indent_top--;
        emit_token(L, TOK_DEDENT, NULL, L->line, 1);
    }

    /* Emit final newline if needed */
    if (L->token_count > 0 && L->tokens[L->token_count - 1].type != TOK_NEWLINE) {
        emit_token(L, TOK_NEWLINE, NULL, L->line, L->col);
    }

    emit_token(L, TOK_EOF, NULL, L->line, L->col);
}

void lexer_free(Lexer* L) {
    for (int i = 0; i < L->token_count; i++) {
        free(L->tokens[i].value);
    }
    free(L->tokens);
    L->tokens = NULL;
    L->token_count = 0;
}
