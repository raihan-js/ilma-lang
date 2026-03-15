# Changelog

All notable changes to ILMA are documented here.

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
