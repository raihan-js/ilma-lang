#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ───────────────────────────────────────────── */

static Token* current(Parser* p) {
    return &p->tokens[p->pos];
}

static Token* peek_tok(Parser* p) {
    if (p->pos + 1 < p->count) return &p->tokens[p->pos + 1];
    return &p->tokens[p->count - 1];
}

static int at(Parser* p, TokenType type) {
    return current(p)->type == type;
}

static int at_end(Parser* p) {
    return current(p)->type == TOK_EOF;
}

static Token* advance_tok(Parser* p) {
    Token* t = current(p);
    if (!at_end(p)) p->pos++;
    return t;
}

static Token* expect(Parser* p, TokenType type) {
    if (!at(p, type)) {
        Token* t = current(p);
        fprintf(stderr, "Oops! On line %d, I expected %s but found %s",
                t->line, token_type_name(type), token_type_name(t->type));
        if (t->value) fprintf(stderr, " (\"%s\")", t->value);
        fprintf(stderr, ".\nCheck your code around line %d to make sure everything is in order.\n", t->line);
        exit(1);
    }
    return advance_tok(p);
}

static void skip_newlines(Parser* p) {
    while (at(p, TOK_NEWLINE)) advance_tok(p);
}

/* Forward declarations */
static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_block(Parser* p);
static ASTNode* parse_test(Parser* p);
static ASTNode* parse_assert(Parser* p);
static ASTNode* parse_run(Parser* p);
static ASTNode* parse_wait(Parser* p);

/* ── Expression parsing ───────────────────────────────── */

static int lambda_counter = 0;

