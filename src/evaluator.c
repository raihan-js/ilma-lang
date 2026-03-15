/*
 * ILMA AST Tree-Walking Evaluator
 *
 * Directly interprets the AST without generating C code.
 * Used for WASM mode (in-browser execution) and future REPL.
 *
 * Design:
 *   - Variables live in EvalScope (linked list for lexical scoping)
 *   - Recipes stored as pointers to their AST nodes
 *   - Blueprints stored with parent info and AST nodes
 *   - capture_output=1 routes ilma_say output to a buffer (for WASM)
 */

#define _POSIX_C_SOURCE 200809L
#include "evaluator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* ── Forward declarations ──────────────────────────────── */

static IlmaValue eval_expr(Evaluator* ev, ASTNode* node);
static void eval_stmt(Evaluator* ev, ASTNode* node);
static void eval_block(Evaluator* ev, ASTNode* block);

/* ── Scope management ─────────────────────────────────── */

static EvalScope* scope_new(void) {
    EvalScope* s = calloc(1, sizeof(EvalScope));
    s->var_count = 0;
    s->parent = NULL;
    return s;
}

static EvalScope* scope_push(Evaluator* ev) {
    EvalScope* s = scope_new();
    s->parent = ev->scope;
    ev->scope = s;
    return s;
}

static void scope_pop(Evaluator* ev) {
    if (!ev->scope) return;
    EvalScope* old = ev->scope;
    ev->scope = old->parent;
    /* Free variable names in the popped scope */
    for (int i = 0; i < old->var_count; i++) {
        free(old->vars[i].name);
    }
    free(old);
}

static void scope_set(Evaluator* ev, const char* name, IlmaValue val) {
    /* Check current scope first — update if exists */
    EvalScope* s = ev->scope;
    while (s) {
        for (int i = 0; i < s->var_count; i++) {
            if (strcmp(s->vars[i].name, name) == 0) {
                s->vars[i].value = val;
                return;
            }
        }
        s = s->parent;
    }
    /* Not found anywhere — create in current scope */
    if (ev->scope->var_count >= ILMA_EVAL_MAX_VARS) {
        fprintf(stderr, "Too many variables in scope (max %d)\n", ILMA_EVAL_MAX_VARS);
        exit(1);
    }
    int idx = ev->scope->var_count++;
    ev->scope->vars[idx].name = strdup(name);
    ev->scope->vars[idx].value = val;
}

static IlmaValue scope_get(Evaluator* ev, const char* name) {
    EvalScope* s = ev->scope;
    while (s) {
        for (int i = 0; i < s->var_count; i++) {
            if (strcmp(s->vars[i].name, name) == 0) {
                return s->vars[i].value;
            }
        }
        s = s->parent;
    }
    /* Not found — return empty */
    return ilma_empty_val();
}

static int scope_exists(Evaluator* ev, const char* name) {
    EvalScope* s = ev->scope;
    while (s) {
        for (int i = 0; i < s->var_count; i++) {
            if (strcmp(s->vars[i].name, name) == 0) {
                return 1;
            }
        }
        s = s->parent;
    }
    return 0;
}

/* ── Recipe / Blueprint lookup ────────────────────────── */

static EvalRecipe* find_recipe(Evaluator* ev, const char* name) {
    for (int i = 0; i < ev->recipe_count; i++) {
        if (strcmp(ev->recipes[i].name, name) == 0) {
            return &ev->recipes[i];
        }
    }
    return NULL;
}

static EvalBlueprint* find_blueprint(Evaluator* ev, const char* name) {
    for (int i = 0; i < ev->blueprint_count; i++) {
        if (strcmp(ev->blueprints[i].name, name) == 0) {
            return &ev->blueprints[i];
        }
    }
    return NULL;
}

static void register_recipe(Evaluator* ev, const char* name, ASTNode* node) {
    if (ev->recipe_count >= ILMA_EVAL_MAX_RECIPES) {
        fprintf(stderr, "Too many recipes (max %d)\n", ILMA_EVAL_MAX_RECIPES);
        exit(1);
    }
    /* Overwrite if already registered (allows redefinition) */
    for (int i = 0; i < ev->recipe_count; i++) {
        if (strcmp(ev->recipes[i].name, name) == 0) {
            ev->recipes[i].node = node;
            return;
        }
    }
    int idx = ev->recipe_count++;
    ev->recipes[idx].name = strdup(name);
    ev->recipes[idx].node = node;
}

