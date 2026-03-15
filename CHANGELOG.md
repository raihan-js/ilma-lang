# Changelog

All notable changes to ILMA are documented here.

---

## [0.5.0] — 2026

### Added

**Interactive REPL**
- `ilma` with no arguments starts an interactive prompt
- Persistent state across inputs (variables, recipes survive between lines)
- Multi-line input when lines end with `:` (continuation prompt `...>`)
- Special commands: quit, help, clear, reset

**Language Specification**
- SPEC.md — 1622-line formal language specification
- Complete EBNF grammar for the entire language
- Full keyword reference, type system, and module API documentation

**Windows Support**
- PowerShell installer: `irm https://ilma-lang.dev/install.ps1 | iex`
- Binary download with PATH setup
- Fallback to source build with MinGW

**Tests**
- 8 new edge case and integration tests
- Total: 65 compiler tests (23 Tier 1, 23 Tier 2, 19 Tier 3), all passing
- Tests for: empty values, string methods, or-patterns, bag.map/filter, blueprints with check/when

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