static ASTNode* parse_primary(Parser* p) {
    Token* t = current(p);

    /* Lambda: recipe(params): body */
    if (at(p, TOK_RECIPE) && peek_tok(p)->type == TOK_LPAREN) {
        Token* lt = advance_tok(p); /* consume 'recipe' */
        expect(p, TOK_LPAREN);

        char lambda_name[64];
        snprintf(lambda_name, sizeof(lambda_name), "_lambda_%d", lambda_counter++);

        ASTNode* node = ast_new(NODE_RECIPE, lt->line);
        node->data.recipe.name = strdup(lambda_name);
        node->data.recipe.params = NULL;
        node->data.recipe.param_count = 0;

        int cap = 4;
        node->data.recipe.params = malloc(sizeof(char*) * cap);

        if (!at(p, TOK_RPAREN)) {
            Token* param = expect(p, TOK_IDENT);
            node->data.recipe.params[node->data.recipe.param_count++] = strdup(param->value);
            while (at(p, TOK_COMMA)) {
                advance_tok(p);
                if (node->data.recipe.param_count >= cap) {
                    cap *= 2;
                    node->data.recipe.params = realloc(node->data.recipe.params, sizeof(char*) * cap);
                }
                param = expect(p, TOK_IDENT);
                node->data.recipe.params[node->data.recipe.param_count++] = strdup(param->value);
            }
        }
        expect(p, TOK_RPAREN);
        expect(p, TOK_COLON);

        /* Check if body is inline (same line) or block (indented) */
        if (at(p, TOK_NEWLINE) || at(p, TOK_INDENT)) {
            /* Block body */
            node->data.recipe.body = parse_block(p);
        } else {
            /* Inline body: parse a single statement */
            ASTNode* body = ast_new(NODE_BLOCK, lt->line);
            node_list_init(&body->data.block.statements);
            node_list_add(&body->data.block.statements, parse_statement(p));
            node->data.recipe.body = body;
        }

        return node;
    }

    /* Integer literal */
    if (at(p, TOK_INT_LIT)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_INT_LIT, t->line);
        node->data.int_val = strtoll(t->value, NULL, 10);
        return node;
    }

    /* Float literal */
    if (at(p, TOK_FLOAT_LIT)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_FLOAT_LIT, t->line);
        node->data.float_val = strtod(t->value, NULL);
        return node;
    }

    /* String literal */
    if (at(p, TOK_STRING_LIT)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_STRING_LIT, t->line);
        node->data.string_val = strdup(t->value);
        return node;
    }

    /* String interpolation: "Hello {name}!" */
    if (at(p, TOK_STRING_INTERP)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_STRING_INTERP, t->line);
        node_list_init(&node->data.interp.parts);
        const char* raw = t->value;
        size_t len = strlen(raw);
        size_t i = 0;
        while (i < len) {
            /* Collect literal text until { or end */
            size_t start = i;
            while (i < len && raw[i] != '{') i++;
            if (i > start) {
                char* lit = malloc(i - start + 1);
                memcpy(lit, raw + start, i - start);
                lit[i - start] = '\0';
                ASTNode* lit_node = ast_new(NODE_STRING_LIT, t->line);
                lit_node->data.string_val = lit;
                node_list_add(&node->data.interp.parts, lit_node);
            }
            if (i < len && raw[i] == '{') {
                i++; /* skip { */
                size_t expr_start = i;
                int depth = 1;
                while (i < len && depth > 0) {
                    if (raw[i] == '{') depth++;
                    else if (raw[i] == '}') { depth--; if (depth == 0) break; }
                    i++;
                }
                size_t expr_len = i - expr_start;
                char* expr_text = malloc(expr_len + 1);
                memcpy(expr_text, raw + expr_start, expr_len);
                expr_text[expr_len] = '\0';
                /* Sub-lex and sub-parse the expression */
                Lexer sub_lexer;
                lexer_init(&sub_lexer, expr_text);
                lexer_tokenize(&sub_lexer);
                Parser sub_parser;
                parser_init(&sub_parser, sub_lexer.tokens, sub_lexer.token_count);
                ASTNode* expr = parse_expression(&sub_parser);
                node_list_add(&node->data.interp.parts, expr);
                lexer_free(&sub_lexer);
                free(expr_text);
                if (i < len) i++; /* skip } */
            }
        }
        return node;
    }

    /* Boolean literals */
    if (at(p, TOK_YES)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_BOOL_LIT, t->line);
        node->data.bool_val = 1;
        return node;
    }
    if (at(p, TOK_NO)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_BOOL_LIT, t->line);
        node->data.bool_val = 0;
        return node;
    }

    /* Empty literal */
    if (at(p, TOK_EMPTY)) {
        advance_tok(p);
        return ast_new(NODE_EMPTY_LIT, t->line);
    }

    /* ask expression */
    if (at(p, TOK_ASK)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_ASK_EXPR, t->line);
        node->data.ask.prompt = parse_expression(p);
        return node;
    }

    /* bag literal: bag[...] */
    if (at(p, TOK_BAG)) {
        advance_tok(p);
        expect(p, TOK_LBRACKET);
        ASTNode* node = ast_new(NODE_BAG_LIT, t->line);
        node_list_init(&node->data.bag.elements);
        if (!at(p, TOK_RBRACKET)) {
            node_list_add(&node->data.bag.elements, parse_expression(p));
            while (at(p, TOK_COMMA)) {
                advance_tok(p);
                node_list_add(&node->data.bag.elements, parse_expression(p));
            }
        }
        expect(p, TOK_RBRACKET);
        return node;
    }

    /* notebook literal: notebook[key: val, ...] */
    if (at(p, TOK_NOTEBOOK)) {
        advance_tok(p);
        expect(p, TOK_LBRACKET);
        ASTNode* node = ast_new(NODE_NOTEBOOK_LIT, t->line);
        int key_cap = 4;
        node->data.notebook.keys = malloc(sizeof(char*) * key_cap);
        node_list_init(&node->data.notebook.values);
        if (!at(p, TOK_RBRACKET)) {
            Token* key = expect(p, TOK_IDENT);
            expect(p, TOK_COLON);
            ASTNode* val = parse_expression(p);
            node->data.notebook.keys[node->data.notebook.values.count] = strdup(key->value);
            node_list_add(&node->data.notebook.values, val);
            while (at(p, TOK_COMMA)) {
                advance_tok(p);
                if (node->data.notebook.values.count >= key_cap) {
                    key_cap *= 2;
                    node->data.notebook.keys = realloc(node->data.notebook.keys, sizeof(char*) * key_cap);
                }
                key = expect(p, TOK_IDENT);
                expect(p, TOK_COLON);
                val = parse_expression(p);
                node->data.notebook.keys[node->data.notebook.values.count] = strdup(key->value);
                node_list_add(&node->data.notebook.values, val);
            }
        }
        expect(p, TOK_RBRACKET);
        return node;
    }

    /* me.field */
    if (at(p, TOK_ME)) {
        advance_tok(p);
        ASTNode* me_node = ast_new(NODE_IDENTIFIER, t->line);
        me_node->data.ident_name = strdup("me");
        /* Check for member access */
        while (at(p, TOK_DOT)) {
            advance_tok(p);
            Token* member = expect(p, TOK_IDENT);
            ASTNode* access = ast_new(NODE_MEMBER_ACCESS, t->line);
            access->data.member.object = me_node;
            access->data.member.member = strdup(member->value);
            me_node = access;

            /* Check for function call on member */
            if (at(p, TOK_LPAREN)) {
                advance_tok(p);
                ASTNode* call = ast_new(NODE_CALL, t->line);
                call->data.call.callee = me_node;
                node_list_init(&call->data.call.args);
                if (!at(p, TOK_RPAREN)) {
                    node_list_add(&call->data.call.args, parse_expression(p));
                    while (at(p, TOK_COMMA)) {
                        advance_tok(p);
                        node_list_add(&call->data.call.args, parse_expression(p));
                    }
                }
                expect(p, TOK_RPAREN);
                me_node = call;
            }
        }
        return me_node;
    }

    /* comes_from.method() */
    if (at(p, TOK_COMES_FROM)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_IDENTIFIER, t->line);
        node->data.ident_name = strdup("comes_from");
        while (at(p, TOK_DOT)) {
            advance_tok(p);
            /* After dot, accept IDENT or CREATE keyword (for comes_from.create()) */
            Token* member;
            if (at(p, TOK_CREATE)) {
                member = advance_tok(p);
            } else {
                member = expect(p, TOK_IDENT);
            }
            ASTNode* access = ast_new(NODE_MEMBER_ACCESS, t->line);
            access->data.member.object = node;
            access->data.member.member = strdup(member->value);
            node = access;
            if (at(p, TOK_LPAREN)) {
                advance_tok(p);
                ASTNode* call = ast_new(NODE_CALL, t->line);
                call->data.call.callee = node;
                node_list_init(&call->data.call.args);
                if (!at(p, TOK_RPAREN)) {
                    node_list_add(&call->data.call.args, parse_expression(p));
                    while (at(p, TOK_COMMA)) {
                        advance_tok(p);
                        node_list_add(&call->data.call.args, parse_expression(p));
                    }
                }
                expect(p, TOK_RPAREN);
                node = call;
            }
        }
        return node;
    }

    /* Identifier */
    if (at(p, TOK_IDENT)) {
        advance_tok(p);
        ASTNode* node = ast_new(NODE_IDENTIFIER, t->line);
        node->data.ident_name = strdup(t->value);
        return node;
    }

    /* Parenthesized expression */
    if (at(p, TOK_LPAREN)) {
        advance_tok(p);
        ASTNode* expr = parse_expression(p);
        expect(p, TOK_RPAREN);
        return expr;
    }

    fprintf(stderr, "Oops! On line %d, I found '%s' when I was expecting a value.\n"
                    "A value can be a number, some text in quotes, yes, no, or a name.\n",
                    t->line, t->value ? t->value : token_type_name(t->type));
    exit(1);
}

