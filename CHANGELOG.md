# Changelog

All notable changes to ILMA are documented here.

---

## [0.7.0] — 2026-03-17

### Added

**Package Ecosystem**
- 10 community packages: math, strings, colors, datetime, random, validate, format, storage, test_helpers, crypto
- Package registry moved to `https://ilma-lang.dev/packages/registry.json`
- Registry supports `file://` URLs for local testing
- Package install now downloads individual `.ilma` files (no tarball)
- `ilma packages --available` now shows version and description columns
- `scripts/build-registry.py` — regenerates `packages/registry.json` from local packages directory
- Package deploy: GitHub Actions copies packages to website on push

**New Built-in Functions**
- `timestamp()` — returns Unix timestamp as a whole number
- `env("VAR")` — reads an environment variable, returns empty if not set
- `print(value)` — outputs a value without a newline
- `exit(code)` — exits the program with a status code
- `sleep(ms)` — sleeps for N milliseconds
- `args()` — returns command-line arguments as a bag

**Homebrew Formula**
- `Formula/ilma.rb` — ready-to-submit Homebrew formula
- `scripts/create-homebrew-tap.sh` — computes SHA256 and guides tap setup
- Install on macOS: `brew tap raihan-js/ilma && brew install ilma`

**Benchmark Tests**
- `tests/bench/fibonacci.ilma` — fib(30) benchmark
- `tests/bench/string_concat.ilma` — 1000x string concatenation
- `tests/bench/bag_operations.ilma` — 1000x bag add
- `make bench` target runs all benchmarks with timing

**Website**
- `website/packages.html` — package directory with search, live registry loading
- `website/blog/` — blog section with first post
- Navigation updated with Packages and Blog links
- Version badge (v0.7.0) in hero section

