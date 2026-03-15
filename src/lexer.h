#ifndef ILMA_LEXER_H
#define ILMA_LEXER_H

#include <stddef.h>

typedef enum {
    /* Keywords */
    TOK_SAY, TOK_ASK, TOK_REMEMBER,
    TOK_IF, TOK_OTHERWISE, TOK_REPEAT,
    TOK_KEEP, TOK_GOING, TOK_WHILE,
    TOK_FOR, TOK_EACH, TOK_IN,
    TOK_RECIPE, TOK_GIVE, TOK_BACK,
    TOK_BLUEPRINT, TOK_CREATE, TOK_ME,
    TOK_COMES, TOK_FROM, TOK_COMES_FROM,
    TOK_USE, TOK_SHOUT,
    TOK_TRY, TOK_WHEN, TOK_WRONG,
    TOK_YES, TOK_NO, TOK_EMPTY,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_IS, TOK_IS_NOT,
    TOK_BAG, TOK_NOTEBOOK,
    TOK_CHECK,

    /* Types */
    TOK_TYPE_WHOLE, TOK_TYPE_DECIMAL,
    TOK_TYPE_TEXT, TOK_TYPE_TRUTH,
    TOK_TYPE_ANYTHING,

    /* Literals */
    TOK_INT_LIT, TOK_FLOAT_LIT,
    TOK_STRING_LIT, TOK_IDENT,

    /* String interpolation */
    TOK_STRING_INTERP,

    /* Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LEQ, TOK_GEQ,
    TOK_ASSIGN, TOK_COLON, TOK_DOT, TOK_DOTDOT, TOK_COMMA,
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACKET, TOK_RBRACKET,

    /* Structural */
    TOK_INDENT, TOK_DEDENT, TOK_NEWLINE, TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    char*     value;   /* heap-allocated, NULL for punctuation */
    int       line;
    int       col;
} Token;

typedef struct {
    const char* source;
    size_t      length;
    size_t      pos;
    int         line;
    int         col;

    /* Indentation tracking */
    int         indent_stack[128];
    int         indent_top;
    int         pending_dedents;
    int         at_line_start;

    /* Token output */
    Token*      tokens;
    int         token_count;
    int         token_capacity;
} Lexer;

void  lexer_init(Lexer* lexer, const char* source);
void  lexer_tokenize(Lexer* lexer);
void  lexer_free(Lexer* lexer);

const char* token_type_name(TokenType type);

#endif /* ILMA_LEXER_H */
