#ifndef ILMA_CHECKER_H
#define ILMA_CHECKER_H

#include "ast.h"

/* Run static analysis on the program.
 * Returns number of errors found (0 = clean). */
int ilma_check(ASTNode* program, const char* filename);

#endif