**VS Code Extension**
- Added configuration contribution: `ilma.executablePath`, `ilma.lspPath`, `ilma.formatOnSave`
- Gallery banner updated to ILMA green (#2d6a4f)
- `PUBLISH.md` instructions for VS Code Marketplace
- `scripts/create-icon-simple.py` — generates 128x128 PNG icon

**Release Workflow**
- `pre-release` job: verifies VERSION matches git tag and runs full test suite
- All build jobs now `needs: [pre-release]`
- Homebrew install instructions in release notes
- `packages.tar.gz` artifact attached to each release

---

## [0.6.0] — 2026-03-17

### Added

**Built-in Test Runner (`ilma test`)**
- `test "label":` block defines a named test
- `assert <expr>` verifies a boolean condition
- `ilma test <file.ilma>` runs all test blocks and reports pass/fail
- Tests are skipped in normal compilation (codegen ignores test/assert nodes)

**Code Formatter (`ilma fmt`)**
- `ilma fmt <file.ilma>` reformats source in-place with canonical style
- `ilma fmt <file.ilma> --check` checks if file needs reformatting (CI mode)
- 4-space indentation, consistent keyword and operator spacing

**Static Checker (`ilma check`)**
- `ilma check <file.ilma>` performs static analysis without running
- Detects: undefined variables, wrong argument counts, unreachable code after `give back`, `me` outside blueprints

**Documentation Generator (`ilma doc`)**
- `ilma doc <file.ilma> [output.html]` generates HTML API docs
- `###` doc comments above `recipe`/`blueprint` definitions are extracted
- Clean HTML output with styled recipe and blueprint entries

**Concurrency (`run`/`wait`)**
- `run task1 = myrecipe()` spawns a concurrent task using pthreads
- `wait task1` blocks until the task completes
- Codegen emits pthread thread wrapper functions automatically

**Gradual Typing**
- Type annotations on `remember` declarations are now enforced at runtime (evaluator mode)
- `remember x: whole = 42` raises a type error if the value doesn't match
- Supported types: `whole`, `decimal`, `text`, `truth`, `anything`

**JSON and HTTP Modules**
- `use json` and `use http` are now wired into the codegen module system

---

## [0.5.0] — 2026-03-17

### Added

**Interactive REPL**
- `ilma` with no arguments starts an interactive prompt with box-character banner
- Persistent state across inputs (variables, recipes, blueprints survive between lines)
- Multi-line input: lines ending with `:` open continuation prompt `...>`
- Expression auto-print: bare expressions like `2 + 2` print their result automatically
- readline support (arrow-key navigation, history) when libreadline is available
- Special commands: quit, exit, help, clear, reset, history (ring buffer of 10)
- Ctrl+C (SIGINT) handled gracefully — prompt shown again instead of crashing
- Ctrl+D (EOF) prints "Goodbye!" and exits cleanly

**Rust-style Error Messages**
- Error format with `-->  filename:line:col` pointer and `^` caret indicator
- Colour output (bold red error, cyan location) when stdout is a terminal
- 12 common error types with contextual hints and code examples
- `--strict` flag enables unused-variable and unreachable-code warnings

**Language Specification**
- SPEC.md — 1622-line formal language specification
- Complete EBNF grammar for the entire language
- Full keyword reference, type system, module API, and error codes

**Website — Learning Platform**
- website/learn.html: 30-lesson structured learning index
- Lessons organised into three tiers: Seed (01–10), Sapling (11–20), Tree (21–30)
- localStorage progress tracking with animated progress bar
- Visited-lesson checkmarks persist across browser sessions
- "Start Learning →" CTA on homepage linking to learn.html

**WASM Pipeline**
- Updated build-wasm.sh: all 9 modules included, exports `ilma_eval_wasm`
- Updated wasm-compiler.js: `IlmaWasm` object with async init/run API
- Playground falls back to JS interpreter when WASM is unavailable

**Windows Support**
- PowerShell installer: `irm https://ilma-lang.dev/install.ps1 | iex`
- Binary download with automatic PATH setup and ILMA_HOME env var
- install.html Windows tab updated to show PowerShell one-liner

**Playground Improvements**
- check/when pattern matching (exact, range, or-patterns)
- Lambda expressions: `recipe(x): expression`
- bag.map(), bag.filter(), bag.each() higher-order functions
- science module: gravity, celsius_to_fahrenheit, kinetic_energy, speed
- trade module: profit, margin, discount, vat, halal_check
- try/when wrong, shout, keep going while, notebook literals

**Tests**
- 3 new tests added (68 total: 24 Tier 1, 24 Tier 2, 20 Tier 3), all passing
- New tests: 24_while_loop (Tier 1), 24_for_each_notebook (Tier 2), 20_closure (Tier 3)

---

## [0.4.0] — 2026

### Added

**Language Features**
- Pattern matching: `check`/`when`/`otherwise` syntax with exact values, ranges (`90..99`), and or-patterns (`"cat" or "dog"`)
- Lambda expressions: `recipe(n): give back n * 2` — anonymous recipes as expressions
- Higher-order bag methods: `bag.map()`, `bag.filter()`, `bag.each()` with lambda arguments
- Multi-line strings with triple quotes: `"""multi\nline"""`
- Raw strings with backticks: `` `no {interpolation}` ``

**Standard Library Modules (C Compiler)**
- `science` — gravity, kinetic/potential energy, speed, temperature conversions (C/F/K), Ohm's law, light-years, atom count
- `trade` — profit, margin, markup, break-even, supply/demand pricing, VAT, discount, halal trade checker

**Package Manager**
- `ilma get <package>` — install packages from the ILMA package registry
- `ilma packages` — list installed packages
- `ilma packages --available` — list available packages
- Package registry with math, strings, and colors starter packages

**IlmaWeb Framework**
- Todo app example with 5 routes
- SQLite database integration (optional, compile with `-DILMA_HAS_SQLITE`)

**Tests**
- 7 new tests: check/when (3), multiline strings, lambdas, science module, trade module
- Total: 57 compiler tests (20 Tier 1, 20 Tier 2, 17 Tier 3), all passing

---

## [0.3.0] — 2026

### Added

**Language Features**
- File I/O built-in functions: `read_file()`, `write_file()`, `file_exists()`

**Standard Library Modules (C Compiler)**
- `draw` — SVG canvas, circle, rect, line, text, polygon, Islamic star patterns, save to file
- `number` — sqrt, abs, floor, ceil, round, power, random, is_prime, fibonacci, to_binary, to_hex, pi, e

**Tooling**
- AST tree-walking evaluator (`src/evaluator.c`) for direct interpretation (WASM-ready)
- WASM build script (`scripts/build-wasm.sh`) for compiling ILMA to WebAssembly
- WASM browser bridge (`website/js/wasm-compiler.js`) with JS interpreter fallback
- LSP server foundation (`src/lsp/ilma_lsp.c`) with diagnostics and hover documentation
- `ilma-lsp` binary for VS Code and other LSP-compatible editors

**VS Code Extension**
- Marketplace-ready metadata (galleryBanner, bugs, keywords)
- Extension CHANGELOG
- Publish script (`scripts/publish-extension.sh`)
- Icon generation script (`scripts/generate-icon.sh`)

**Website**
- GitHub Pages deployment workflow (`.github/workflows/deploy-website.yml`)
- CNAME file for ilma-lang.dev custom domain
- 404 page with ILMA-themed design
- Open Graph and Twitter meta tags for social sharing
- Dedicated playground page (`/playground`) with 8 example programs, share/download buttons
- Documentation for draw module, number module, and file I/O built-ins

**Tests**
- 3 new tests: draw module, number module, file I/O
- 2 LSP tests: diagnostics and hover
- Total: 50 compiler tests (20 Tier 1, 16 Tier 2, 14 Tier 3), all passing

---

## [0.2.0] — 2026

### Added

**Language Features**
- String interpolation: `say "Hello {name}, you are {age} years old."`
- Range loops: `repeat i in 1..10:` — iterate over numeric ranges
- `..` (dotdot) operator in the lexer for range expressions

**Standard Library Modules (C Compiler)**
- `finance` — `compound()`, `zakat()`, `profit()`, `margin()`, `simple_interest()`
- `time` — `today_str()`, `to_hijri()` (Gregorian→Hijri conversion), `days_between()`
- `body` — `bmi()`, `bmi_category()`, `daily_water()`, `sleep_advice()`
- `think` — `stoic_question()` (12 rotating questions), `pros_cons_new()`, `weigh_result()`
- `quran` — `ayah_of_the_day()` (30 embedded ayat), `search()`, `surah()`

**Module System**
- `use module` now emits proper `#include` directives and links module source files
- Module method calls (`finance.zakat(...)`) compile to `ilma_finance_zakat(...)` C calls
- Module source files auto-detected and passed to GCC at compile time
- Install target copies module files to system library path

**Ecosystem**
- VS Code extension with TextMate grammar, 10 snippets, and run command
- ilma-lang.dev website with playground, install page, and language reference
- IlmaWeb framework for building web servers in ILMA
- ROADMAP.md with versioned milestones

**Tests**
- 6 new tests: string interpolation, range loops, finance, body, think, quran modules
- Total: 47 compiler tests (20 Tier 1, 15 Tier 2, 12 Tier 3), all passing

---

## [0.1.0] — 2025

### Added

**Core Language — Tier 1 (Seed)**
- `say` — print to screen
- `remember x = value` — variables
- `ask "question"` — user input
- `repeat N:` — counted loops
- `if` / `otherwise` / `otherwise if` — conditionals
- `yes` / `no` / `empty` — boolean and null literals
- `and`, `or`, `not` — logical operators
- `is` / `is not` — equality
- `<`, `>`, `<=`, `>=` — comparison
- `+`, `-`, `*`, `/`, `%` — arithmetic
- String concatenation with `+`
- Comments with `#`
- Unicode identifiers (Arabic/Urdu variable names)

**Core Language — Tier 2 (Sapling)**
- `recipe name(params):` / `give back` — functions with return values
- `bag[...]` — ordered lists with `.add()`, `.remove()`, `.size`, `.sorted()`
- `notebook[key: val]` — key-value dictionaries with iteration
- `for each item in collection:` — collection loops
- `keep going while condition:` — while loops
- Text methods: `.upper()`, `.lower()`, `.contains()`, `.slice()`, `.length`, `.join()`
- `try:` / `when wrong:` / `shout` — error handling (setjmp/longjmp)
- Variable reassignment without `remember`

**Core Language — Tier 3 (Tree)**
- `blueprint Name:` — classes with `create()` constructor
- `me.field` — object field access
- `comes from` — inheritance
- `comes_from.create()` — parent constructor delegation
- Method override with runtime dynamic dispatch

**Standard Library Modules (JS interpreter)**
- `finance` — compound interest, zakat, profit, margin, budget (50/30/20)
- `time` — today, Hijri calendar conversion, days_between
- `body` — BMI, BMI category, daily water intake
- `think` — Stoic reflection questions
- `quran` — surah lookup, search translations, ayah of the day
- `trade` — profit, margin, halal trade checker, supply/demand
- `number` — binary, hex, Roman numerals, primes, Fibonacci
- `science` — gravity, temperature, kinetic energy, distance
- `draw` — SVG canvas with circle, square, line, text, Islamic star patterns

**C Compiler**
- Full ILMA → C transpilation (clean, readable output)
- 41 test programs across all three tiers (20 Tier 1, 13 Tier 2, 8 Tier 3)
- `ilma file.ilma` — compile and run
- `ilma --compile` — compile to binary
- `ilma --c` — show generated C
- `ilma --tokens` — show token stream
- `ilma --version` — version info
- Absolute path support (fixed)
- Runtime path auto-detection (ILMA_HOME, binary-relative, fallbacks)

**Web Platform**
- Monaco editor with ILMA syntax highlighting and dark theme
- In-browser JavaScript interpreter (Tier 1 + 2 + blueprints)
- 30 structured lessons covering the complete Tier 2 curriculum
- Socratic tutor (rule-based hint engine, never gives answers)
- Teacher dashboard with student management and CSV export
- Program sharing via URL (base64 encoded)
- PWA with service worker for offline support
- Canvas graphics rendering (SVG in output)
- Monaco autocompletion with snippets for all keywords and modules

**Infrastructure**
- Docker deployment (nginx, multi-stage build)
- `make install` / `make uninstall`
- `install.sh` — one-command installer (binary or source)
- GitHub Actions CI (Ubuntu + macOS)
- GitHub Actions Release (Linux x86_64, ARM64, macOS universal, Windows)
- Homebrew formula
- MIT License

---

## Roadmap

See [ROADMAP.md](ROADMAP.md) for the full versioned roadmap.
