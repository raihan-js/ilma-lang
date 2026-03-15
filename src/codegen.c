#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* ── Helpers ───────────────────────────────────────────── */

static void emit(CodeGen* cg, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    for (int i = 0; i < cg->indent; i++) fprintf(cg->out, "    ");
    vfprintf(cg->out, fmt, args);
    va_end(args);
}


/* Sanitize identifier: replace non-ASCII with _uXXXX */
static char* sanitize_ident(const char* name) {
    size_t len = strlen(name);
    char* buf = malloc(len * 7 + 1); /* worst case */
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (isalnum(c) || c == '_') {
            buf[j++] = c;
        } else {
            j += sprintf(buf + j, "_u%02X", c);
        }
    }
    buf[j] = '\0';
    return buf;
}

/* Escape a C string */
static char* escape_c_string(const char* s) {
    size_t len = strlen(s);
    char* buf = malloc(len * 4 + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
            case '\n': buf[j++] = '\\'; buf[j++] = 'n'; break;
            case '\t': buf[j++] = '\\'; buf[j++] = 't'; break;
            case '\\': buf[j++] = '\\'; buf[j++] = '\\'; break;
            case '"':  buf[j++] = '\\'; buf[j++] = '"'; break;
            default:   buf[j++] = s[i]; break;
        }
    }
    buf[j] = '\0';
    return buf;
}

/* Forward declarations */
static void gen_expression(CodeGen* cg, ASTNode* node);
static void gen_statement(CodeGen* cg, ASTNode* node);
static void gen_block(CodeGen* cg, ASTNode* node);

/* ── Collect top-level recipes and blueprints for forward declarations ── */

typedef struct {
    ASTNode** items;
    int count;
    int capacity;
} ASTList;

static void ast_list_init(ASTList* list, int cap) {
    list->items = malloc(sizeof(ASTNode*) * cap);
    list->count = 0;
    list->capacity = cap;
}

static void ast_list_add(ASTList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(ASTNode*) * list->capacity);
    }
    list->items[list->count++] = node;
}

typedef ASTList RecipeList;

static void collect_recipes(ASTNode* program, RecipeList* list) {
    if (program->type != NODE_PROGRAM && program->type != NODE_BLOCK) return;
    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* stmt = program->data.block.statements.items[i];
        if (stmt->type == NODE_RECIPE) {
            ast_list_add(list, stmt);
        }
    }
}

/* Recursively collect ALL NODE_RECIPE nodes (including lambdas inside expressions) */
static void collect_all_recipes_recursive(ASTNode* node, RecipeList* list) {
    if (!node) return;
    if (node->type == NODE_RECIPE) {
        ast_list_add(list, node);
        /* Still recurse into the body for nested lambdas */
        collect_all_recipes_recursive(node->data.recipe.body, list);
        return;
    }
    switch (node->type) {
        case NODE_PROGRAM:
        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statements.count; i++)
                collect_all_recipes_recursive(node->data.block.statements.items[i], list);
            break;
        case NODE_SAY:
        case NODE_EXPR_STMT:
            collect_all_recipes_recursive(node->data.say.expr, list);
            break;
        case NODE_REMEMBER:
            collect_all_recipes_recursive(node->data.remember.value, list);
            break;
        case NODE_ASSIGN:
            collect_all_recipes_recursive(node->data.assign.target, list);
            collect_all_recipes_recursive(node->data.assign.value, list);
            break;
        case NODE_IF:
            collect_all_recipes_recursive(node->data.if_stmt.condition, list);
            collect_all_recipes_recursive(node->data.if_stmt.then_block, list);
            collect_all_recipes_recursive(node->data.if_stmt.else_block, list);
            break;
        case NODE_CHECK:
            collect_all_recipes_recursive(node->data.check_stmt.subject, list);
            for (int i = 0; i < node->data.check_stmt.case_count; i++) {
                collect_all_recipes_recursive(node->data.check_stmt.case_exprs[i], list);
                collect_all_recipes_recursive(node->data.check_stmt.case_range_ends[i], list);
                /* Avoid duplicating body traversals for shared "or" bodies */
                int is_dup = 0;
                for (int j = 0; j < i; j++) {
                    if (node->data.check_stmt.case_bodies[j] == node->data.check_stmt.case_bodies[i]) {
                        is_dup = 1;
                        break;
                    }
                }
                if (!is_dup) collect_all_recipes_recursive(node->data.check_stmt.case_bodies[i], list);
            }
            collect_all_recipes_recursive(node->data.check_stmt.otherwise_body, list);
            break;
        case NODE_REPEAT:
            collect_all_recipes_recursive(node->data.repeat.count, list);
            collect_all_recipes_recursive(node->data.repeat.body, list);
            break;
        case NODE_RANGE_LOOP:
            collect_all_recipes_recursive(node->data.range_loop.start, list);
            collect_all_recipes_recursive(node->data.range_loop.end, list);
            collect_all_recipes_recursive(node->data.range_loop.body, list);
            break;
        case NODE_WHILE:
            collect_all_recipes_recursive(node->data.while_stmt.condition, list);
            collect_all_recipes_recursive(node->data.while_stmt.body, list);
            break;
        case NODE_FOR_EACH:
            collect_all_recipes_recursive(node->data.for_each.iterable, list);
            collect_all_recipes_recursive(node->data.for_each.body, list);
            break;
        case NODE_TRY:
            collect_all_recipes_recursive(node->data.try_stmt.try_block, list);
            collect_all_recipes_recursive(node->data.try_stmt.when_wrong_block, list);
            break;
        case NODE_GIVE_BACK:
            collect_all_recipes_recursive(node->data.give_back.value, list);
            break;
        case NODE_CALL:
            collect_all_recipes_recursive(node->data.call.callee, list);
            for (int i = 0; i < node->data.call.args.count; i++)
                collect_all_recipes_recursive(node->data.call.args.items[i], list);
            break;
        case NODE_BINARY_OP:
            collect_all_recipes_recursive(node->data.binary.left, list);
            collect_all_recipes_recursive(node->data.binary.right, list);
            break;
        case NODE_UNARY_OP:
            collect_all_recipes_recursive(node->data.unary.operand, list);
            break;
        case NODE_MEMBER_ACCESS:
            collect_all_recipes_recursive(node->data.member.object, list);
            break;
        case NODE_INDEX_ACCESS:
            collect_all_recipes_recursive(node->data.index_access.object, list);
            collect_all_recipes_recursive(node->data.index_access.index, list);
            break;
        case NODE_SHOUT:
            collect_all_recipes_recursive(node->data.shout.message, list);
            break;
        case NODE_STRING_INTERP:
            for (int i = 0; i < node->data.interp.parts.count; i++)
                collect_all_recipes_recursive(node->data.interp.parts.items[i], list);
            break;
        case NODE_BAG_LIT:
            for (int i = 0; i < node->data.bag.elements.count; i++)
                collect_all_recipes_recursive(node->data.bag.elements.items[i], list);
            break;
        case NODE_NOTEBOOK_LIT:
            for (int i = 0; i < node->data.notebook.values.count; i++)
                collect_all_recipes_recursive(node->data.notebook.values.items[i], list);
            break;
        default:
            break;
    }
}

