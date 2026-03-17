#ifndef ILMA_TEST_RUNNER_H
#define ILMA_TEST_RUNNER_H

#include "ast.h"

/* Run all test blocks in the program.
 * Returns 0 if all tests pass, 1 if any fail. */
int ilma_run_tests(ASTNode* program, const char* filename);

#endif