static void register_blueprint(Evaluator* ev, const char* name, const char* parent, ASTNode* node) {
    if (ev->blueprint_count >= ILMA_EVAL_MAX_BLUEPRINTS) {
        fprintf(stderr, "Too many blueprints (max %d)\n", ILMA_EVAL_MAX_BLUEPRINTS);
        exit(1);
    }
    int idx = ev->blueprint_count++;
    ev->blueprints[idx].name = strdup(name);
    ev->blueprints[idx].parent = parent ? strdup(parent) : NULL;
    ev->blueprints[idx].node = node;
}

/* ── Output capture ───────────────────────────────────── */

static void output_append(Evaluator* ev, const char* str) {
    int len = (int)strlen(str);
    while (ev->output_len + len + 1 >= ev->output_cap) {
        ev->output_cap = ev->output_cap ? ev->output_cap * 2 : 1024;
        ev->output_buffer = realloc(ev->output_buffer, ev->output_cap);
    }
    memcpy(ev->output_buffer + ev->output_len, str, len);
    ev->output_len += len;
    ev->output_buffer[ev->output_len] = '\0';
}

static void eval_output(Evaluator* ev, IlmaValue val) {
    if (ev->capture_output) {
        char* s = ilma_to_string(val);
        output_append(ev, s);
        output_append(ev, "\n");
        free(s);
    } else {
        ilma_say(val);
    }
}

/* ── Find a method inside a blueprint's members ───────── */

static ASTNode* find_blueprint_method(ASTNode* bp_node, const char* method_name) {
    NodeList* members = &bp_node->data.blueprint.members;
    for (int i = 0; i < members->count; i++) {
        ASTNode* m = members->items[i];
        if (m->type == NODE_RECIPE) {
            if (strcmp(m->data.recipe.name, method_name) == 0) {
                return m;
            }
        }
    }
    return NULL;
}

/* ── Call a recipe (free function or method) ──────────── */

static IlmaValue call_recipe(Evaluator* ev, ASTNode* recipe_node,
                              IlmaValue* args, int arg_count) {
    scope_push(ev);

    /* Bind parameters */
    int param_count = recipe_node->data.recipe.param_count;
    for (int i = 0; i < param_count && i < arg_count; i++) {
        scope_set(ev, recipe_node->data.recipe.params[i], args[i]);
    }
    /* Fill missing params with empty */
    for (int i = arg_count; i < param_count; i++) {
        scope_set(ev, recipe_node->data.recipe.params[i], ilma_empty_val());
    }

    /* Execute body */
    ev->returning = 0;
    eval_block(ev, recipe_node->data.recipe.body);

    IlmaValue result = ilma_empty_val();
    if (ev->returning) {
        result = ev->return_value;
        ev->returning = 0;
    }

    scope_pop(ev);
    return result;
}

/* ── Call a blueprint constructor ─────────────────────── */

static IlmaValue call_blueprint_constructor(Evaluator* ev, EvalBlueprint* bp,
                                             IlmaValue* args, int arg_count);

static void run_parent_constructor(Evaluator* ev, EvalBlueprint* bp,
                                   IlmaValue obj, IlmaValue* args, int arg_count) {
    if (!bp->parent) return;
    EvalBlueprint* parent_bp = find_blueprint(ev, bp->parent);
    if (!parent_bp) return;

    /* Find parent's create method */
    ASTNode* parent_create = find_blueprint_method(parent_bp->node, "create");
    if (!parent_create) return;

    /* Execute parent constructor with "me" bound to this object */
    scope_push(ev);
    scope_set(ev, "me", obj);

    int param_count = parent_create->data.recipe.param_count;
    for (int i = 0; i < param_count && i < arg_count; i++) {
        scope_set(ev, parent_create->data.recipe.params[i], args[i]);
    }
    for (int i = arg_count; i < param_count; i++) {
        scope_set(ev, parent_create->data.recipe.params[i], ilma_empty_val());
    }

    ev->returning = 0;
    eval_block(ev, parent_create->data.recipe.body);
    ev->returning = 0;

    scope_pop(ev);
}

