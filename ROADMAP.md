# ILMA Roadmap

## v0.1.0 — Core Language ✅

- ILMA compiler (lexer, parser, AST, code generator)
- 41 compiler tests across all three tiers (Seed, Sapling, Tree)
- Complete OOP: blueprints, inheritance, method dispatch
- Error handling: try / when wrong / shout
- Collections: bags, notebooks
- Unicode support (Arabic/Urdu variable names)
- Web IDE with Monaco editor and JS interpreter
- 30 structured lessons with Socratic tutor
- 9 knowledge modules (JS)
- PWA offline support
- Docker deployment
- Multi-platform release builds (Linux, macOS, Windows)

## v0.2.0 — Standard Library & Language Features ✅

- Port knowledge modules to C compiler (finance, time, quran, science, trade, body, number, think, draw)
- String interpolation: `say "Hello {name}"`
- Multi-line strings
- Range operator: `repeat i in 1..10:`
- Standard math functions (sqrt, abs, round, random, floor, ceil)
- File I/O: `read_file()`, `write_file()`
- Date/time operations

## v0.3.0 — Ecosystem & Tooling ✅

- draw module: SVG canvas, shapes, Islamic geometric star patterns
- number module: math functions, primes, Fibonacci, binary/hex conversion
- File I/O built-ins: read_file(), write_file(), file_exists()
- AST evaluator for direct interpretation (WASM-ready)
- LSP server foundation (ilma-lsp binary with diagnostics + hover)
- VS Code extension Marketplace-ready with publish scripts
- GitHub Pages deployment with custom domain
- Website playground page with 8 examples, share/download
- 50 compiler tests, all passing

## v0.4.0 — Language Maturity ✅

- Pattern matching: check/when/otherwise with ranges and or-patterns
- Lambda expressions: anonymous recipes, bag.map/filter/each
- Multi-line strings (triple quotes) and raw strings (backticks)
- Package manager: ilma get, ilma packages
- science module: physics, temperature, energy, Ohm's law
- trade module: profit, margin, halal trade checker
- IlmaWeb: todo app example, SQLite integration
- 57 compiler tests, all passing

## v0.5.0 — Professional Release ✅

- Interactive REPL with persistent state and multi-line input
- Language specification (SPEC.md, 1622 lines, full EBNF grammar)
- Windows PowerShell installer (install.ps1)
- 65 compiler tests + 2 LSP tests, all passing

## v0.6.0 — Ecosystem Growth 📋

- WASM compilation via Emscripten (real compiler in browser)
- Full LSP: go-to-definition, auto-complete, rename
- Cross-platform installers (APT PPA, AUR, Scoop, winget)
- Generics / type parameters
- HTTP client in stdlib

## v1.0.0 — Production Ready 📋

- Language specification document frozen
- Homebrew formula published
- Full test coverage and security audit
- Stable API — no breaking changes
- Comprehensive standard library
- SaaS learning platform at ilma-lang.dev/learn
- Full-stack framework for production web apps
- Debugger integration for VS Code
