#include "checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256
#define MAX_RECIPES 64

typedef struct {
    char* name;
    int   param_count;
} RecipeInfo;

typedef struct CheckScope {
    char* vars[MAX_VARS];
    int   var_count;
    struct CheckScope* parent;
} CheckScope;

static RecipeInfo g_recipes[MAX_RECIPES];
static int        g_recipe_count = 0;
static int        g_errors = 0;
static int        g_in_blueprint = 0;
static const char* g_filename = NULL;

static CheckScope* scope_new_cs(void) {
    CheckScope* s = calloc(1, sizeof(CheckScope));
    return s;
}

static void scope_push_cs(CheckScope** sp) {
    CheckScope* s = scope_new_cs();
    s->parent = *sp;
    *sp = s;
}

static void scope_pop_cs(CheckScope** sp) {
    if (!*sp) return;
    CheckScope* old = *sp;
    *sp = old->parent;
    for (int i = 0; i < old->var_count; i++) free(old->vars[i]);
    free(old);
}

static void scope_define(CheckScope* s, const char* name) {
    if (s->var_count < MAX_VARS) {
        s->vars[s->var_count++] = strdup(name);
    }
}

static int scope_lookup(CheckScope* s, const char* name) {
    while (s) {
        for (int i = 0; i < s->var_count; i++) {
            if (strcmp(s->vars[i], name) == 0) return 1;
        }
        s = s->parent;
    }
    return 0;
}

static void check_expr(ASTNode* node, CheckScope* scope);
static void check_stmt(ASTNode* node, CheckScope* scope);
static void check_block(ASTNode* block, CheckScope* scope);

static void check_expr(ASTNode* node, CheckScope* scope) {
    if (!node) return;
    switch (node->type) {
        case NODE_IDENTIFIER: {
            const char* name = node->data.ident_name;
            /* Special: "me" */
            if (strcmp(name, "me") == 0) {
                if (!g_in_blueprint) {
                    fprintf(stderr, "%s:%d: error: 'me' used outside a blueprint\n",
                            g_filename, node->line);
                    g_errors++;
                }
                return;
            }
            if (!scope_lookup(scope, name)) {
                fprintf(stderr, "%s:%d: error: undefined variable '%s'\n",
                        g_filename, node->line, name);
                g_errors++;
            }
            break;
        }
        case NODE_BINARY_OP:
            check_expr(node->data.binary.left, scope);
            check_expr(node->data.binary.right, scope);
            break;
        case NODE_UNARY_OP:
            check_expr(node->data.unary.operand, scope);
            break;
        case NODE_CALL: {
            check_expr(node->data.call.callee, scope);
            for (int i = 0; i < node->data.call.args.count; i++)
                check_expr(node->data.call.args.items[i], scope);
            /* Check arg count for known recipes */
            if (node->data.call.callee->type == NODE_IDENTIFIER) {
                const char* fname = node->data.call.callee->data.ident_name;
                for (int i = 0; i < g_recipe_count; i++) {
                    if (strcmp(g_recipes[i].name, fname) == 0) {
                        if (node->data.call.args.count != g_recipes[i].param_count) {
                            fprintf(stderr, "%s:%d: error: '%s' expects %d argument(s), got %d\n",
                                    g_filename, node->line, fname,
                                    g_recipes[i].param_count, node->data.call.args.count);
                            g_errors++;
                        }
                        break;
                    }
                }
            }
            break;
        }
        case NODE_MEMBER_ACCESS:
            check_expr(node->data.member.object, scope);
            break;
        case NODE_INDEX_ACCESS:
            check_expr(node->data.index_access.object, scope);
            check_expr(node->data.index_access.index, scope);
            break;
        case NODE_BAG_LIT:
            for (int i = 0; i < node->data.bag.elements.count; i++)
                check_expr(node->data.bag.elements.items[i], scope);
            break;
        case NODE_NOTEBOOK_LIT:
            for (int i = 0; i < node->data.notebook.values.count; i++)
                check_expr(node->data.notebook.values.items[i], scope);
            break;
        case NODE_STRING_INTERP:
            for (int i = 0; i < node->data.interp.parts.count; i++)
                check_expr(node->data.interp.parts.items[i], scope);
            break;
        case NODE_ASK_EXPR:
            if (node->data.ask.prompt)
                check_expr(node->data.ask.prompt, scope);
            break;
        default: break;
    }
}