static IlmaValue call_blueprint_constructor(Evaluator* ev, EvalBlueprint* bp,
                                             IlmaValue* args, int arg_count) {
    /* Create the object */
    IlmaValue obj = ilma_obj_new(bp->name);

    /* Run parent constructor first if comes_from */
    if (bp->parent) {
        run_parent_constructor(ev, bp, obj, args, arg_count);
    }

    /* Find the "create" method */
    ASTNode* create = find_blueprint_method(bp->node, "create");
    if (create) {
        scope_push(ev);
        scope_set(ev, "me", obj);

        int param_count = create->data.recipe.param_count;
        for (int i = 0; i < param_count && i < arg_count; i++) {
            scope_set(ev, create->data.recipe.params[i], args[i]);
        }
        for (int i = arg_count; i < param_count; i++) {
            scope_set(ev, create->data.recipe.params[i], ilma_empty_val());
        }

        ev->returning = 0;
        eval_block(ev, create->data.recipe.body);
        ev->returning = 0;

        /* Re-read "me" in case it was modified */
        obj = scope_get(ev, "me");

        scope_pop(ev);
    }

    return obj;
}

/* ── Call a blueprint method ──────────────────────────── */

static IlmaValue call_method(Evaluator* ev, IlmaValue obj, const char* method_name,
                              IlmaValue* args, int arg_count) {
    if (obj.type != ILMA_OBJECT || !obj.as_object) {
        return ilma_empty_val();
    }

    const char* bp_name = ilma_obj_blueprint(obj.as_object);
    EvalBlueprint* bp = bp_name ? find_blueprint(ev, bp_name) : NULL;

    /* Search through blueprint hierarchy */
    while (bp) {
        ASTNode* method = find_blueprint_method(bp->node, method_name);
        if (method) {
            scope_push(ev);
            scope_set(ev, "me", obj);

            int param_count = method->data.recipe.param_count;
            for (int i = 0; i < param_count && i < arg_count; i++) {
                scope_set(ev, method->data.recipe.params[i], args[i]);
            }
            for (int i = arg_count; i < param_count; i++) {
                scope_set(ev, method->data.recipe.params[i], ilma_empty_val());
            }

            ev->returning = 0;
            eval_block(ev, method->data.recipe.body);

            IlmaValue result = ilma_empty_val();
            if (ev->returning) {
                result = ev->return_value;
                ev->returning = 0;
            }

            scope_pop(ev);
            return result;
        }
        /* Walk up the parent chain */
        if (bp->parent) {
            bp = find_blueprint(ev, bp->parent);
        } else {
            break;
        }
    }

    return ilma_empty_val();
}

/* ── Built-in method dispatch on bags, text, notebooks ── */

static IlmaValue call_builtin_method(Evaluator* ev, IlmaValue obj,
                                      const char* method,
                                      IlmaValue* args, int arg_count) {
    (void)ev;

    /* Bag methods */
    if (obj.type == ILMA_BAG && obj.as_bag) {
        if (strcmp(method, "add") == 0 && arg_count >= 1) {
            ilma_bag_add(obj.as_bag, args[0]);
            return ilma_empty_val();
        }
        if (strcmp(method, "remove") == 0 && arg_count >= 1) {
            ilma_bag_remove(obj.as_bag, args[0]);
            return ilma_empty_val();
        }
        if (strcmp(method, "size") == 0) {
            return ilma_bag_size_val(obj.as_bag);
        }
        if (strcmp(method, "sorted") == 0) {
            return ilma_bag_sorted(obj.as_bag);
        }
        if (strcmp(method, "join") == 0) {
            IlmaValue sep = arg_count >= 1 ? args[0] : ilma_text(", ");
            return ilma_text_join(sep, obj.as_bag);
        }
    }

    /* Text methods */
    if (obj.type == ILMA_TEXT) {
        if (strcmp(method, "upper") == 0) {
            return ilma_text_upper(obj);
        }
        if (strcmp(method, "lower") == 0) {
            return ilma_text_lower(obj);
        }
        if (strcmp(method, "length") == 0) {
            return ilma_text_length(obj);
        }
        if (strcmp(method, "contains") == 0 && arg_count >= 1) {
            return ilma_text_contains(obj, args[0]);
        }
        if (strcmp(method, "slice") == 0 && arg_count >= 2) {
            return ilma_text_slice(obj, args[0], args[1]);
        }
    }

    /* Notebook methods */
    if (obj.type == ILMA_NOTEBOOK && obj.as_notebook) {
        if (strcmp(method, "keys") == 0) {
            return ilma_notebook_keys(obj.as_notebook);
        }
        if (strcmp(method, "size") == 0) {
            return ilma_whole(ilma_notebook_size(obj.as_notebook));
        }
    }

    return ilma_empty_val();
}

/* ── Expression evaluation ────────────────────────────── */