static void collect_blueprints(ASTNode* program, ASTList* list) {
    if (program->type != NODE_PROGRAM && program->type != NODE_BLOCK) return;
    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* stmt = program->data.block.statements.items[i];
        if (stmt->type == NODE_BLUEPRINT) {
            ast_list_add(list, stmt);
        }
    }
}

/* Track blueprint names globally so codegen can resolve constructor calls */
static char** known_blueprints = NULL;
static int known_blueprint_count = 0;

static int is_blueprint_name(const char* name) {
    for (int i = 0; i < known_blueprint_count; i++) {
        if (strcmp(known_blueprints[i], name) == 0) return 1;
    }
    return 0;
}

/* Track blueprint parent relationships */
static char** blueprint_parents = NULL; /* parallel array with known_blueprints */

static const char* get_blueprint_parent(const char* name) {
    for (int i = 0; i < known_blueprint_count; i++) {
        if (strcmp(known_blueprints[i], name) == 0) {
            return blueprint_parents[i];
        }
    }
    return NULL;
}

/* Track method name → blueprint name mapping */
typedef struct {
    char* method_name;
    char* blueprint_name;
    int   param_count;
} MethodMapping;

static MethodMapping* method_mappings = NULL;
static int method_mapping_count = 0;
static int method_mapping_cap = 0;

static void register_method(const char* bp_name, const char* method, int param_count) {
    if (method_mapping_count >= method_mapping_cap) {
        method_mapping_cap = method_mapping_cap ? method_mapping_cap * 2 : 32;
        method_mappings = realloc(method_mappings, sizeof(MethodMapping) * method_mapping_cap);
    }
    method_mappings[method_mapping_count].method_name = strdup(method);
    method_mappings[method_mapping_count].blueprint_name = strdup(bp_name);
    method_mappings[method_mapping_count].param_count = param_count;
    method_mapping_count++;
}

/* Count how many blueprints define this method */
static int method_definition_count(const char* method) {
    int count = 0;
    for (int i = 0; i < method_mapping_count; i++) {
        if (strcmp(method_mappings[i].method_name, method) == 0) {
            count++;
        }
    }
    return count;
}

/* Check if a method needs runtime dispatch (defined in multiple blueprints) */
static int method_needs_dispatch(const char* method) {
    return method_definition_count(method) > 1;
}

/* Find which blueprint defines a method (for non-overridden methods) */
static const char* find_method_blueprint(const char* method) {
    /* Return the last registered one (child overrides parent) */
    for (int i = method_mapping_count - 1; i >= 0; i--) {
        if (strcmp(method_mappings[i].method_name, method) == 0) {
            return method_mappings[i].blueprint_name;
        }
    }
    return NULL;
}

/* Get all blueprint names that define a method */
static int get_method_blueprints(const char* method, const char** out, int max) {
    int count = 0;
    for (int i = 0; i < method_mapping_count && count < max; i++) {
        if (strcmp(method_mappings[i].method_name, method) == 0) {
            out[count++] = method_mappings[i].blueprint_name;
        }
    }
    return count;
}

/* ── Expression generation ────────────────────────────── */

