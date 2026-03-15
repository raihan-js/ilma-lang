# Changelog

All notable changes to ILMA are documented here.

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

### [0.2.0] — planned
- C compiler standard library modules (finance, time, etc.)
- ILMA → WebAssembly compilation via Emscripten
- Multi-line strings
- String interpolation
- Package manager

### [1.0.0] — planned
- Stable language specification
- ILMA Web framework
- ILMA Mobile compilation target
- Full documentation site