static IlmaValue eval_expr(Evaluator* ev, ASTNode* node) {
    if (!node) return ilma_empty_val();

    switch (node->type) {

    case NODE_INT_LIT:
        return ilma_whole(node->data.int_val);

    case NODE_FLOAT_LIT:
        return ilma_decimal(node->data.float_val);

    case NODE_STRING_LIT:
        return ilma_text(node->data.string_val ? node->data.string_val : "");

    case NODE_BOOL_LIT:
        return node->data.bool_val ? ilma_yes() : ilma_no();

    case NODE_EMPTY_LIT:
        return ilma_empty_val();

    case NODE_IDENTIFIER: {
        const char* name = node->data.ident_name;
        if (!name) return ilma_empty_val();

        /* Check "me" first */
        if (strcmp(name, "me") == 0) {
            return scope_get(ev, "me");
        }

        /* Check variable scope */
        if (scope_exists(ev, name)) {
            return scope_get(ev, name);
        }

        /* Could be a blueprint name used as value (rare) */
        return ilma_empty_val();
    }

    case NODE_BINARY_OP: {
        const char* op = node->data.binary.op;
        IlmaValue left = eval_expr(ev, node->data.binary.left);
        /* Short-circuit for "and" / "or" */
        if (strcmp(op, "and") == 0) {
            if (!ilma_is_truthy(left)) return ilma_no();
            IlmaValue right = eval_expr(ev, node->data.binary.right);
            return ilma_is_truthy(right) ? ilma_yes() : ilma_no();
        }
        if (strcmp(op, "or") == 0) {
            if (ilma_is_truthy(left)) return ilma_yes();
            IlmaValue right = eval_expr(ev, node->data.binary.right);
            return ilma_is_truthy(right) ? ilma_yes() : ilma_no();
        }
        IlmaValue right = eval_expr(ev, node->data.binary.right);

        if (strcmp(op, "+") == 0)       return ilma_add(left, right);
        if (strcmp(op, "-") == 0)       return ilma_sub(left, right);
        if (strcmp(op, "*") == 0)       return ilma_mul(left, right);
        if (strcmp(op, "/") == 0)       return ilma_div(left, right);
        if (strcmp(op, "%") == 0)       return ilma_mod(left, right);
        if (strcmp(op, "is") == 0)      return ilma_eq(left, right);
        if (strcmp(op, "is not") == 0)  return ilma_neq(left, right);
        if (strcmp(op, "<") == 0)       return ilma_lt(left, right);
        if (strcmp(op, ">") == 0)       return ilma_gt(left, right);
        if (strcmp(op, "<=") == 0)      return ilma_leq(left, right);
        if (strcmp(op, ">=") == 0)      return ilma_geq(left, right);

        fprintf(stderr, "Unknown binary operator: %s\n", op);
        return ilma_empty_val();
    }

    case NODE_UNARY_OP: {
        const char* op = node->data.unary.op;
        IlmaValue operand = eval_expr(ev, node->data.unary.operand);

        if (strcmp(op, "not") == 0) return ilma_not(operand);
        if (strcmp(op, "-") == 0)   return ilma_neg(operand);

        fprintf(stderr, "Unknown unary operator: %s\n", op);
        return ilma_empty_val();
    }

    case NODE_CALL: {
        ASTNode* callee = node->data.call.callee;
        NodeList* arg_nodes = &node->data.call.args;

        /* Evaluate arguments */
        int arg_count = arg_nodes->count;
        IlmaValue* args = NULL;
        if (arg_count > 0) {
            args = malloc(sizeof(IlmaValue) * arg_count);
            for (int i = 0; i < arg_count; i++) {
                args[i] = eval_expr(ev, arg_nodes->items[i]);
            }
        }

        IlmaValue result = ilma_empty_val();

        if (callee->type == NODE_IDENTIFIER) {
            const char* name = callee->data.ident_name;

            /* Check blueprint constructor first */
            EvalBlueprint* bp = find_blueprint(ev, name);
            if (bp) {
                result = call_blueprint_constructor(ev, bp, args, arg_count);
                free(args);
                return result;
            }

            /* Check recipe */
            EvalRecipe* recipe = find_recipe(ev, name);
            if (recipe) {
                result = call_recipe(ev, recipe->node, args, arg_count);
                free(args);
                return result;
            }

            /* Unknown function */
            fprintf(stderr, "Unknown recipe or blueprint: %s\n", name);
            free(args);
            return ilma_empty_val();
        }

        if (callee->type == NODE_MEMBER_ACCESS) {
            /* Method call: obj.method(args) */
            IlmaValue obj = eval_expr(ev, callee->data.member.object);
            const char* method_name = callee->data.member.member;

            /* Try built-in methods first (bag.add, text.upper, etc.) */
            if (obj.type == ILMA_BAG || obj.type == ILMA_TEXT ||
                obj.type == ILMA_NOTEBOOK) {
                result = call_builtin_method(ev, obj, method_name, args, arg_count);
                /* Check if the built-in method returned something meaningful
                   or if we should try blueprint methods */
                free(args);
                return result;
            }

            /* For objects: check comes_from.create() pattern and blueprint methods */
            if (obj.type == ILMA_OBJECT) {
                result = call_method(ev, obj, method_name, args, arg_count);
                free(args);
                return result;
            }

            /* Fallback: try calling the method as a builtin anyway */
            result = call_builtin_method(ev, obj, method_name, args, arg_count);
            free(args);
            return result;
        }

        /* Fallback: evaluate callee as expression and give up */
        free(args);
        return ilma_empty_val();
    }

    case NODE_MEMBER_ACCESS: {
        IlmaValue obj = eval_expr(ev, node->data.member.object);
        const char* member = node->data.member.member;

        /* Object field access */
        if (obj.type == ILMA_OBJECT && obj.as_object) {
            return ilma_obj_get(obj.as_object, member);
        }

        /* Bag properties */
        if (obj.type == ILMA_BAG && obj.as_bag) {
            if (strcmp(member, "size") == 0) {
                return ilma_bag_size_val(obj.as_bag);
            }
            if (strcmp(member, "length") == 0) {
                return ilma_bag_size_val(obj.as_bag);
            }
        }

        /* Text properties */
        if (obj.type == ILMA_TEXT) {
            if (strcmp(member, "length") == 0) {
                return ilma_text_length(obj);
            }
            if (strcmp(member, "size") == 0) {
                return ilma_text_length(obj);
            }
        }

        /* Notebook properties */
        if (obj.type == ILMA_NOTEBOOK && obj.as_notebook) {
            if (strcmp(member, "size") == 0) {
                return ilma_whole(ilma_notebook_size(obj.as_notebook));
            }
            /* Generic key access on notebooks */
            return ilma_notebook_get(obj.as_notebook, member);
        }

        return ilma_empty_val();
    }

    case NODE_INDEX_ACCESS: {
        IlmaValue obj = eval_expr(ev, node->data.index_access.object);
        IlmaValue index = eval_expr(ev, node->data.index_access.index);

        /* Bag index */
        if (obj.type == ILMA_BAG && obj.as_bag) {
            int idx = (int)index.as_whole;
            return ilma_bag_get(obj.as_bag, idx);
        }

        /* Notebook key access */
        if (obj.type == ILMA_NOTEBOOK && obj.as_notebook) {
            return ilma_notebook_get_val(obj.as_notebook, index);
        }

        /* Text character access */
        if (obj.type == ILMA_TEXT && obj.as_text) {
            int idx = (int)index.as_whole;
            int len = (int)strlen(obj.as_text);
            if (idx >= 0 && idx < len) {
                char buf[2] = { obj.as_text[idx], '\0' };
                return ilma_text(buf);
            }
            return ilma_empty_val();
        }

        return ilma_empty_val();
    }

    case NODE_ASK_EXPR: {
        /* In WASM mode, stdin is not available */
        if (ev->capture_output) {
            return ilma_text("(input not available)");
        }
        /* In native mode, use the runtime ask function */
        IlmaValue prompt = ilma_empty_val();
        if (node->data.ask.prompt) {
            prompt = eval_expr(ev, node->data.ask.prompt);
        }
        return ilma_ask(prompt);
    }

    case NODE_BAG_LIT: {
        IlmaValue bag = ilma_bag_new();
        NodeList* elements = &node->data.bag.elements;
        for (int i = 0; i < elements->count; i++) {
            IlmaValue elem = eval_expr(ev, elements->items[i]);
            ilma_bag_add(bag.as_bag, elem);
        }
        return bag;
    }

    case NODE_NOTEBOOK_LIT: {
        IlmaValue nb = ilma_notebook_new();
        int count = node->data.notebook.values.count;
        for (int i = 0; i < count; i++) {
            const char* key = node->data.notebook.keys[i];
            IlmaValue val = eval_expr(ev, node->data.notebook.values.items[i]);
            ilma_notebook_set(nb.as_notebook, key, val);
        }
        return nb;
    }

    case NODE_STRING_INTERP: {
        /* Concatenate all parts */
        IlmaValue result = ilma_text("");
        NodeList* parts = &node->data.interp.parts;
        for (int i = 0; i < parts->count; i++) {
            IlmaValue part = eval_expr(ev, parts->items[i]);
            IlmaValue new_result = ilma_concat(result, part);
            result = new_result;
        }
        return result;
    }

    /* Statement nodes should not appear in expression context,
       but handle block/program gracefully */
    case NODE_BLOCK:
    case NODE_PROGRAM:
        eval_block(ev, node);
        return ilma_empty_val();

    default:
        /* For any other node type used as expression, try to evaluate as statement */
        eval_stmt(ev, node);
        return ilma_empty_val();
    }
}