/* Parse postfix: member access, index, calls */
static ASTNode* parse_postfix(Parser* p) {
    ASTNode* node = parse_primary(p);

    for (;;) {
        if (at(p, TOK_DOT)) {
            advance_tok(p);
            /* Accept identifiers or keywords as member names (e.g., .each, .create) */
            Token* member;
            if (at(p, TOK_IDENT) || at(p, TOK_EACH) || at(p, TOK_CREATE) ||
                at(p, TOK_IN) || at(p, TOK_FROM) || at(p, TOK_NOT) ||
                at(p, TOK_CHECK) || at(p, TOK_WHEN) || at(p, TOK_OR) ||
                at(p, TOK_AND) || at(p, TOK_IS) || at(p, TOK_BAG) ||
                at(p, TOK_NOTEBOOK) || at(p, TOK_EMPTY)) {
                member = advance_tok(p);
            } else {
                member = expect(p, TOK_IDENT); /* fallback for error message */
            }
            ASTNode* access = ast_new(NODE_MEMBER_ACCESS, node->line);
            access->data.member.object = node;
            access->data.member.member = strdup(member->value);
            node = access;

            /* Method call */
            if (at(p, TOK_LPAREN)) {
                advance_tok(p);
                ASTNode* call = ast_new(NODE_CALL, node->line);
                call->data.call.callee = node;
                node_list_init(&call->data.call.args);
                if (!at(p, TOK_RPAREN)) {
                    node_list_add(&call->data.call.args, parse_expression(p));
                    while (at(p, TOK_COMMA)) {
                        advance_tok(p);
                        node_list_add(&call->data.call.args, parse_expression(p));
                    }
                }
                expect(p, TOK_RPAREN);
                node = call;
            }
        } else if (at(p, TOK_LBRACKET)) {
            advance_tok(p);
            ASTNode* idx_node;
            /* notebook[name] — bare identifier used as string key */
            if (at(p, TOK_IDENT) && peek_tok(p)->type == TOK_RBRACKET) {
                Token* key = advance_tok(p);
                idx_node = ast_new(NODE_STRING_LIT, key->line);
                idx_node->data.string_val = strdup(key->value);
            } else {
                idx_node = parse_expression(p);
            }
            expect(p, TOK_RBRACKET);
            ASTNode* access = ast_new(NODE_INDEX_ACCESS, node->line);
            access->data.index_access.object = node;
            access->data.index_access.index = idx_node;
            node = access;
        } else if (at(p, TOK_LPAREN)) {
            advance_tok(p);
            ASTNode* call = ast_new(NODE_CALL, node->line);
            call->data.call.callee = node;
            node_list_init(&call->data.call.args);
            if (!at(p, TOK_RPAREN)) {
                node_list_add(&call->data.call.args, parse_expression(p));
                while (at(p, TOK_COMMA)) {
                    advance_tok(p);
                    node_list_add(&call->data.call.args, parse_expression(p));
                }
            }
            expect(p, TOK_RPAREN);
            node = call;
        } else {
            break;
        }
    }

    return node;
}