static void gen_expression(CodeGen* cg, ASTNode* node) {
    switch (node->type) {
        case NODE_INT_LIT:
            fprintf(cg->out, "ilma_whole(%ldLL)", (long)node->data.int_val);
            break;

        case NODE_FLOAT_LIT:
            fprintf(cg->out, "ilma_decimal(%g)", node->data.float_val);
            break;

        case NODE_STRING_LIT: {
            char* esc = escape_c_string(node->data.string_val);
            fprintf(cg->out, "ilma_text(\"%s\")", esc);
            free(esc);
            break;
        }

        case NODE_BOOL_LIT:
            fprintf(cg->out, node->data.bool_val ? "ilma_yes()" : "ilma_no()");
            break;

        case NODE_EMPTY_LIT:
            fprintf(cg->out, "ilma_empty_val()");
            break;

        case NODE_IDENTIFIER: {
            char* safe = sanitize_ident(node->data.ident_name);
            fprintf(cg->out, "%s", safe);
            free(safe);
            break;
        }

        case NODE_BINARY_OP: {
            const char* op = node->data.binary.op;
            const char* func = NULL;
            if (strcmp(op, "+") == 0) func = "ilma_add";
            else if (strcmp(op, "-") == 0) func = "ilma_sub";
            else if (strcmp(op, "*") == 0) func = "ilma_mul";
            else if (strcmp(op, "/") == 0) func = "ilma_div";
            else if (strcmp(op, "%") == 0) func = "ilma_mod";
            else if (strcmp(op, "is") == 0) func = "ilma_eq";
            else if (strcmp(op, "is not") == 0) func = "ilma_neq";
            else if (strcmp(op, "<") == 0) func = "ilma_lt";
            else if (strcmp(op, ">") == 0) func = "ilma_gt";
            else if (strcmp(op, "<=") == 0) func = "ilma_leq";
            else if (strcmp(op, ">=") == 0) func = "ilma_geq";
            else if (strcmp(op, "and") == 0) func = "ilma_and";
            else if (strcmp(op, "or") == 0) func = "ilma_or";

            if (func) {
                fprintf(cg->out, "%s(", func);
                gen_expression(cg, node->data.binary.left);
                fprintf(cg->out, ", ");
                gen_expression(cg, node->data.binary.right);
                fprintf(cg->out, ")");
            }
            break;
        }

        case NODE_UNARY_OP: {
            const char* op = node->data.unary.op;
            if (strcmp(op, "not") == 0) {
                fprintf(cg->out, "ilma_not(");
                gen_expression(cg, node->data.unary.operand);
                fprintf(cg->out, ")");
            } else if (strcmp(op, "-") == 0) {
                fprintf(cg->out, "ilma_neg(");
                gen_expression(cg, node->data.unary.operand);
                fprintf(cg->out, ")");
            }
            break;
        }

        case NODE_CALL: {
            /* Generate function call */
            ASTNode* callee = node->data.call.callee;
            if (callee->type == NODE_IDENTIFIER) {
                const char* name = callee->data.ident_name;
                char* safe = sanitize_ident(name);
                if (is_blueprint_name(name)) {
                    /* Constructor call: ClassName(args) */
                    fprintf(cg->out, "ilma_bp_%s_create(", safe);
                } else if (strcmp(name, "read_file") == 0 ||
                           strcmp(name, "write_file") == 0 ||
                           strcmp(name, "file_exists") == 0) {
                    /* Built-in file I/O functions */
                    fprintf(cg->out, "ilma_%s(", name);
                } else {
                    fprintf(cg->out, "ilma_recipe_%s(", safe);
                }
                free(safe);
            } else if (callee->type == NODE_MEMBER_ACCESS) {
                const char* method = callee->data.member.member;
                ASTNode* obj = callee->data.member.object;

                /* comes_from.create(args) — call parent constructor init */
                if (obj->type == NODE_IDENTIFIER &&
                    strcmp(obj->data.ident_name, "comes_from") == 0 &&
                    strcmp(method, "create") == 0 && cg->in_blueprint) {
                    const char* parent = get_blueprint_parent(cg->current_bp);
                    if (parent) {
                        char* safe_parent = sanitize_ident(parent);
                        fprintf(cg->out, "(ilma_bp_%s_create_init(me", safe_parent);
                        for (int i = 0; i < node->data.call.args.count; i++) {
                            fprintf(cg->out, ", ");
                            gen_expression(cg, node->data.call.args.items[i]);
                        }
                        fprintf(cg->out, "), ilma_empty_val())");
                        free(safe_parent);
                        break;
                    }
                }

                /* comes_from.method(args) — call parent method */
                if (obj->type == NODE_IDENTIFIER &&
                    strcmp(obj->data.ident_name, "comes_from") == 0 && cg->in_blueprint) {
                    const char* parent = get_blueprint_parent(cg->current_bp);
                    if (parent) {
                        char* safe_parent = sanitize_ident(parent);
                        fprintf(cg->out, "ilma_bp_%s_%s(me", safe_parent, method);
                        for (int i = 0; i < node->data.call.args.count; i++) {
                            fprintf(cg->out, ", ");
                            gen_expression(cg, node->data.call.args.items[i]);
                        }
                        fprintf(cg->out, ")");
                        free(safe_parent);
                        break;
                    }
                }

                /* Bag methods: obj.add(x), obj.remove(x), obj.sorted() */
                if (strcmp(method, "add") == 0) {
                    fprintf(cg->out, "(ilma_bag_add(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ".as_bag, ");
                    for (int i = 0; i < node->data.call.args.count; i++) {
                        if (i > 0) fprintf(cg->out, ", ");
                        gen_expression(cg, node->data.call.args.items[i]);
                    }
                    fprintf(cg->out, "), ilma_empty_val())");
                    break;
                } else if (strcmp(method, "remove") == 0) {
                    fprintf(cg->out, "(ilma_bag_remove(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ".as_bag, ");
                    for (int i = 0; i < node->data.call.args.count; i++) {
                        if (i > 0) fprintf(cg->out, ", ");
                        gen_expression(cg, node->data.call.args.items[i]);
                    }
                    fprintf(cg->out, "), ilma_empty_val())");
                    break;
                } else if (strcmp(method, "sorted") == 0) {
                    fprintf(cg->out, "ilma_bag_sorted(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ".as_bag)");
                    break;
                }
                /* Higher-order bag methods: map, filter, each */
                else if (strcmp(method, "map") == 0 || strcmp(method, "filter") == 0 || strcmp(method, "each") == 0) {
                    const char* rt_fn = strcmp(method, "map") == 0 ? "ilma_bag_map" :
                                         strcmp(method, "filter") == 0 ? "ilma_bag_filter" : "ilma_bag_each";
                    if (strcmp(method, "each") == 0) {
                        fprintf(cg->out, "(");
                    }
                    fprintf(cg->out, "%s(", rt_fn);
                    gen_expression(cg, obj);
                    fprintf(cg->out, ".as_bag, ");
                    if (node->data.call.args.count > 0) {
                        ASTNode* arg = node->data.call.args.items[0];
                        if (arg->type == NODE_RECIPE) {
                            char* safe_ln = sanitize_ident(arg->data.recipe.name);
                            fprintf(cg->out, "ilma_recipe_%s", safe_ln);
                            free(safe_ln);
                        } else if (arg->type == NODE_IDENTIFIER) {
                            char* safe_ln = sanitize_ident(arg->data.ident_name);
                            fprintf(cg->out, "ilma_recipe_%s", safe_ln);
                            free(safe_ln);
                        }
                    }
                    fprintf(cg->out, ")");
                    if (strcmp(method, "each") == 0) {
                        fprintf(cg->out, ", ilma_empty_val())");
                    }
                    break;
                }
                /* Text methods: text.join(bag), text.length(), text.upper(), etc. */
                else if (strcmp(method, "join") == 0) {
                    fprintf(cg->out, "ilma_text_join(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ", ");
                    if (node->data.call.args.count > 0) {
                        gen_expression(cg, node->data.call.args.items[0]);
                        fprintf(cg->out, ".as_bag");
                    }
                    fprintf(cg->out, ")");
                    break;
                } else if (strcmp(method, "upper") == 0) {
                    fprintf(cg->out, "ilma_text_upper(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ")");
                    break;
                } else if (strcmp(method, "lower") == 0) {
                    fprintf(cg->out, "ilma_text_lower(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ")");
                    break;
                } else if (strcmp(method, "contains") == 0) {
                    fprintf(cg->out, "ilma_text_contains(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ", ");
                    if (node->data.call.args.count > 0) {
                        gen_expression(cg, node->data.call.args.items[0]);
                    }
                    fprintf(cg->out, ")");
                    break;
                } else if (strcmp(method, "slice") == 0) {
                    fprintf(cg->out, "ilma_text_slice(");
                    gen_expression(cg, obj);
                    for (int i = 0; i < node->data.call.args.count; i++) {
                        fprintf(cg->out, ", ");
                        gen_expression(cg, node->data.call.args.items[i]);
                    }
                    fprintf(cg->out, ")");
                    break;
                }

                /* Module method call — e.g. finance.compound(args) */
                if (obj->type == NODE_IDENTIFIER) {
                    const char* mod_name = obj->data.ident_name;
                    int is_module = 0;
                    for (int m = 0; m < cg->used_module_count; m++) {
                        if (strcmp(cg->used_modules[m], mod_name) == 0) {
                            is_module = 1;
                            break;
                        }
                    }
                    if (is_module) {
                        /* Map module.method to ilma_module_method */
                        if (strcmp(mod_name, "time") == 0)
                            fprintf(cg->out, "ilma_time_%s(", method);
                        else if (strcmp(mod_name, "draw") == 0 && strcmp(method, "text") == 0)
                            fprintf(cg->out, "ilma_draw_text_elem(");
                        else
                            fprintf(cg->out, "ilma_%s_%s(", mod_name, method);
                        for (int i = 0; i < node->data.call.args.count; i++) {
                            if (i > 0) fprintf(cg->out, ", ");
                            gen_expression(cg, node->data.call.args.items[i]);
                        }
                        fprintf(cg->out, ")");
                        break;
                    }
                }

                /* Blueprint method call — obj.method(args) */
                {
                    const char* bp = find_method_blueprint(method);
                    if (bp) {
                        if (method_needs_dispatch(method)) {
                            /* Overridden method — dispatch based on runtime type */
                            fprintf(cg->out, "ilma_dispatch_%s(", method);
                        } else {
                            char* safe_bp = sanitize_ident(bp);
                            fprintf(cg->out, "ilma_bp_%s_%s(", safe_bp, method);
                            free(safe_bp);
                        }
                        gen_expression(cg, obj);
                        if (node->data.call.args.count > 0) fprintf(cg->out, ", ");
                        for (int i = 0; i < node->data.call.args.count; i++) {
                            if (i > 0) fprintf(cg->out, ", ");
                            gen_expression(cg, node->data.call.args.items[i]);
                        }
                        fprintf(cg->out, ")");
                        break;
                    }
                }
                /* Truly generic method call */
                gen_expression(cg, obj);
                fprintf(cg->out, "_%s(", method);
            } else {
                fprintf(cg->out, "/* unknown callee */ (");
            }
            for (int i = 0; i < node->data.call.args.count; i++) {
                if (i > 0) fprintf(cg->out, ", ");
                gen_expression(cg, node->data.call.args.items[i]);
            }
            fprintf(cg->out, ")");
            break;
        }

        case NODE_MEMBER_ACCESS: {
            const char* member = node->data.member.member;
            ASTNode* obj = node->data.member.object;

            /* me.field inside a blueprint — object field access */
            if (obj->type == NODE_IDENTIFIER && strcmp(obj->data.ident_name, "me") == 0 &&
                cg->in_blueprint) {
                fprintf(cg->out, "ilma_obj_get(me.as_object, \"%s\")", member);
                break;
            }

            /* obj.field on any object variable — dynamic field access */
            if (obj->type == NODE_IDENTIFIER) {
                const char* obj_name = obj->data.ident_name;
                /* Don't intercept bag.size or text.length — they have their own paths */
                (void)obj_name;
            }

            /* Bag properties: obj.size */
            if (strcmp(member, "size") == 0) {
                fprintf(cg->out, "ilma_bag_size_val(");
                gen_expression(cg, obj);
                fprintf(cg->out, ".as_bag)");
                break;
            }

            /* Text properties: obj.length */
            if (strcmp(member, "length") == 0) {
                fprintf(cg->out, "ilma_text_length(");
                gen_expression(cg, obj);
                fprintf(cg->out, ")");
                break;
            }

            /* Generic object field access (for non-me objects) */
            fprintf(cg->out, "ilma_obj_get(");
            gen_expression(cg, obj);
            fprintf(cg->out, ".as_object, \"%s\")", member);
            break;
        }

        case NODE_INDEX_ACCESS: {
            ASTNode* obj = node->data.index_access.object;
            ASTNode* idx = node->data.index_access.index;
            if (idx->type == NODE_STRING_LIT) {
                /* Static notebook key: notebook[name] → ilma_notebook_get(book, "name") */
                char* esc = escape_c_string(idx->data.string_val);
                fprintf(cg->out, "ilma_notebook_get(");
                gen_expression(cg, obj);
                fprintf(cg->out, ".as_notebook, \"%s\")", esc);
                free(esc);
            } else if (idx->type == NODE_INT_LIT) {
                /* Numeric index for bags */
                fprintf(cg->out, "ilma_bag_get(");
                gen_expression(cg, obj);
                fprintf(cg->out, ".as_bag, ");
                gen_expression(cg, idx);
                fprintf(cg->out, ".as_whole)");
            } else {
                /* Dynamic index: could be bag index or notebook key variable */
                fprintf(cg->out, "ilma_notebook_get_val(");
                gen_expression(cg, obj);
                fprintf(cg->out, ".as_notebook, ");
                gen_expression(cg, idx);
                fprintf(cg->out, ")");
            }
            break;
        }

        case NODE_ASK_EXPR:
            fprintf(cg->out, "ilma_ask(");
            gen_expression(cg, node->data.ask.prompt);
            fprintf(cg->out, ")");
            break;

        case NODE_BAG_LIT: {
            fprintf(cg->out, "ilma_bag_new()");
            break;
        }

        case NODE_NOTEBOOK_LIT: {
            fprintf(cg->out, "ilma_notebook_new()");
            break;
        }

        case NODE_STRING_INTERP: {
            NodeList* parts = &node->data.interp.parts;
            if (parts->count == 0) {
                fprintf(cg->out, "ilma_text(\"\")");
            } else if (parts->count == 1) {
                gen_expression(cg, parts->items[0]);
            } else {
                /* Chain ilma_add calls: ilma_add(ilma_add(part0, part1), part2) */
                for (int i = 0; i < parts->count - 1; i++) {
                    fprintf(cg->out, "ilma_add(");
                }
                gen_expression(cg, parts->items[0]);
                for (int i = 1; i < parts->count; i++) {
                    fprintf(cg->out, ", ");
                    gen_expression(cg, parts->items[i]);
                    fprintf(cg->out, ")");
                }
            }
            break;
        }

        case NODE_RECIPE: {
            /* Lambda expression used in expression context — reference the generated function */
            char* safe = sanitize_ident(node->data.recipe.name);
            fprintf(cg->out, "ilma_empty_val() /* lambda %s */", safe);
            free(safe);
            break;
        }

        default:
            fprintf(cg->out, "/* unhandled expression type %d */ilma_empty_val()", node->type);
            break;
    }
}

/* ── Statement generation ─────────────────────────────── */

static void gen_block(CodeGen* cg, ASTNode* node) {
    if (!node) return;
    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->data.block.statements.count; i++) {
            gen_statement(cg, node->data.block.statements.items[i]);
        }
    } else {
        gen_statement(cg, node);
    }
}

static void gen_bag_init(CodeGen* cg, const char* var_name, ASTNode* bag_lit) {
    for (int i = 0; i < bag_lit->data.bag.elements.count; i++) {
        emit(cg, "ilma_bag_add(%s.as_bag, ", var_name);
        gen_expression(cg, bag_lit->data.bag.elements.items[i]);
        fprintf(cg->out, ");\n");
    }
}

static void gen_notebook_init(CodeGen* cg, const char* var_name, ASTNode* nb_lit) {
    for (int i = 0; i < nb_lit->data.notebook.values.count; i++) {
        char* esc = escape_c_string(nb_lit->data.notebook.keys[i]);
        emit(cg, "ilma_notebook_set(%s.as_notebook, \"%s\", ", var_name, esc);
        gen_expression(cg, nb_lit->data.notebook.values.items[i]);
        fprintf(cg->out, ");\n");
        free(esc);
    }
}

static void gen_statement(CodeGen* cg, ASTNode* node) {
    switch (node->type) {
        case NODE_SAY:
            emit(cg, "ilma_say(");
            gen_expression(cg, node->data.say.expr);
            fprintf(cg->out, ");\n");
            break;

        case NODE_REMEMBER: {
            /* Skip lambda assignments — the function is already generated as a top-level recipe */
            if (node->data.remember.value && node->data.remember.value->type == NODE_RECIPE) break;
            char* safe = sanitize_ident(node->data.remember.name);
            emit(cg, "IlmaValue %s = ", safe);
            gen_expression(cg, node->data.remember.value);
            fprintf(cg->out, ";\n");
            /* Initialize bag elements if needed */
            if (node->data.remember.value->type == NODE_BAG_LIT) {
                gen_bag_init(cg, safe, node->data.remember.value);
            }
            /* Initialize notebook entries if needed */
            if (node->data.remember.value->type == NODE_NOTEBOOK_LIT) {
                gen_notebook_init(cg, safe, node->data.remember.value);
            }
            free(safe);
            break;
        }

        case NODE_ASSIGN: {
            ASTNode* target = node->data.assign.target;
            if (target->type == NODE_IDENTIFIER) {
                char* safe = sanitize_ident(target->data.ident_name);
                emit(cg, "%s = ", safe);
                gen_expression(cg, node->data.assign.value);
                fprintf(cg->out, ";\n");
                free(safe);
            } else if (target->type == NODE_MEMBER_ACCESS) {
                const char* member = target->data.member.member;
                ASTNode* obj = target->data.member.object;

                /* me.field = value inside blueprint */
                if (obj->type == NODE_IDENTIFIER &&
                    strcmp(obj->data.ident_name, "me") == 0 && cg->in_blueprint) {
                    emit(cg, "ilma_obj_set(me.as_object, \"%s\", ", member);
                    gen_expression(cg, node->data.assign.value);
                    fprintf(cg->out, ");\n");
                } else {
                    /* Generic object field set */
                    emit(cg, "ilma_obj_set(");
                    gen_expression(cg, obj);
                    fprintf(cg->out, ".as_object, \"%s\", ", member);
                    gen_expression(cg, node->data.assign.value);
                    fprintf(cg->out, ");\n");
                }
            } else {
                emit(cg, "/* unsupported assign target */ ");
                gen_expression(cg, node->data.assign.value);
                fprintf(cg->out, ";\n");
            }
            break;
        }

        case NODE_IF: {
            emit(cg, "if (ilma_is_truthy(");
            gen_expression(cg, node->data.if_stmt.condition);
            fprintf(cg->out, ")) {\n");
            cg->indent++;
            gen_block(cg, node->data.if_stmt.then_block);
            cg->indent--;
            if (node->data.if_stmt.else_block) {
                if (node->data.if_stmt.else_block->type == NODE_IF) {
                    emit(cg, "} else ");
                    /* Don't indent — the recursive call handles it */
                    /* Reset indent for the "else if" case */
                    int saved_indent = cg->indent;
                    cg->indent = 0;
                    gen_statement(cg, node->data.if_stmt.else_block);
                    cg->indent = saved_indent;
                } else {
                    emit(cg, "} else {\n");
                    cg->indent++;
                    gen_block(cg, node->data.if_stmt.else_block);
                    cg->indent--;
                    emit(cg, "}\n");
                }
            } else {
                emit(cg, "}\n");
            }
            break;
        }

        case NODE_REPEAT: {
            int tmp = cg->temp_counter++;
            emit(cg, "for (int64_t _i%d = 0; _i%d < ", tmp, tmp);
            fprintf(cg->out, "(");
            gen_expression(cg, node->data.repeat.count);
            fprintf(cg->out, ").as_whole; _i%d++) {\n", tmp);
            cg->indent++;
            gen_block(cg, node->data.repeat.body);
            cg->indent--;
            emit(cg, "}\n");
            break;
        }

        case NODE_RANGE_LOOP: {
            int tmp = cg->temp_counter++;
            char* safe_var = sanitize_ident(node->data.range_loop.var_name);
            emit(cg, "{\n");
            cg->indent++;
            emit(cg, "int64_t _rstart%d = (", tmp);
            gen_expression(cg, node->data.range_loop.start);
            fprintf(cg->out, ").as_whole;\n");
            emit(cg, "int64_t _rend%d = (", tmp);
            gen_expression(cg, node->data.range_loop.end);
            fprintf(cg->out, ").as_whole;\n");
            emit(cg, "for (int64_t _ri%d = _rstart%d; _ri%d <= _rend%d; _ri%d++) {\n",
                 tmp, tmp, tmp, tmp, tmp);
            cg->indent++;
            emit(cg, "IlmaValue %s = ilma_whole(_ri%d);\n", safe_var, tmp);
            gen_block(cg, node->data.range_loop.body);
            cg->indent--;
            emit(cg, "}\n");
            cg->indent--;
            emit(cg, "}\n");
            free(safe_var);
            break;
        }

        case NODE_WHILE: {
            emit(cg, "while (ilma_is_truthy(");
            gen_expression(cg, node->data.while_stmt.condition);
            fprintf(cg->out, ")) {\n");
            cg->indent++;
            gen_block(cg, node->data.while_stmt.body);
            cg->indent--;
            emit(cg, "}\n");
            break;
        }

        case NODE_FOR_EACH: {
            int tmp = cg->temp_counter++;
            char* safe_var = sanitize_ident(node->data.for_each.var_name);
            char* iter_name = malloc(strlen(safe_var) + 16);
            sprintf(iter_name, "_iter%d", tmp);

            emit(cg, "{\n");
            cg->indent++;
            emit(cg, "IlmaValue %s = ", iter_name);
            gen_expression(cg, node->data.for_each.iterable);
            fprintf(cg->out, ";\n");
            /* If notebook, iterate keys; otherwise iterate bag */
            emit(cg, "IlmaValue _iterbag%d = (%s.type == ILMA_NOTEBOOK) ? "
                 "ilma_notebook_keys(%s.as_notebook) : %s;\n",
                 tmp, iter_name, iter_name, iter_name);
            emit(cg, "for (int _i%d = 0; _i%d < ilma_bag_size(_iterbag%d.as_bag); _i%d++) {\n",
                 tmp, tmp, tmp, tmp);
            cg->indent++;
            emit(cg, "IlmaValue %s = ilma_bag_get(_iterbag%d.as_bag, _i%d);\n", safe_var, tmp, tmp);
            gen_block(cg, node->data.for_each.body);
            cg->indent--;
            emit(cg, "}\n");
            cg->indent--;
            emit(cg, "}\n");

            free(safe_var);
            free(iter_name);
            break;
        }

        case NODE_RECIPE: {
            /* Already emitted as top-level function — skip if we're in main */
            if (!cg->in_recipe) break;
            /* Nested recipes not supported at this level */
            break;
        }

        case NODE_GIVE_BACK:
            if (node->data.give_back.value) {
                emit(cg, "return ");
                gen_expression(cg, node->data.give_back.value);
                fprintf(cg->out, ";\n");
            } else {
                emit(cg, "return ilma_empty_val();\n");
            }
            break;

        case NODE_USE:
            emit(cg, "/* use %s — module loaded */\n", node->data.use.module_name);
            break;

        case NODE_SHOUT: {
            emit(cg, "ilma_shout(ilma_to_string(");
            gen_expression(cg, node->data.shout.message);
            fprintf(cg->out, "), %d);\n", node->line);
            break;
        }

        case NODE_TRY: {
            int tmp = cg->temp_counter++;
            emit(cg, "{\n");
            cg->indent++;
            emit(cg, "int _try_idx%d = ilma_error_stack.depth++;\n", tmp);
            emit(cg, "if (setjmp(ilma_error_stack.buf[_try_idx%d]) == 0) {\n", tmp);
            cg->indent++;
            gen_block(cg, node->data.try_stmt.try_block);
            emit(cg, "ilma_error_stack.depth--;\n");
            cg->indent--;
            emit(cg, "} else {\n");
            cg->indent++;
            emit(cg, "ilma_error_stack.depth--;\n");
            gen_block(cg, node->data.try_stmt.when_wrong_block);
            cg->indent--;
            emit(cg, "}\n");
            cg->indent--;
            emit(cg, "}\n");
            break;
        }

        case NODE_CHECK: {
            int tmp = cg->temp_counter++;
            emit(cg, "{\n");
            cg->indent++;
            emit(cg, "IlmaValue _check%d = ", tmp);
            gen_expression(cg, node->data.check_stmt.subject);
            fprintf(cg->out, ";\n");

            for (int i = 0; i < node->data.check_stmt.case_count; i++) {
                emit(cg, "%sif (", i > 0 ? "} else " : "");
                if (node->data.check_stmt.case_range_ends[i]) {
                    /* Range: when 90..99: -> if (val >= 90 && val <= 99) */
                    fprintf(cg->out, "ilma_is_truthy(ilma_geq(_check%d, ", tmp);
                    gen_expression(cg, node->data.check_stmt.case_exprs[i]);
                    fprintf(cg->out, ")) && ilma_is_truthy(ilma_leq(_check%d, ", tmp);
                    gen_expression(cg, node->data.check_stmt.case_range_ends[i]);
                    fprintf(cg->out, "))");
                } else {
                    /* Exact: when 100: -> if (val == 100) */
                    fprintf(cg->out, "ilma_is_truthy(ilma_eq(_check%d, ", tmp);
                    gen_expression(cg, node->data.check_stmt.case_exprs[i]);
                    fprintf(cg->out, "))");
                }
                fprintf(cg->out, ") {\n");
                cg->indent++;
                gen_block(cg, node->data.check_stmt.case_bodies[i]);
                cg->indent--;
            }

            if (node->data.check_stmt.otherwise_body) {
                if (node->data.check_stmt.case_count > 0) {
                    emit(cg, "} else {\n");
                } else {
                    emit(cg, "{\n");
                }
                cg->indent++;
                gen_block(cg, node->data.check_stmt.otherwise_body);
                cg->indent--;
            }

            if (node->data.check_stmt.case_count > 0 || node->data.check_stmt.otherwise_body) {
                emit(cg, "}\n");
            }
            cg->indent--;
            emit(cg, "}\n");
            break;
        }

        case NODE_EXPR_STMT:
            emit(cg, "");
            gen_expression(cg, node->data.say.expr);
            fprintf(cg->out, ";\n");
            break;

        case NODE_BLUEPRINT:
            /* Blueprints are generated as top-level functions, skip in main */
            break;

        case NODE_BLOCK:
            gen_block(cg, node);
            break;

        default:
            emit(cg, "/* unhandled statement type %d */\n", node->type);
            break;
    }
}

/* ── Top-level recipe generation ──────────────────────── */

static void gen_recipe_function(CodeGen* cg, ASTNode* node) {
    char* safe = sanitize_ident(node->data.recipe.name);

    fprintf(cg->out, "IlmaValue ilma_recipe_%s(", safe);
    for (int i = 0; i < node->data.recipe.param_count; i++) {
        if (i > 0) fprintf(cg->out, ", ");
        char* param_safe = sanitize_ident(node->data.recipe.params[i]);
        fprintf(cg->out, "IlmaValue %s", param_safe);
        free(param_safe);
    }
    fprintf(cg->out, ") {\n");
    cg->indent = 1;
    cg->in_recipe = 1;
    gen_block(cg, node->data.recipe.body);
    emit(cg, "return ilma_empty_val();\n");
    cg->in_recipe = 0;
    cg->indent = 0;
    fprintf(cg->out, "}\n\n");
    free(safe);
}

/* ── Blueprint generation ─────────────────────────────── */

static void gen_blueprint_method(CodeGen* cg, const char* bp_name, ASTNode* method_node) {
    char* safe_bp = sanitize_ident(bp_name);
    const char* method_name = method_node->data.recipe.name;

    if (strcmp(method_name, "create") == 0) {
        /* First generate the _init version (takes existing me) */
        fprintf(cg->out, "void ilma_bp_%s_create_init(IlmaValue me", safe_bp);
        for (int i = 0; i < method_node->data.recipe.param_count; i++) {
            fprintf(cg->out, ", ");
            char* psafe = sanitize_ident(method_node->data.recipe.params[i]);
            fprintf(cg->out, "IlmaValue %s", psafe);
            free(psafe);
        }
        fprintf(cg->out, ") {\n");
        cg->indent = 1;
        cg->in_blueprint = 1;
        cg->current_bp = bp_name;
        gen_block(cg, method_node->data.recipe.body);
        cg->in_blueprint = 0;
        cg->current_bp = NULL;
        cg->indent = 0;
        fprintf(cg->out, "}\n\n");

        /* Then generate the public constructor */
        fprintf(cg->out, "IlmaValue ilma_bp_%s_create(", safe_bp);
        for (int i = 0; i < method_node->data.recipe.param_count; i++) {
            if (i > 0) fprintf(cg->out, ", ");
            char* psafe = sanitize_ident(method_node->data.recipe.params[i]);
            fprintf(cg->out, "IlmaValue %s", psafe);
            free(psafe);
        }
        fprintf(cg->out, ") {\n");
        cg->indent = 1;
        emit(cg, "IlmaValue me = ilma_obj_new(\"%s\");\n", bp_name);
        emit(cg, "ilma_bp_%s_create_init(me", safe_bp);
        for (int i = 0; i < method_node->data.recipe.param_count; i++) {
            char* psafe = sanitize_ident(method_node->data.recipe.params[i]);
            fprintf(cg->out, ", %s", psafe);
            free(psafe);
        }
        fprintf(cg->out, ");\n");
        emit(cg, "return me;\n");
        cg->indent = 0;
        fprintf(cg->out, "}\n\n");
    } else {
        /* Regular method */
        fprintf(cg->out, "IlmaValue ilma_bp_%s_%s(IlmaValue me", safe_bp, method_name);
        for (int i = 0; i < method_node->data.recipe.param_count; i++) {
            fprintf(cg->out, ", ");
            char* psafe = sanitize_ident(method_node->data.recipe.params[i]);
            fprintf(cg->out, "IlmaValue %s", psafe);
            free(psafe);
        }
        fprintf(cg->out, ") {\n");
        cg->indent = 1;
        cg->in_blueprint = 1;
        cg->current_bp = bp_name;
        gen_block(cg, method_node->data.recipe.body);
        emit(cg, "return ilma_empty_val();\n");
        cg->in_blueprint = 0;
        cg->current_bp = NULL;
        cg->indent = 0;
        fprintf(cg->out, "}\n\n");
    }

    free(safe_bp);
}

static void gen_blueprint(CodeGen* cg, ASTNode* node) {
    const char* bp_name = node->data.blueprint.name;

    /* Generate each method */
    for (int i = 0; i < node->data.blueprint.members.count; i++) {
        ASTNode* member = node->data.blueprint.members.items[i];
        if (member->type == NODE_RECIPE) {
            gen_blueprint_method(cg, bp_name, member);
        }
    }
}

/* ── Entry point ──────────────────────────────────────── */

void codegen_init(CodeGen* cg, FILE* out) {
    cg->out = out;
    cg->indent = 0;
    cg->temp_counter = 0;
    cg->in_recipe = 0;
    cg->in_blueprint = 0;
    cg->current_bp = NULL;
    cg->used_module_count = 0;
}

void codegen_generate(CodeGen* cg, ASTNode* program) {
    /* Pre-scan for used modules */
    cg->used_module_count = 0;
    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* stmt = program->data.block.statements.items[i];
        if (stmt->type == NODE_USE && cg->used_module_count < 16) {
            cg->used_modules[cg->used_module_count++] = strdup(stmt->data.use.module_name);
        }
    }

    /* Preamble */
    fprintf(cg->out, "/* Generated by the ILMA compiler — ilma-lang.dev */\n");
    fprintf(cg->out, "#include \"ilma_runtime.h\"\n");
    fprintf(cg->out, "#include <string.h>\n");

    /* Emit module includes */
    for (int i = 0; i < cg->used_module_count; i++) {
        const char* mod = cg->used_modules[i];
        if (strcmp(mod, "finance") == 0)      fprintf(cg->out, "#include \"modules/finance.h\"\n");
        else if (strcmp(mod, "think") == 0)   fprintf(cg->out, "#include \"modules/think.h\"\n");
        else if (strcmp(mod, "body") == 0)    fprintf(cg->out, "#include \"modules/body.h\"\n");
        else if (strcmp(mod, "time") == 0)    fprintf(cg->out, "#include \"modules/time_mod.h\"\n");
        else if (strcmp(mod, "quran") == 0)   fprintf(cg->out, "#include \"modules/quran.h\"\n");
        else if (strcmp(mod, "draw") == 0)    fprintf(cg->out, "#include \"modules/draw.h\"\n");
        else if (strcmp(mod, "number") == 0)  fprintf(cg->out, "#include \"modules/number.h\"\n");
        else if (strcmp(mod, "science") == 0) fprintf(cg->out, "#include \"modules/science.h\"\n");
        else if (strcmp(mod, "trade") == 0)   fprintf(cg->out, "#include \"modules/trade.h\"\n");
    }
    fprintf(cg->out, "\n");

    /* Collect all recipes (including lambdas nested in expressions) */
    RecipeList recipes;
    ast_list_init(&recipes, 16);
    collect_all_recipes_recursive(program, &recipes);

    /* Collect top-level blueprints */
    ASTList blueprints;
    ast_list_init(&blueprints, 8);
    collect_blueprints(program, &blueprints);

    /* Register blueprint names and methods */
    known_blueprints = malloc(sizeof(char*) * (blueprints.count + 1));
    blueprint_parents = malloc(sizeof(char*) * (blueprints.count + 1));
    known_blueprint_count = 0;
    method_mapping_count = 0;

    /* First pass: register all blueprint names */
    for (int i = 0; i < blueprints.count; i++) {
        ASTNode* bp = blueprints.items[i];
        known_blueprints[known_blueprint_count] = bp->data.blueprint.name;
        blueprint_parents[known_blueprint_count] = bp->data.blueprint.parent;
        known_blueprint_count++;
    }

    /* Second pass: register methods (child methods override parent) */
    for (int i = 0; i < blueprints.count; i++) {
        ASTNode* bp = blueprints.items[i];

        /* If this blueprint has a parent, register parent's methods for this blueprint
           (so child objects can call parent methods) */
        if (bp->data.blueprint.parent) {
            /* Find the parent blueprint */
            for (int p = 0; p < blueprints.count; p++) {
                if (strcmp(blueprints.items[p]->data.blueprint.name, bp->data.blueprint.parent) == 0) {
                    ASTNode* parent_bp = blueprints.items[p];
                    for (int j = 0; j < parent_bp->data.blueprint.members.count; j++) {
                        ASTNode* member = parent_bp->data.blueprint.members.items[j];
                        if (member->type == NODE_RECIPE && strcmp(member->data.recipe.name, "create") != 0) {
                            /* Only register parent method if child doesn't override it */
                            int overridden = 0;
                            for (int k = 0; k < bp->data.blueprint.members.count; k++) {
                                ASTNode* child_member = bp->data.blueprint.members.items[k];
                                if (child_member->type == NODE_RECIPE &&
                                    strcmp(child_member->data.recipe.name, member->data.recipe.name) == 0) {
                                    overridden = 1;
                                    break;
                                }
                            }
                            if (!overridden) {
                                register_method(parent_bp->data.blueprint.name, member->data.recipe.name, member->data.recipe.param_count);
                            }
                        }
                    }
                    break;
                }
            }
        }

        /* Register own methods */
        for (int j = 0; j < bp->data.blueprint.members.count; j++) {
            ASTNode* member = bp->data.blueprint.members.items[j];
            if (member->type == NODE_RECIPE && strcmp(member->data.recipe.name, "create") != 0) {
                register_method(bp->data.blueprint.name, member->data.recipe.name, member->data.recipe.param_count);
            }
        }
    }

    /* Forward declarations — recipes */
    for (int i = 0; i < recipes.count; i++) {
        ASTNode* r = recipes.items[i];
        char* safe = sanitize_ident(r->data.recipe.name);
        fprintf(cg->out, "IlmaValue ilma_recipe_%s(", safe);
        for (int j = 0; j < r->data.recipe.param_count; j++) {
            if (j > 0) fprintf(cg->out, ", ");
            fprintf(cg->out, "IlmaValue");
        }
        fprintf(cg->out, ");\n");
        free(safe);
    }

    /* Forward declarations — blueprint methods */
    for (int i = 0; i < blueprints.count; i++) {
        ASTNode* bp = blueprints.items[i];
        char* safe_bp = sanitize_ident(bp->data.blueprint.name);
        for (int j = 0; j < bp->data.blueprint.members.count; j++) {
            ASTNode* member = bp->data.blueprint.members.items[j];
            if (member->type != NODE_RECIPE) continue;
            const char* mname = member->data.recipe.name;

            if (strcmp(mname, "create") == 0) {
                /* Forward declare both create and create_init */
                fprintf(cg->out, "void ilma_bp_%s_create_init(IlmaValue", safe_bp);
                for (int k = 0; k < member->data.recipe.param_count; k++) {
                    fprintf(cg->out, ", IlmaValue");
                }
                fprintf(cg->out, ");\n");
                fprintf(cg->out, "IlmaValue ilma_bp_%s_create(", safe_bp);
                for (int k = 0; k < member->data.recipe.param_count; k++) {
                    if (k > 0) fprintf(cg->out, ", ");
                    fprintf(cg->out, "IlmaValue");
                }
                fprintf(cg->out, ");\n");
            } else {
                fprintf(cg->out, "IlmaValue ilma_bp_%s_%s(IlmaValue", safe_bp, mname);
                for (int k = 0; k < member->data.recipe.param_count; k++) {
                    fprintf(cg->out, ", IlmaValue");
                }
                fprintf(cg->out, ");\n");
            }
        }
        free(safe_bp);
    }

    /* Forward declarations — dispatch functions for overridden methods */
    {
        char* declared[64];
        int declared_count = 0;
        for (int i = 0; i < method_mapping_count; i++) {
            const char* mname = method_mappings[i].method_name;
            if (!method_needs_dispatch(mname)) continue;
            int already = 0;
            for (int j = 0; j < declared_count; j++) {
                if (strcmp(declared[j], mname) == 0) { already = 1; break; }
            }
            if (already) continue;
            declared[declared_count++] = method_mappings[i].method_name;
            int param_count = method_mappings[i].param_count;
            fprintf(cg->out, "IlmaValue ilma_dispatch_%s(IlmaValue", mname);
            for (int p = 0; p < param_count; p++) {
                fprintf(cg->out, ", IlmaValue");
            }
            fprintf(cg->out, ");\n");
        }
    }

    if (recipes.count > 0 || blueprints.count > 0) fprintf(cg->out, "\n");

    /* Generate recipe functions */
    for (int i = 0; i < recipes.count; i++) {
        gen_recipe_function(cg, recipes.items[i]);
    }

    /* Generate blueprint functions */
    for (int i = 0; i < blueprints.count; i++) {
        gen_blueprint(cg, blueprints.items[i]);
    }

    /* Generate dispatch functions for overridden methods */
    {
        /* Find unique method names that need dispatch */
        char* dispatched[64];
        int dispatched_count = 0;

        for (int i = 0; i < method_mapping_count; i++) {
            const char* mname = method_mappings[i].method_name;
            if (!method_needs_dispatch(mname)) continue;

            /* Check if already generated */
            int already = 0;
            for (int j = 0; j < dispatched_count; j++) {
                if (strcmp(dispatched[j], mname) == 0) { already = 1; break; }
            }
            if (already) continue;
            dispatched[dispatched_count++] = method_mappings[i].method_name;

            /* Get all blueprints that define this method */
            const char* bps[32];
            int bp_count = get_method_blueprints(mname, bps, 32);
            int param_count = method_mappings[i].param_count;

            /* Generate dispatch function */
            fprintf(cg->out, "IlmaValue ilma_dispatch_%s(IlmaValue me", mname);
            for (int p = 0; p < param_count; p++) {
                fprintf(cg->out, ", IlmaValue _arg%d", p);
            }
            fprintf(cg->out, ") {\n");
            fprintf(cg->out, "    const char* _bp = ilma_obj_blueprint(me.as_object);\n");
            for (int b = 0; b < bp_count; b++) {
                char* safe_bp = sanitize_ident(bps[b]);
                fprintf(cg->out, "    %sif (strcmp(_bp, \"%s\") == 0) return ilma_bp_%s_%s(me",
                        b > 0 ? "else " : "", bps[b], safe_bp, mname);
                for (int p = 0; p < param_count; p++) {
                    fprintf(cg->out, ", _arg%d", p);
                }
                fprintf(cg->out, ");\n");
                free(safe_bp);
            }
            /* Fallback to first (parent) */
            if (bp_count > 0) {
                char* safe_bp = sanitize_ident(bps[0]);
                fprintf(cg->out, "    return ilma_bp_%s_%s(me", safe_bp, mname);
                for (int p = 0; p < param_count; p++) {
                    fprintf(cg->out, ", _arg%d", p);
                }
                fprintf(cg->out, ");\n");
                free(safe_bp);
            }
            fprintf(cg->out, "}\n\n");
        }
    }

    /* Generate main */
    fprintf(cg->out, "int main(void) {\n");
    cg->indent = 1;

    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* stmt = program->data.block.statements.items[i];
        if (stmt->type == NODE_RECIPE) continue;    /* already generated */
        if (stmt->type == NODE_BLUEPRINT) continue;  /* already generated */
        gen_statement(cg, stmt);
    }

    emit(cg, "return 0;\n");
    cg->indent = 0;
    fprintf(cg->out, "}\n");

    free(recipes.items);
    free(blueprints.items);
    free(known_blueprints);
    free(blueprint_parents);
    /* Note: used_modules are NOT freed here — main.c reads them after codegen */
    /* Clean up method mappings */
    for (int i = 0; i < method_mapping_count; i++) {
        free(method_mappings[i].method_name);
        free(method_mappings[i].blueprint_name);
    }
    free(method_mappings);
    method_mappings = NULL;
    method_mapping_count = 0;
    method_mapping_cap = 0;
    known_blueprints = NULL;
    known_blueprint_count = 0;
}
