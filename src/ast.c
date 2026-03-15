#include "ast.h"
#include <stdlib.h>
#include <string.h>

void node_list_init(NodeList* list) {
    list->count = 0;
    list->capacity = 4;
    list->items = malloc(sizeof(ASTNode*) * list->capacity);
}

void node_list_add(NodeList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(ASTNode*) * list->capacity);
    }
    list->items[list->count++] = node;
}

ASTNode* ast_new(NodeType type, int line) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    return node;
}

void ast_free(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_STRING_LIT:
            free(node->data.string_val);
            break;
        case NODE_IDENTIFIER:
            free(node->data.ident_name);
            break;
        case NODE_BINARY_OP:
            free(node->data.binary.op);
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case NODE_UNARY_OP:
            free(node->data.unary.op);
            ast_free(node->data.unary.operand);
            break;
        case NODE_CALL:
            ast_free(node->data.call.callee);
            for (int i = 0; i < node->data.call.args.count; i++)
                ast_free(node->data.call.args.items[i]);
            free(node->data.call.args.items);
            break;
        case NODE_MEMBER_ACCESS:
            ast_free(node->data.member.object);
            free(node->data.member.member);
            break;
        case NODE_INDEX_ACCESS:
            ast_free(node->data.index_access.object);
            ast_free(node->data.index_access.index);
            break;
        case NODE_ASK_EXPR:
            ast_free(node->data.ask.prompt);
            break;
        case NODE_BAG_LIT:
            for (int i = 0; i < node->data.bag.elements.count; i++)
                ast_free(node->data.bag.elements.items[i]);
            free(node->data.bag.elements.items);
            break;
        case NODE_NOTEBOOK_LIT:
            for (int i = 0; i < node->data.notebook.values.count; i++) {
                free(node->data.notebook.keys[i]);
                ast_free(node->data.notebook.values.items[i]);
            }
            free(node->data.notebook.keys);
            free(node->data.notebook.values.items);
            break;
        case NODE_STRING_INTERP:
            for (int i = 0; i < node->data.interp.parts.count; i++)
                ast_free(node->data.interp.parts.items[i]);
            free(node->data.interp.parts.items);
            break;
        case NODE_SAY:
            ast_free(node->data.say.expr);
            break;
        case NODE_REMEMBER:
            free(node->data.remember.name);
            free(node->data.remember.type_annotation);
            ast_free(node->data.remember.value);
            break;
        case NODE_ASSIGN:
            ast_free(node->data.assign.target);
            ast_free(node->data.assign.value);
            break;
        case NODE_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_block);
            ast_free(node->data.if_stmt.else_block);
            break;
        case NODE_REPEAT:
            ast_free(node->data.repeat.count);
            ast_free(node->data.repeat.body);
            break;
        case NODE_WHILE:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;
        case NODE_RANGE_LOOP:
            free(node->data.range_loop.var_name);
            ast_free(node->data.range_loop.start);
            ast_free(node->data.range_loop.end);
            ast_free(node->data.range_loop.body);
            break;
        case NODE_FOR_EACH:
            free(node->data.for_each.var_name);
            ast_free(node->data.for_each.iterable);
            ast_free(node->data.for_each.body);
            break;
        case NODE_RECIPE:
            free(node->data.recipe.name);
            for (int i = 0; i < node->data.recipe.param_count; i++)
                free(node->data.recipe.params[i]);
            free(node->data.recipe.params);
            ast_free(node->data.recipe.body);
            break;
        case NODE_GIVE_BACK:
            ast_free(node->data.give_back.value);
            break;
        case NODE_BLUEPRINT:
            free(node->data.blueprint.name);
            free(node->data.blueprint.parent);
            for (int i = 0; i < node->data.blueprint.members.count; i++)
                ast_free(node->data.blueprint.members.items[i]);
            free(node->data.blueprint.members.items);
            break;
        case NODE_USE:
            free(node->data.use.module_name);
            break;
        case NODE_SHOUT:
            ast_free(node->data.shout.message);
            break;
        case NODE_TRY:
            ast_free(node->data.try_stmt.try_block);
            ast_free(node->data.try_stmt.when_wrong_block);
            break;
        case NODE_EXPR_STMT:
            /* The expression is stored directly as a child — reuse say.expr */
            ast_free(node->data.say.expr);
            break;
        case NODE_BLOCK:
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.block.statements.count; i++)
                ast_free(node->data.block.statements.items[i]);
            free(node->data.block.statements.items);
            break;
        default:
            break;
    }
    free(node);
}