/* Unary: not, - */
static ASTNode* parse_unary(Parser* p) {
    if (at(p, TOK_NOT)) {
        Token* t = advance_tok(p);
        ASTNode* node = ast_new(NODE_UNARY_OP, t->line);
        node->data.unary.op = strdup("not");
        node->data.unary.operand = parse_unary(p);
        return node;
    }
    if (at(p, TOK_MINUS)) {
        Token* t = advance_tok(p);
        ASTNode* node = ast_new(NODE_UNARY_OP, t->line);
        node->data.unary.op = strdup("-");
        node->data.unary.operand = parse_unary(p);
        return node;
    }
    return parse_postfix(p);
}

/* Multiplication, division, modulo */
static ASTNode* parse_factor(Parser* p) {
    ASTNode* left = parse_unary(p);
    while (at(p, TOK_STAR) || at(p, TOK_SLASH) || at(p, TOK_PERCENT)) {
        Token* op = advance_tok(p);
        const char* op_str = op->type == TOK_STAR ? "*" : (op->type == TOK_SLASH ? "/" : "%");
        ASTNode* right = parse_unary(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup(op_str);
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

/* Addition, subtraction */
static ASTNode* parse_term(Parser* p) {
    ASTNode* left = parse_factor(p);
    while (at(p, TOK_PLUS) || at(p, TOK_MINUS)) {
        Token* op = advance_tok(p);
        const char* op_str = op->type == TOK_PLUS ? "+" : "-";
        ASTNode* right = parse_factor(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup(op_str);
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

/* Comparison: <, >, <=, >= */
static ASTNode* parse_comparison(Parser* p) {
    ASTNode* left = parse_term(p);
    while (at(p, TOK_LT) || at(p, TOK_GT) || at(p, TOK_LEQ) || at(p, TOK_GEQ)) {
        Token* op = advance_tok(p);
        const char* op_str;
        switch (op->type) {
            case TOK_LT:  op_str = "<"; break;
            case TOK_GT:  op_str = ">"; break;
            case TOK_LEQ: op_str = "<="; break;
            case TOK_GEQ: op_str = ">="; break;
            default: op_str = "?"; break;
        }
        ASTNode* right = parse_term(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup(op_str);
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

/* Equality: is, is not */
static ASTNode* parse_equality(Parser* p) {
    ASTNode* left = parse_comparison(p);
    while (at(p, TOK_IS) || at(p, TOK_IS_NOT)) {
        Token* op = advance_tok(p);
        const char* op_str = op->type == TOK_IS ? "is" : "is not";
        ASTNode* right = parse_comparison(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup(op_str);
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

/* Logical AND */
static ASTNode* parse_and(Parser* p) {
    ASTNode* left = parse_equality(p);
    while (at(p, TOK_AND)) {
        Token* op = advance_tok(p);
        ASTNode* right = parse_equality(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup("and");
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

/* Logical OR */
static ASTNode* parse_or(Parser* p) {
    ASTNode* left = parse_and(p);
    while (at(p, TOK_OR)) {
        Token* op = advance_tok(p);
        ASTNode* right = parse_and(p);
        ASTNode* bin = ast_new(NODE_BINARY_OP, op->line);
        bin->data.binary.op = strdup("or");
        bin->data.binary.left = left;
        bin->data.binary.right = right;
        left = bin;
    }
    return left;
}

static ASTNode* parse_expression(Parser* p) {
    return parse_or(p);
}

/* ── Block parsing ────────────────────────────────────── */

static ASTNode* parse_block(Parser* p) {
    skip_newlines(p);
    expect(p, TOK_INDENT);
    ASTNode* block = ast_new(NODE_BLOCK, current(p)->line);
    node_list_init(&block->data.block.statements);

    while (!at(p, TOK_DEDENT) && !at_end(p)) {
        skip_newlines(p);
        if (at(p, TOK_DEDENT) || at_end(p)) break;
        node_list_add(&block->data.block.statements, parse_statement(p));
        skip_newlines(p);
    }

    if (at(p, TOK_DEDENT)) advance_tok(p);
    return block;
}

/* ── Statement parsing ────────────────────────────────── */

/* say <expr> */
static ASTNode* parse_say(Parser* p) {
    Token* t = advance_tok(p); /* consume 'say' */
    ASTNode* node = ast_new(NODE_SAY, t->line);
    node->data.say.expr = parse_expression(p);
    return node;
}

/* remember <name> = <expr>
   remember <name>: <type> = <expr> */
static ASTNode* parse_remember(Parser* p) {
    Token* t = advance_tok(p); /* consume 'remember' */
    Token* name = expect(p, TOK_IDENT);
    ASTNode* node = ast_new(NODE_REMEMBER, t->line);
    node->data.remember.name = strdup(name->value);
    node->data.remember.type_annotation = NULL;

    /* Optional type annotation */
    if (at(p, TOK_COLON) && peek_tok(p)->type != TOK_NEWLINE &&
        peek_tok(p)->type != TOK_INDENT && peek_tok(p)->type != TOK_EOF) {
        /* Check if next token after colon is a type keyword */
        Token* next = peek_tok(p);
        if (next->type >= TOK_TYPE_WHOLE && next->type <= TOK_TYPE_ANYTHING) {
            advance_tok(p); /* consume ':' */
            Token* type_tok = advance_tok(p);
            node->data.remember.type_annotation = strdup(type_tok->value);
        }
    }

    expect(p, TOK_ASSIGN);
    node->data.remember.value = parse_expression(p);

    /* If the value is a lambda, rename it to match the variable */
    if (node->data.remember.value && node->data.remember.value->type == NODE_RECIPE) {
        free(node->data.remember.value->data.recipe.name);
        node->data.remember.value->data.recipe.name = strdup(node->data.remember.name);
    }

    return node;
}

/* if <expr>:
       <block>
   otherwise if <expr>:
       <block>
   otherwise:
       <block> */
static ASTNode* parse_if(Parser* p) {
    Token* t = advance_tok(p); /* consume 'if' */
    ASTNode* node = ast_new(NODE_IF, t->line);
    node->data.if_stmt.condition = parse_expression(p);
    expect(p, TOK_COLON);
    node->data.if_stmt.then_block = parse_block(p);
    node->data.if_stmt.else_block = NULL;

    skip_newlines(p);

    if (at(p, TOK_OTHERWISE)) {
        advance_tok(p); /* consume 'otherwise' */
        if (at(p, TOK_IF)) {
            /* otherwise if — chain */
            node->data.if_stmt.else_block = parse_if(p);
        } else {
            /* otherwise: */
            expect(p, TOK_COLON);
            node->data.if_stmt.else_block = parse_block(p);
        }
    }
    return node;
}

/* repeat <expr>:
       <block>
   repeat <var> in <start>..<end>:
       <block> */
static ASTNode* parse_repeat(Parser* p) {
    Token* t = advance_tok(p); /* consume 'repeat' */

    /* Check for range loop: repeat <var> in <start>..<end>: */
    if (at(p, TOK_IDENT) && peek_tok(p)->type == TOK_IN) {
        Token* var = advance_tok(p); /* consume variable name */
        advance_tok(p); /* consume 'in' */
        ASTNode* start = parse_expression(p);
        expect(p, TOK_DOTDOT);
        ASTNode* end = parse_expression(p);
        expect(p, TOK_COLON);
        ASTNode* body = parse_block(p);
        ASTNode* node = ast_new(NODE_RANGE_LOOP, t->line);
        node->data.range_loop.var_name = strdup(var->value);
        node->data.range_loop.start = start;
        node->data.range_loop.end = end;
        node->data.range_loop.body = body;
        return node;
    }

    /* Regular repeat: repeat <count>: */
    ASTNode* node = ast_new(NODE_REPEAT, t->line);
    node->data.repeat.count = parse_expression(p);
    expect(p, TOK_COLON);
    node->data.repeat.body = parse_block(p);
    return node;
}

/* keep going while <expr>:
       <block> */
static ASTNode* parse_while(Parser* p) {
    Token* t = advance_tok(p); /* consume 'keep' */
    expect(p, TOK_GOING);
    expect(p, TOK_WHILE);
    ASTNode* node = ast_new(NODE_WHILE, t->line);
    node->data.while_stmt.condition = parse_expression(p);
    expect(p, TOK_COLON);
    node->data.while_stmt.body = parse_block(p);
    return node;
}

/* for each <var> in <expr>:
       <block> */
static ASTNode* parse_for_each(Parser* p) {
    Token* t = advance_tok(p); /* consume 'for' */
    expect(p, TOK_EACH);
    Token* var = expect(p, TOK_IDENT);
    expect(p, TOK_IN);
    ASTNode* node = ast_new(NODE_FOR_EACH, t->line);
    node->data.for_each.var_name = strdup(var->value);
    node->data.for_each.iterable = parse_expression(p);
    expect(p, TOK_COLON);
    node->data.for_each.body = parse_block(p);
    return node;
}

/* recipe <name>(<params>):
       <block> */
static ASTNode* parse_recipe(Parser* p) {
    Token* t = advance_tok(p); /* consume 'recipe' */
    Token* name = expect(p, TOK_IDENT);
    expect(p, TOK_LPAREN);

    ASTNode* node = ast_new(NODE_RECIPE, t->line);
    node->data.recipe.name = strdup(name->value);
    node->data.recipe.params = NULL;
    node->data.recipe.param_count = 0;

    int cap = 4;
    node->data.recipe.params = malloc(sizeof(char*) * cap);

    if (!at(p, TOK_RPAREN)) {
        Token* param = expect(p, TOK_IDENT);
        node->data.recipe.params[node->data.recipe.param_count++] = strdup(param->value);
        while (at(p, TOK_COMMA)) {
            advance_tok(p);
            if (node->data.recipe.param_count >= cap) {
                cap *= 2;
                node->data.recipe.params = realloc(node->data.recipe.params, sizeof(char*) * cap);
            }
            param = expect(p, TOK_IDENT);
            node->data.recipe.params[node->data.recipe.param_count++] = strdup(param->value);
        }
    }
    expect(p, TOK_RPAREN);
    expect(p, TOK_COLON);
    node->data.recipe.body = parse_block(p);
    return node;
}

/* give back <expr> */
static ASTNode* parse_give_back(Parser* p) {
    Token* t = advance_tok(p); /* consume 'give' */
    expect(p, TOK_BACK);
    ASTNode* node = ast_new(NODE_GIVE_BACK, t->line);
    if (!at(p, TOK_NEWLINE) && !at(p, TOK_DEDENT) && !at_end(p)) {
        node->data.give_back.value = parse_expression(p);
    } else {
        node->data.give_back.value = NULL;
    }
    return node;
}

/* blueprint <Name>:
       create(...): <block>
       recipe <name>(...): <block>
   blueprint <Name> comes from <Parent>:
       ... */
static ASTNode* parse_blueprint(Parser* p) {
    Token* t = advance_tok(p); /* consume 'blueprint' */
    Token* name = expect(p, TOK_IDENT);
    ASTNode* node = ast_new(NODE_BLUEPRINT, t->line);
    node->data.blueprint.name = strdup(name->value);
    node->data.blueprint.parent = NULL;
    node_list_init(&node->data.blueprint.members);

    /* "comes from" */
    if (at(p, TOK_COMES)) {
        advance_tok(p);
        expect(p, TOK_FROM);
        Token* parent = expect(p, TOK_IDENT);
        node->data.blueprint.parent = strdup(parent->value);
    }

    expect(p, TOK_COLON);
    skip_newlines(p);
    expect(p, TOK_INDENT);

    while (!at(p, TOK_DEDENT) && !at_end(p)) {
        skip_newlines(p);
        if (at(p, TOK_DEDENT) || at_end(p)) break;

        if (at(p, TOK_CREATE)) {
            /* Constructor: create(...): block */
            Token* ct = advance_tok(p);
            expect(p, TOK_LPAREN);
            ASTNode* ctor = ast_new(NODE_RECIPE, ct->line);
            ctor->data.recipe.name = strdup("create");
            int cap = 4;
            ctor->data.recipe.params = malloc(sizeof(char*) * cap);
            ctor->data.recipe.param_count = 0;
            if (!at(p, TOK_RPAREN)) {
                Token* param = expect(p, TOK_IDENT);
                ctor->data.recipe.params[ctor->data.recipe.param_count++] = strdup(param->value);
                while (at(p, TOK_COMMA)) {
                    advance_tok(p);
                    if (ctor->data.recipe.param_count >= cap) {
                        cap *= 2;
                        ctor->data.recipe.params = realloc(ctor->data.recipe.params, sizeof(char*) * cap);
                    }
                    param = expect(p, TOK_IDENT);
                    ctor->data.recipe.params[ctor->data.recipe.param_count++] = strdup(param->value);
                }
            }
            expect(p, TOK_RPAREN);
            expect(p, TOK_COLON);
            ctor->data.recipe.body = parse_block(p);
            node_list_add(&node->data.blueprint.members, ctor);
        } else if (at(p, TOK_RECIPE)) {
            node_list_add(&node->data.blueprint.members, parse_recipe(p));
        } else {
            fprintf(stderr, "Oops! Inside a blueprint on line %d, I expected 'create' or 'recipe' "
                            "but found '%s'.\n", current(p)->line,
                            current(p)->value ? current(p)->value : token_type_name(current(p)->type));
            exit(1);
        }
        skip_newlines(p);
    }

    if (at(p, TOK_DEDENT)) advance_tok(p);
    return node;
}

/* use <module> */
static ASTNode* parse_use(Parser* p) {
    Token* t = advance_tok(p); /* consume 'use' */
    Token* mod = expect(p, TOK_IDENT);
    ASTNode* node = ast_new(NODE_USE, t->line);
    node->data.use.module_name = strdup(mod->value);
    return node;
}

/* shout <expr> */
static ASTNode* parse_shout(Parser* p) {
    Token* t = advance_tok(p); /* consume 'shout' */
    ASTNode* node = ast_new(NODE_SHOUT, t->line);
    node->data.shout.message = parse_expression(p);
    return node;
}

/* try:
       <block>
   when wrong:
       <block> */
static ASTNode* parse_try(Parser* p) {
    Token* t = advance_tok(p); /* consume 'try' */
    expect(p, TOK_COLON);
    ASTNode* node = ast_new(NODE_TRY, t->line);
    node->data.try_stmt.try_block = parse_block(p);

    skip_newlines(p);
    expect(p, TOK_WHEN);
    expect(p, TOK_WRONG);
    expect(p, TOK_COLON);
    node->data.try_stmt.when_wrong_block = parse_block(p);
    return node;
}

/* check <expr>:
       when <pattern>:
           <block>
       when <lo>..<hi>:
           <block>
       when <a> or <b>:
           <block>
       otherwise:
           <block> */
static ASTNode* parse_check(Parser* p) {
    Token* t = advance_tok(p); /* consume 'check' */
    ASTNode* node = ast_new(NODE_CHECK, t->line);
    node->data.check_stmt.subject = parse_expression(p);
    expect(p, TOK_COLON);

    /* Initialize arrays */
    int cap = 8;
    node->data.check_stmt.case_exprs = malloc(sizeof(ASTNode*) * cap);
    node->data.check_stmt.case_range_ends = malloc(sizeof(ASTNode*) * cap);
    node->data.check_stmt.case_bodies = malloc(sizeof(ASTNode*) * cap);
    node->data.check_stmt.case_count = 0;
    node->data.check_stmt.case_capacity = cap;
    node->data.check_stmt.otherwise_body = NULL;

    skip_newlines(p);
    expect(p, TOK_INDENT);

    while (!at(p, TOK_DEDENT) && !at_end(p)) {
        skip_newlines(p);
        if (at(p, TOK_DEDENT) || at_end(p)) break;

        if (at(p, TOK_OTHERWISE)) {
            advance_tok(p);
            expect(p, TOK_COLON);
            node->data.check_stmt.otherwise_body = parse_block(p);
        } else if (at(p, TOK_WHEN)) {
            advance_tok(p);

            /* Parse patterns (possibly with 'or' for multiple patterns)
               Use parse_and() instead of parse_expression() so that 'or'
               is reserved as a pattern separator, not a logical operator. */
            ASTNode* patterns[16];
            ASTNode* range_ends[16];
            int pattern_count = 0;

            ASTNode* pat = parse_and(p);
            ASTNode* rend = NULL;
            if (at(p, TOK_DOTDOT)) {
                advance_tok(p);
                rend = parse_expression(p);
            }
            patterns[pattern_count] = pat;
            range_ends[pattern_count] = rend;
            pattern_count++;

            while (at(p, TOK_OR)) {
                advance_tok(p);
                pat = parse_and(p);
                rend = NULL;
                if (at(p, TOK_DOTDOT)) {
                    advance_tok(p);
                    rend = parse_expression(p);
                }
                patterns[pattern_count] = pat;
                range_ends[pattern_count] = rend;
                pattern_count++;
            }

            expect(p, TOK_COLON);
            ASTNode* body = parse_block(p);

            /* Add all patterns as separate cases sharing the same body */
            for (int i = 0; i < pattern_count; i++) {
                /* Grow arrays if needed */
                if (node->data.check_stmt.case_count >= node->data.check_stmt.case_capacity) {
                    node->data.check_stmt.case_capacity *= 2;
                    node->data.check_stmt.case_exprs = realloc(node->data.check_stmt.case_exprs,
                        sizeof(ASTNode*) * node->data.check_stmt.case_capacity);
                    node->data.check_stmt.case_range_ends = realloc(node->data.check_stmt.case_range_ends,
                        sizeof(ASTNode*) * node->data.check_stmt.case_capacity);
                    node->data.check_stmt.case_bodies = realloc(node->data.check_stmt.case_bodies,
                        sizeof(ASTNode*) * node->data.check_stmt.case_capacity);
                }
                int idx = node->data.check_stmt.case_count++;
                node->data.check_stmt.case_exprs[idx] = patterns[i];
                node->data.check_stmt.case_range_ends[idx] = range_ends[i];
                node->data.check_stmt.case_bodies[idx] = body;
            }
        } else {
            fprintf(stderr, "Oops! Inside check on line %d, expected 'when' or 'otherwise'\n",
                    current(p)->line);
            exit(1);
        }
        skip_newlines(p);
    }
    if (at(p, TOK_DEDENT)) advance_tok(p);
    return node;
}

/* test "label":
       assert <expr>
       ... */
static ASTNode* parse_test(Parser* p) {
    Token* t = advance_tok(p); /* consume 'test' */
    ASTNode* node = ast_new(NODE_TEST, t->line);
    /* parse label string */
    Token* label = expect(p, TOK_STRING_LIT);
    node->data.test_stmt.name = strdup(label->value);
    expect(p, TOK_COLON);
    node->data.test_stmt.body = parse_block(p);
    return node;
}

/* assert <expr> */
static ASTNode* parse_assert(Parser* p) {
    Token* t = advance_tok(p); /* consume 'assert' */
    ASTNode* node = ast_new(NODE_ASSERT, t->line);
    node->data.assert_stmt.expr = parse_expression(p);
    node->data.assert_stmt.label = NULL;
    return node;
}

/* run <name> = <call_expr> */
static ASTNode* parse_run(Parser* p) {
    Token* t = advance_tok(p); /* consume 'run' */
    ASTNode* node = ast_new(NODE_RUN_STMT, t->line);
    Token* name = expect(p, TOK_IDENT);
    node->data.run_stmt.task_name = strdup(name->value);
    expect(p, TOK_ASSIGN);
    node->data.run_stmt.call = parse_expression(p);
    return node;
}

/* wait <name> */
static ASTNode* parse_wait(Parser* p) {
    Token* t = advance_tok(p); /* consume 'wait' */
    ASTNode* node = ast_new(NODE_WAIT_STMT, t->line);
    Token* name = expect(p, TOK_IDENT);
    node->data.wait_stmt.task_name = strdup(name->value);
    return node;
}

/* General statement dispatcher */
static ASTNode* parse_statement(Parser* p) {
    skip_newlines(p);

    switch (current(p)->type) {
        case TOK_SAY:       return parse_say(p);
        case TOK_REMEMBER:  return parse_remember(p);
        case TOK_IF:        return parse_if(p);
        case TOK_REPEAT:    return parse_repeat(p);
        case TOK_KEEP:      return parse_while(p);
        case TOK_FOR:       return parse_for_each(p);
        case TOK_RECIPE:    return parse_recipe(p);
        case TOK_GIVE:      return parse_give_back(p);
        case TOK_BLUEPRINT: return parse_blueprint(p);
        case TOK_USE:       return parse_use(p);
        case TOK_SHOUT:     return parse_shout(p);
        case TOK_TRY:       return parse_try(p);
        case TOK_CHECK:     return parse_check(p);
        case TOK_TEST:      return parse_test(p);
        case TOK_ASSERT:    return parse_assert(p);
        case TOK_RUN:       return parse_run(p);
        case TOK_WAIT:      return parse_wait(p);
        default: {
            /* Expression statement (assignment or function call) */
            ASTNode* expr = parse_expression(p);

            /* Check for assignment: target = value */
            if (at(p, TOK_ASSIGN)) {
                advance_tok(p); /* consume '=' */
                ASTNode* assign = ast_new(NODE_ASSIGN, expr->line);
                assign->data.assign.target = expr;
                assign->data.assign.value = parse_expression(p);
                return assign;
            }

            /* Bare expression statement (e.g., function call) */
            ASTNode* stmt = ast_new(NODE_EXPR_STMT, expr->line);
            stmt->data.say.expr = expr; /* reuse say.expr field for the expression */
            return stmt;
        }
    }
}

/* ── Entry point ──────────────────────────────────────── */

void parser_init(Parser* p, Token* tokens, int count) {
    p->tokens = tokens;
    p->count = count;
    p->pos = 0;
}

ASTNode* parser_parse(Parser* p) {
    ASTNode* program = ast_new(NODE_PROGRAM, 1);
    node_list_init(&program->data.block.statements);

    skip_newlines(p);
    while (!at_end(p)) {
        node_list_add(&program->data.block.statements, parse_statement(p));
        skip_newlines(p);
    }

    return program;
}
