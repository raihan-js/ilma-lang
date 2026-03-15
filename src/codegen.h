#ifndef ILMA_CODEGEN_H
#define ILMA_CODEGEN_H

#include "ast.h"
#include <stdio.h>

typedef struct {
    FILE* out;
    int   indent;
    int   temp_counter;
    int   in_recipe;          /* 1 if currently generating inside a recipe */
    int   in_blueprint;       /* 1 if currently generating inside a blueprint method */
    const char* current_bp;   /* current blueprint name when in_blueprint */
} CodeGen;

void codegen_init(CodeGen* cg, FILE* out);
void codegen_generate(CodeGen* cg, ASTNode* program);

#endif /* ILMA_CODEGEN_H */
