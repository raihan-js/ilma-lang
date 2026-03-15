#ifndef ILMA_EVALUATOR_H
#define ILMA_EVALUATOR_H

#include "ast.h"
#include "runtime/ilma_runtime.h"

#define ILMA_EVAL_MAX_VARS 256
#define ILMA_EVAL_MAX_RECIPES 128
#define ILMA_EVAL_MAX_BLUEPRINTS 64

typedef struct {
    char* name;
    IlmaValue value;
} EvalVar;

typedef struct {
    char* name;
    ASTNode* node;  /* points to the NODE_RECIPE AST node */
} EvalRecipe;

typedef struct {
    char* name;
    char* parent;
    ASTNode* node;  /* points to the NODE_BLUEPRINT AST node */
} EvalBlueprint;

typedef struct EvalScope EvalScope;
struct EvalScope {
    EvalVar vars[ILMA_EVAL_MAX_VARS];
    int var_count;
    EvalScope* parent;
};

typedef struct {
    EvalScope* scope;           /* current variable scope */
    EvalRecipe recipes[ILMA_EVAL_MAX_RECIPES];
    int recipe_count;
    EvalBlueprint blueprints[ILMA_EVAL_MAX_BLUEPRINTS];
    int blueprint_count;
    char* output_buffer;        /* captured output for WASM mode */
    int output_len;
    int output_cap;
    int capture_output;         /* 1 = capture to buffer, 0 = print to stdout */
    int returning;              /* 1 = currently returning from recipe */
    IlmaValue return_value;
} Evaluator;

void evaluator_init(Evaluator* ev);
void evaluator_free(Evaluator* ev);
IlmaValue evaluator_run(Evaluator* ev, ASTNode* program);
char* evaluator_get_output(Evaluator* ev);

#endif