/* ── Statement evaluation ─────────────────────────────── */

static void eval_stmt(Evaluator* ev, ASTNode* node) {
    if (!node) return;
    if (ev->returning) return;  /* propagate return up */

    switch (node->type) {

    case NODE_SAY: {
        IlmaValue val = eval_expr(ev, node->data.say.expr);
        eval_output(ev, val);
        break;
    }

    case NODE_REMEMBER: {
        IlmaValue val = eval_expr(ev, node->data.remember.value);
        scope_set(ev, node->data.remember.name, val);
        break;
    }

    case NODE_ASSIGN: {
        ASTNode* target = node->data.assign.target;
        IlmaValue val = eval_expr(ev, node->data.assign.value);

        if (target->type == NODE_IDENTIFIER) {
            scope_set(ev, target->data.ident_name, val);
        }
        else if (target->type == NODE_MEMBER_ACCESS) {
            IlmaValue obj = eval_expr(ev, target->data.member.object);
            const char* member = target->data.member.member;

            if (obj.type == ILMA_OBJECT && obj.as_object) {
                ilma_obj_set(obj.as_object, member, val);
            }
            else if (obj.type == ILMA_NOTEBOOK && obj.as_notebook) {
                ilma_notebook_set(obj.as_notebook, member, val);
            }
        }
        else if (target->type == NODE_INDEX_ACCESS) {
            IlmaValue obj = eval_expr(ev, target->data.index_access.object);
            IlmaValue index = eval_expr(ev, target->data.index_access.index);

            if (obj.type == ILMA_BAG && obj.as_bag) {
                int idx = (int)index.as_whole;
                if (idx >= 0 && idx < obj.as_bag->count) {
                    obj.as_bag->items[idx] = val;
                }
            }
            else if (obj.type == ILMA_NOTEBOOK && obj.as_notebook) {
                char* key = ilma_to_string(index);
                ilma_notebook_set(obj.as_notebook, key, val);
                free(key);
            }
        }
        break;
    }

    case NODE_IF: {
        IlmaValue cond = eval_expr(ev, node->data.if_stmt.condition);
        if (ilma_is_truthy(cond)) {
            if (node->data.if_stmt.then_block) {
                eval_block(ev, node->data.if_stmt.then_block);
            }
        } else {
            if (node->data.if_stmt.else_block) {
                /* else_block can be another NODE_IF (for "otherwise if") or a block */
                if (node->data.if_stmt.else_block->type == NODE_IF) {
                    eval_stmt(ev, node->data.if_stmt.else_block);
                } else {
                    eval_block(ev, node->data.if_stmt.else_block);
                }
            }
        }
        break;
    }

    case NODE_REPEAT: {
        IlmaValue count_val = eval_expr(ev, node->data.repeat.count);
        int count = (int)count_val.as_whole;
        for (int i = 0; i < count && !ev->returning; i++) {
            eval_block(ev, node->data.repeat.body);
        }
        break;
    }

    case NODE_RANGE_LOOP: {
        IlmaValue start_val = eval_expr(ev, node->data.range_loop.start);
        IlmaValue end_val = eval_expr(ev, node->data.range_loop.end);
        int64_t start = start_val.as_whole;
        int64_t end = end_val.as_whole;
        const char* var = node->data.range_loop.var_name;

        if (start <= end) {
            for (int64_t i = start; i <= end && !ev->returning; i++) {
                scope_set(ev, var, ilma_whole(i));
                eval_block(ev, node->data.range_loop.body);
            }
        } else {
            for (int64_t i = start; i >= end && !ev->returning; i--) {
                scope_set(ev, var, ilma_whole(i));
                eval_block(ev, node->data.range_loop.body);
            }
        }
        break;
    }

    case NODE_WHILE: {
        while (!ev->returning) {
            IlmaValue cond = eval_expr(ev, node->data.while_stmt.condition);
            if (!ilma_is_truthy(cond)) break;
            eval_block(ev, node->data.while_stmt.body);
        }
        break;
    }

    case NODE_FOR_EACH: {
        const char* var = node->data.for_each.var_name;
        IlmaValue iterable = eval_expr(ev, node->data.for_each.iterable);

        if (iterable.type == ILMA_BAG && iterable.as_bag) {
            for (int i = 0; i < iterable.as_bag->count && !ev->returning; i++) {
                scope_set(ev, var, iterable.as_bag->items[i]);
                eval_block(ev, node->data.for_each.body);
            }
        }
        else if (iterable.type == ILMA_NOTEBOOK && iterable.as_notebook) {
            /* Iterate over keys */
            IlmaBook* book = iterable.as_notebook;
            for (int i = 0; i < book->capacity && !ev->returning; i++) {
                if (book->entries[i].used) {
                    scope_set(ev, var, ilma_text(book->entries[i].key));
                    eval_block(ev, node->data.for_each.body);
                }
            }
        }
        else if (iterable.type == ILMA_TEXT && iterable.as_text) {
            /* Iterate over characters */
            size_t len = strlen(iterable.as_text);
            for (size_t i = 0; i < len && !ev->returning; i++) {
                char buf[2] = { iterable.as_text[i], '\0' };
                scope_set(ev, var, ilma_text(buf));
                eval_block(ev, node->data.for_each.body);
            }
        }
        break;
    }

    case NODE_RECIPE: {
        register_recipe(ev, node->data.recipe.name, node);
        break;
    }

    case NODE_GIVE_BACK: {
        if (node->data.give_back.value) {
            ev->return_value = eval_expr(ev, node->data.give_back.value);
        } else {
            ev->return_value = ilma_empty_val();
        }
        ev->returning = 1;
        break;
    }

    case NODE_BLUEPRINT: {
        register_blueprint(ev, node->data.blueprint.name,
                           node->data.blueprint.parent, node);
        break;
    }

    case NODE_USE: {
        /* Module loading is not available in interpreter mode */
        /* No-op — modules would need to be linked at compile time */
        (void)node->data.use.module_name;
        break;
    }

    case NODE_SHOUT: {
        IlmaValue msg = eval_expr(ev, node->data.shout.message);
        char* msg_str = ilma_to_string(msg);
        ilma_shout(msg_str, node->line);
        free(msg_str);
        break;
    }

    case NODE_TRY: {
        /* Use the runtime error stack with setjmp/longjmp */
        if (ilma_error_stack.depth >= ILMA_MAX_TRY_DEPTH) {
            fprintf(stderr, "Too many nested try blocks\n");
            exit(1);
        }

        int idx = ilma_error_stack.depth;
        ilma_error_stack.depth++;
        ilma_error_stack.error_msg[idx] = NULL;

        if (setjmp(ilma_error_stack.buf[idx]) == 0) {
            /* Normal execution path — run try block */
            eval_block(ev, node->data.try_stmt.try_block);
            ilma_error_stack.depth--;
        } else {
            /* Error was thrown — run when wrong block */
            ilma_error_stack.depth--;
            if (ilma_error_stack.error_msg[idx]) {
                /* Make the error message available as "error" variable */
                scope_set(ev, "error", ilma_text(ilma_error_stack.error_msg[idx]));
                free(ilma_error_stack.error_msg[idx]);
                ilma_error_stack.error_msg[idx] = NULL;
            }
            if (node->data.try_stmt.when_wrong_block) {
                eval_block(ev, node->data.try_stmt.when_wrong_block);
            }
        }
        break;
    }

    case NODE_EXPR_STMT: {
        /* The parser reuses data.say.expr to store the inner expression */
        eval_expr(ev, node->data.say.expr);
        break;
    }

    case NODE_BLOCK:
    case NODE_PROGRAM: {
        eval_block(ev, node);
        break;
    }

    default:
        /* Try to evaluate as expression (e.g., bare function calls) */
        eval_expr(ev, node);
        break;
    }
}

