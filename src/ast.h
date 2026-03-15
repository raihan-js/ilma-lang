#ifndef ILMA_AST_H
#define ILMA_AST_H

#include <stdint.h>

typedef enum {
    /* Expressions */
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT,
    NODE_BOOL_LIT,
    NODE_EMPTY_LIT,
    NODE_IDENTIFIER,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_CALL,
    NODE_MEMBER_ACCESS,
    NODE_INDEX_ACCESS,
    NODE_ASK_EXPR,
    NODE_BAG_LIT,
    NODE_NOTEBOOK_LIT,

    /* Statements */
    NODE_SAY,
    NODE_REMEMBER,
    NODE_ASSIGN,
    NODE_IF,
    NODE_REPEAT,
    NODE_WHILE,
    NODE_FOR_EACH,
    NODE_RECIPE,
    NODE_GIVE_BACK,
    NODE_BLUEPRINT,
    NODE_USE,
    NODE_SHOUT,
    NODE_TRY,
    NODE_EXPR_STMT,
    NODE_BLOCK,
    NODE_PROGRAM,
} NodeType;

typedef struct ASTNode ASTNode;

/* A dynamic list of child nodes */
typedef struct {
    ASTNode** items;
    int       count;
    int       capacity;
} NodeList;

struct ASTNode {
    NodeType type;
    int      line;

    union {
        /* NODE_INT_LIT */
        int64_t int_val;

        /* NODE_FLOAT_LIT */
        double float_val;

        /* NODE_STRING_LIT */
        char* string_val;

        /* NODE_BOOL_LIT */
        int bool_val;   /* 1 = yes, 0 = no */

        /* NODE_IDENTIFIER */
        char* ident_name;

        /* NODE_BINARY_OP */
        struct {
            char*    op;        /* "+", "-", "*", "/", "%", "is", "is not", ">", "<", ">=", "<=", "and", "or" */
            ASTNode* left;
            ASTNode* right;
        } binary;

        /* NODE_UNARY_OP */
        struct {
            char*    op;        /* "not", "-" */
            ASTNode* operand;
        } unary;

        /* NODE_CALL */
        struct {
            ASTNode* callee;    /* identifier or member_access */
            NodeList args;
        } call;

        /* NODE_MEMBER_ACCESS */
        struct {
            ASTNode* object;
            char*    member;
        } member;

        /* NODE_INDEX_ACCESS */
        struct {
            ASTNode* object;
            ASTNode* index;
        } index_access;

        /* NODE_ASK_EXPR */
        struct {
            ASTNode* prompt;    /* expression for the prompt string */
        } ask;

        /* NODE_BAG_LIT */
        struct {
            NodeList elements;
        } bag;

        /* NODE_NOTEBOOK_LIT */
        struct {
            char**   keys;
            NodeList values;
        } notebook;

        /* NODE_SAY */
        struct {
            ASTNode* expr;
        } say;

        /* NODE_REMEMBER */
        struct {
            char*    name;
            char*    type_annotation; /* NULL if inferred */
            ASTNode* value;
        } remember;

        /* NODE_ASSIGN */
        struct {
            ASTNode* target;    /* identifier or member_access */
            ASTNode* value;
        } assign;

        /* NODE_IF */
        struct {
            ASTNode* condition;
            ASTNode* then_block;
            ASTNode* else_block; /* NULL, or another NODE_IF for "otherwise if" */
        } if_stmt;

        /* NODE_REPEAT */
        struct {
            ASTNode* count;
            ASTNode* body;
        } repeat;

        /* NODE_WHILE */
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;

        /* NODE_FOR_EACH */
        struct {
            char*    var_name;
            ASTNode* iterable;
            ASTNode* body;
        } for_each;

        /* NODE_RECIPE */
        struct {
            char*    name;
            char**   params;
            int      param_count;
            ASTNode* body;
        } recipe;

        /* NODE_GIVE_BACK */
        struct {
            ASTNode* value;     /* NULL for bare give back */
        } give_back;

        /* NODE_BLUEPRINT */
        struct {
            char*    name;
            char*    parent;    /* NULL if no "comes from" */
            NodeList members;   /* recipes and create */
        } blueprint;

        /* NODE_USE */
        struct {
            char* module_name;
        } use;

        /* NODE_SHOUT */
        struct {
            ASTNode* message;
        } shout;

        /* NODE_TRY */
        struct {
            ASTNode* try_block;
            ASTNode* when_wrong_block;
        } try_stmt;

        /* NODE_BLOCK / NODE_PROGRAM */
        struct {
            NodeList statements;
        } block;
    } data;
};

ASTNode* ast_new(NodeType type, int line);
void     ast_free(ASTNode* node);

void node_list_init(NodeList* list);
void node_list_add(NodeList* list, ASTNode* node);

#endif /* ILMA_AST_H */
