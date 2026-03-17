#include "test_runner.h"
#include "evaluator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_passed = 0;
static int g_failed = 0;

static int run_test_block(ASTNode* test_node, const char* filename) {
    (void)filename;
    Evaluator ev;
    evaluator_init(&ev);
    ev.repl_mode = 0;

    const char* label = test_node->data.test_stmt.name;
    (void)label;
    ASTNode* body = test_node->data.test_stmt.body;

    /* Run non-assert statements first (setup) */
    for (int i = 0; i < body->data.block.statements.count; i++) {
        ASTNode* stmt = body->data.block.statements.items[i];
        if (stmt->type == NODE_ASSERT) continue;
        evaluator_run_stmt(&ev, stmt);
    }

    /* Now run assert statements */
    int all_ok = 1;
    for (int i = 0; i < body->data.block.statements.count; i++) {
        ASTNode* stmt = body->data.block.statements.items[i];
        if (stmt->type != NODE_ASSERT) continue;

        IlmaValue result = evaluator_eval_expr_public(&ev, stmt->data.assert_stmt.expr);
        int ok = (result.type == ILMA_TRUTH && result.as_truth == 1);
        if (!ok) {
            printf("  FAIL  assert on line %d\n", stmt->line);
            all_ok = 0;
        }
    }

    evaluator_free(&ev);
    return all_ok;
}

int ilma_run_tests(ASTNode* program, const char* filename) {
    g_passed = 0;
    g_failed = 0;

    printf("Running tests in %s\n", filename);
    printf("\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n");

    for (int i = 0; i < program->data.block.statements.count; i++) {
        ASTNode* node = program->data.block.statements.items[i];
        if (node->type != NODE_TEST) continue;

        const char* label = node->data.test_stmt.name;
        int ok = run_test_block(node, filename);
        if (ok) {
            printf("  PASS  %s\n", label);
            g_passed++;
        } else {
            printf("  FAIL  %s\n", label);
            g_failed++;
        }
    }

    printf("\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n");
    printf("%d passed, %d failed\n", g_passed, g_failed);

    return g_failed > 0 ? 1 : 0;
}