/* ── Block evaluation ─────────────────────────────────── */

static void eval_block(Evaluator* ev, ASTNode* block) {
    if (!block) return;

    if (block->type == NODE_BLOCK || block->type == NODE_PROGRAM) {
        NodeList* stmts = &block->data.block.statements;
        for (int i = 0; i < stmts->count && !ev->returning; i++) {
            eval_stmt(ev, stmts->items[i]);
        }
    } else {
        /* Single statement as block */
        eval_stmt(ev, block);
    }
}

/* ── Public API ───────────────────────────────────────── */

void evaluator_init(Evaluator* ev) {
    memset(ev, 0, sizeof(Evaluator));
    ev->scope = scope_new();
    ev->recipe_count = 0;
    ev->blueprint_count = 0;
    ev->output_buffer = NULL;
    ev->output_len = 0;
    ev->output_cap = 0;
    ev->capture_output = 0;
    ev->returning = 0;
    ev->return_value = ilma_empty_val();
}

void evaluator_free(Evaluator* ev) {
    /* Free all scopes */
    while (ev->scope) {
        scope_pop(ev);
    }
    /* Free recipe names */
    for (int i = 0; i < ev->recipe_count; i++) {
        free(ev->recipes[i].name);
    }
    /* Free blueprint names and parent names */
    for (int i = 0; i < ev->blueprint_count; i++) {
        free(ev->blueprints[i].name);
        free(ev->blueprints[i].parent);
    }
    /* Free output buffer */
    free(ev->output_buffer);
    ev->output_buffer = NULL;
    ev->output_len = 0;
    ev->output_cap = 0;
}

