# Contributing to ILMA

Thank you for your interest in contributing. ILMA is a community project and every contribution matters — whether you are fixing a typo in an error message, adding a test case, or implementing a new standard library module.

---

## Before you start

1. **Check existing issues** — your idea or bug may already be tracked.
2. **For large changes**, open an issue first and describe what you intend to do. This prevents duplicate work and lets us discuss the approach before you invest time.
3. **For small fixes** (typos, single test cases, one-line bug fixes) — just open a pull request directly.

---

## Setting up

```bash
git clone https://github.com/raihan-js/ilma
cd ilma
make all        # build the compiler
make test       # all 17 tests must pass before you change anything
```

---

## Project structure

```
src/
  lexer.c/.h       — tokeniser (reads .ilma, produces tokens)
  parser.c/.h      — parser (tokens → AST)
  ast.c/.h         — AST node types and memory management
  codegen.c/.h     — code generator (AST → C source)
  main.c           — CLI entry point
  runtime/
    ilma_runtime.c — bags, notebooks, error handling, value types
    ilma_runtime.h — public API for generated code

tests/tier1/       — .ilma programs + .expected output files
examples/          — example ILMA programs
```

---

## Adding a test

The easiest contribution. Create two files in `tests/tier1/`:

```
tests/tier1/18_my_feature.ilma       # the ILMA program
tests/tier1/18_my_feature.expected   # exact expected stdout, no trailing newline issues
```

Generate the expected output:

```bash
ILMA_HOME=$(pwd)/src ./build/ilma --compile tests/tier1/18_my_feature.ilma
./tests/tier1/18_my_feature > tests/tier1/18_my_feature.expected
cat tests/tier1/18_my_feature.expected   # verify it looks right
rm tests/tier1/18_my_feature
```

Run `make test` to verify everything still passes. Then open a pull request.

---

## Adding a keyword

Changing or adding a keyword touches four files in this order:

1. **`src/lexer.h`** — add the `TOK_` constant to the enum
2. **`src/lexer.c`** — add the string match in `lex_identifier()` and the name in `token_type_name()`
3. **`src/parser.c`** — handle the new token in the appropriate `parse_*` function, produce an AST node
4. **`src/codegen.c`** — handle the new AST node in `gen_statement()` or `gen_expression()`

Always write a test before touching the compiler.

---

## Adding a standard library module

Standard library modules live in `src/runtime/`. Each module is one `.c` and one `.h` file:

```c
/* src/runtime/finance.h */
#ifndef ILMA_FINANCE_H
#define ILMA_FINANCE_H
#include "ilma_runtime.h"

IlmaValue ilma_finance_compound(IlmaValue principal, IlmaValue rate, IlmaValue years);
IlmaValue ilma_finance_zakat(IlmaValue wealth, IlmaValue nisab);

#endif
```

The module is registered in `codegen.c` in the `use` statement handler so the compiler knows to `#include` it and link it.

Add at least 3 test programs that use the module before submitting.

---

## Keyword naming philosophy

Every ILMA keyword must pass this test: **if a child who has never seen code before reads this line, do they immediately understand what it does?**

If the answer is not an immediate yes, the name is rejected. We do not borrow from other languages. We do not use abbreviations. Plain English only.

Examples of names that would be rejected: `def`, `fn`, `impl`, `mod`, `var`, `const`, `enum`, `struct`, `ptr`, `ref`.

---

## Code style

- C99, no C11 features
- No external libraries beyond the C standard library
- `#define _POSIX_C_SOURCE 200809L` at the top of any file using `strdup`, `readlink`, or other POSIX extensions
- All compiler warnings treated as issues to fix, not ignore
- Error messages must be in plain English, no error codes
- Functions under 60 lines where possible — split if longer

---

## Pull request checklist

- [ ] `make test` passes with 0 failures
- [ ] New features have at least one test in `tests/tier1/`
- [ ] Error messages are plain English, understandable by a 10-year-old
- [ ] No new warnings added (`-Wall -Wextra`)
- [ ] CHANGELOG.md updated under a new version heading if needed

---

## Good first issues

- Add a test for a missing edge case (nested recipes, empty bags, etc.)
- Improve an error message — make it friendlier and more specific
- Implement a `body` module function (BMI, sleep hours, water intake)
- Add a `finance` module function (profit margin, simple interest)
- Write an example program in `examples/`

---

## Code of conduct

Be kind. This project exists to help children learn. Every interaction in this community — issues, pull requests, code review — should reflect that intention.
