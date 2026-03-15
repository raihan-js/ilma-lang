#ifndef ILMA_PARSER_H
#define ILMA_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Token*   tokens;
    int      count;
    int      pos;
} Parser;

void     parser_init(Parser* p, Token* tokens, int count);
ASTNode* parser_parse(Parser* p);

#endif /* ILMA_PARSER_H */