char* evaluator_get_output(Evaluator* ev) {
    if (!ev->output_buffer) return strdup("");
    return strdup(ev->output_buffer);
}

IlmaValue evaluator_run(Evaluator* ev, ASTNode* program) {
    if (!program) return ilma_empty_val();

    if (program->type == NODE_PROGRAM || program->type == NODE_BLOCK) {
        NodeList* stmts = &program->data.block.statements;

        /* First pass: register all top-level recipes and blueprints */
        for (int i = 0; i < stmts->count; i++) {
            ASTNode* stmt = stmts->items[i];
            if (stmt->type == NODE_RECIPE) {
                register_recipe(ev, stmt->data.recipe.name, stmt);
            } else if (stmt->type == NODE_BLUEPRINT) {
                register_blueprint(ev, stmt->data.blueprint.name,
                                   stmt->data.blueprint.parent, stmt);
            }
        }

        /* Second pass: execute all statements in order */
        for (int i = 0; i < stmts->count && !ev->returning; i++) {
            ASTNode* stmt = stmts->items[i];
            /* Skip recipes and blueprints — already registered */
            if (stmt->type == NODE_RECIPE || stmt->type == NODE_BLUEPRINT) {
                continue;
            }
            eval_stmt(ev, stmt);
        }
    } else {
        eval_stmt(ev, program);
    }

    if (ev->returning) {
        return ev->return_value;
    }
    return ilma_empty_val();
}