static void check_block(ASTNode* block, CheckScope* scope) {
    if (!block) return;
    int found_return = 0;
    for (int i = 0; i < block->data.block.statements.count; i++) {
        ASTNode* stmt = block->data.block.statements.items[i];
        if (found_return) {
            fprintf(stderr, "%s:%d: warning: unreachable code after 'give back'\n",
                    g_filename, stmt->line);
            /* Not a fatal error, just warn */
            break;
        }
        check_stmt(stmt, scope);
        if (stmt->type == NODE_GIVE_BACK) found_return = 1;
    }
}

static void check_stmt(ASTNode* node, CheckScope* scope) {
    if (!node) return;
    switch (node->type) {
        case NODE_SAY:
            check_expr(node->data.say.expr, scope);
            break;
        case NODE_REMEMBER:
            check_expr(node->data.remember.value, scope);
            scope_define(scope, node->data.remember.name);
            break;
        case NODE_ASSIGN:
            check_expr(node->data.assign.target, scope);
            check_expr(node->data.assign.value, scope);
            break;
        case NODE_IF:
            check_expr(node->data.if_stmt.condition, scope);
            scope_push_cs(&scope);
            check_block(node->data.if_stmt.then_block, scope);
            scope_pop_cs(&scope);
            if (node->data.if_stmt.else_block) {
                scope_push_cs(&scope);
                check_stmt(node->data.if_stmt.else_block, scope);
                scope_pop_cs(&scope);
            }
            break;
        case NODE_WHILE:
            check_expr(node->data.while_stmt.condition, scope);
            scope_push_cs(&scope);
            check_block(node->data.while_stmt.body, scope);
            scope_pop_cs(&scope);
            break;
        case NODE_REPEAT:
            check_expr(node->data.repeat.count, scope);
            scope_push_cs(&scope);
            check_block(node->data.repeat.body, scope);
            scope_pop_cs(&scope);
            break;
        case NODE_RANGE_LOOP: {
            check_expr(node->data.range_loop.start, scope);
            check_expr(node->data.range_loop.end, scope);
            scope_push_cs(&scope);
            scope_define(scope, node->data.range_loop.var_name);
            check_block(node->data.range_loop.body, scope);
            scope_pop_cs(&scope);
            break;
        }
        case NODE_FOR_EACH: {
            check_expr(node->data.for_each.iterable, scope);
            scope_push_cs(&scope);
            scope_define(scope, node->data.for_each.var_name);
            check_block(node->data.for_each.body, scope);
            scope_pop_cs(&scope);
            break;
        }
        case NODE_RECIPE: {
            /* Register recipe */
            if (g_recipe_count < MAX_RECIPES) {
                g_recipes[g_recipe_count].name = node->data.recipe.name;
                g_recipes[g_recipe_count].param_count = node->data.recipe.param_count;
                g_recipe_count++;
            }
            scope_define(scope, node->data.recipe.name);
            scope_push_cs(&scope);
            for (int i = 0; i < node->data.recipe.param_count; i++)
                scope_define(scope, node->data.recipe.params[i]);
            check_block(node->data.recipe.body, scope);
            scope_pop_cs(&scope);
            break;
        }
        case NODE_GIVE_BACK:
            if (node->data.give_back.value)
                check_expr(node->data.give_back.value, scope);
            break;
        case NODE_BLUEPRINT: {
            scope_define(scope, node->data.blueprint.name);
            int old_in_bp = g_in_blueprint;
            g_in_blueprint = 1;
            for (int i = 0; i < node->data.blueprint.members.count; i++)
                check_stmt(node->data.blueprint.members.items[i], scope);
            g_in_blueprint = old_in_bp;
            break;
        }
        case NODE_SHOUT:
            check_expr(node->data.shout.message, scope);
            break;
        case NODE_TRY:
            check_block(node->data.try_stmt.try_block, scope);
            check_block(node->data.try_stmt.when_wrong_block, scope);
            break;
        case NODE_CHECK:
            check_expr(node->data.check_stmt.subject, scope);
            for (int i = 0; i < node->data.check_stmt.case_count; i++) {
                check_expr(node->data.check_stmt.case_exprs[i], scope);
                scope_push_cs(&scope);
                check_block(node->data.check_stmt.case_bodies[i], scope);
                scope_pop_cs(&scope);
            }
            if (node->data.check_stmt.otherwise_body) {
                scope_push_cs(&scope);
                check_block(node->data.check_stmt.otherwise_body, scope);
                scope_pop_cs(&scope);
            }
            break;
        case NODE_ASSERT:
            check_expr(node->data.assert_stmt.expr, scope);
            break;
        case NODE_TEST: {
            scope_push_cs(&scope);
            check_block(node->data.test_stmt.body, scope);
            scope_pop_cs(&scope);
            break;
        }
        case NODE_RUN_STMT:
            check_expr(node->data.run_stmt.call, scope);
            scope_define(scope, node->data.run_stmt.task_name);
            break;
        case NODE_WAIT_STMT:
            /* task_name should be in scope */
            if (!scope_lookup(scope, node->data.wait_stmt.task_name)) {
                fprintf(stderr, "%s:%d: error: unknown task '%s'\n",
                        g_filename, node->line, node->data.wait_stmt.task_name);
                g_errors++;
            }
            break;
        case NODE_EXPR_STMT:
            /* NODE_EXPR_STMT reuses say.expr field */
            check_expr(node->data.say.expr, scope);
            break;
        case NODE_BLOCK:
            check_block(node, scope);
            break;
        case NODE_USE:
        default: break;
    }
}

int ilma_check(ASTNode* program, const char* filename) {
    g_errors = 0;
    g_recipe_count = 0;
    g_in_blueprint = 0;
    g_filename = filename;

    CheckScope* scope = scope_new_cs();

    /* Pre-define builtin functions */
    const char* builtins[] = {
        "say", "ask", "read_file", "write_file", "file_exists",
        "sqrt", "abs", "round", "random", "floor", "ceil", "power",
        "length", "upper", "lower", "contains", "starts_with", "ends_with",
        "to_whole", "to_decimal", "to_text", "type_of",
        NULL
    };
    for (int i = 0; builtins[i]; i++) scope_define(scope, builtins[i]);

    /* First pass: collect all top-level recipe names */
    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* node = program->data.block.statements.items[i];
        if (node->type == NODE_RECIPE) {
            scope_define(scope, node->data.recipe.name);
            if (g_recipe_count < MAX_RECIPES) {
                g_recipes[g_recipe_count].name = node->data.recipe.name;
                g_recipes[g_recipe_count].param_count = node->data.recipe.param_count;
                g_recipe_count++;
            }
        }
        if (node->type == NODE_BLUEPRINT) {
            scope_define(scope, node->data.blueprint.name);
        }
    }

    /* Second pass: check all statements */
    for (int i = 0; i < program->data.block.statements.count; i++) {
        check_stmt(program->data.block.statements.items[i], scope);
    }

    /* Free scope */
    for (int i = 0; i < scope->var_count; i++) free(scope->vars[i]);
    free(scope);

    if (g_errors == 0) {
        printf("%s: no issues found\n", filename);
    } else {
        printf("%s: %d error(s) found\n", filename, g_errors);
    }

    return g_errors;
}