/* ── WASM entry point ─────────────────────────────────── */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "lexer.h"
#include "parser.h"

static char* json_escape(const char* s) {
    size_t len = strlen(s);
    char* buf = malloc(len * 6 + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
            case '"':  buf[j++] = '\\'; buf[j++] = '"';  break;
            case '\\': buf[j++] = '\\'; buf[j++] = '\\'; break;
            case '\n': buf[j++] = '\\'; buf[j++] = 'n';  break;
            case '\r': buf[j++] = '\\'; buf[j++] = 'r';  break;
            case '\t': buf[j++] = '\\'; buf[j++] = 't';  break;
            default:   buf[j++] = s[i]; break;
        }
    }
    buf[j] = '\0';
    return buf;
}

EMSCRIPTEN_KEEPALIVE
const char* wasm_run_ilma(const char* source) {
    static char result[65536];

    Lexer lexer;
    lexer_init(&lexer, source);
    lexer_tokenize(&lexer);

    Parser parser;
    parser_init(&parser, lexer.tokens, lexer.token_count);
    ASTNode* program = parser_parse(&parser);

    Evaluator ev;
    evaluator_init(&ev);
    ev.capture_output = 1;

    evaluator_run(&ev, program);

    char* output = evaluator_get_output(&ev);
    char* escaped = json_escape(output ? output : "");
    snprintf(result, sizeof(result),
             "{\"output\":\"%s\",\"error\":null}", escaped);

    free(escaped);
    free(output);
    evaluator_free(&ev);
    ast_free(program);
    lexer_free(&lexer);

    return result;
}
#endif
