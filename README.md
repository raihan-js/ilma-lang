# ILMA

**A programming language for children, built on C, simpler than Python.**

ILMA (from Arabic *ilm* — knowledge) is a complete programming language designed for children as young as three, built to grow with them into adulthood. Every keyword is a word a child already knows. Every module teaches something about the real world.

```
say "Bismillah"
remember name = ask "What is your name? "
say "Assalamu Alaikum, " + name

repeat 3:
    say "SubhanAllah"
```

## Features

- **Three tiers**: Seed (ages 3-7), Sapling (7-13), Tree (13+) — one language, three depth levels
- **Original keywords**: `say`, `remember`, `recipe`, `blueprint`, `give back`, `bag`, `notebook`
- **Compiles to C**: Native performance, runs on any device
- **Web IDE**: Monaco editor, in-browser interpreter, 30 structured lessons
- **Knowledge modules**: `finance`, `time`, `quran`, `science`, `trade`, `body`, `number`, `think`
- **Islamic values**: Zakat calculations, Hijri calendar, Quran search, halal trade checks
- **Unicode support**: Arabic and Urdu variable names from day one
- **Child-friendly errors**: No jargon, plain English messages
- **Socratic tutor**: Never gives answers — asks guiding questions

## Quick Start

### Install (one command)

```bash
curl -fsSL https://ilmalang.dev/install.sh | bash
```

### Or with Homebrew (macOS)

```bash
brew install ilmalang/ilma/ilma
```

### Or from source

```bash
git clone https://github.com/raihan-js/ilma-lang
cd ilma-lang && make && sudo make install
```

### Or with Docker

```bash
docker compose up -d
# Open http://localhost:3000
```

### Run a program

```bash
ilma program.ilma                  # compile and run
ilma --c program.ilma              # show generated C
ilma --tokens program.ilma         # show lexer tokens
ilma --compile program.ilma        # compile to binary only
```

## The Three Tiers

### Tier 1 — Seed (ages 3-7)
```
say "Hello, World!"
remember age = 7
repeat 5:
    say "I love learning"
```

### Tier 2 — Sapling (ages 7-13)
```
use finance
remember savings = 5000
remember zakat = finance.zakat(savings, 595)
say "Zakat due: " + zakat

recipe greet(name):
    say "Salaam, " + name
greet("Amira")

remember fruits = bag["dates", "mango", "apple"]
for each fruit in fruits:
    say fruit
```

### Tier 3 — Tree (ages 13+)
```
blueprint BankAccount:
    create(owner, balance):
        me.owner = owner
        me.balance = balance
    recipe deposit(amount):
        me.balance = me.balance + amount
    recipe show():
        say me.owner + ": " + me.balance

blueprint SavingsAccount comes from BankAccount:
    create(owner, balance, rate):
        comes_from.create(owner, balance)
        me.rate = rate

remember acc = SavingsAccount("Yusuf", 1000, 0.05)
acc.deposit(200)
acc.show()
```

## Knowledge Modules

| Module | What it teaches |
|--------|----------------|
| `finance` | Compound interest, zakat, budgeting, profit margins |
| `time` | Gregorian and Hijri calendar, date calculations |
| `quran` | Search ayat, daily verses, surah lookup |
| `science` | Gravity, temperature, kinetic energy |
| `trade` | Profit/loss, supply and demand, halal commerce |
| `body` | BMI, water intake, health metrics |
| `number` | Binary, hexadecimal, Roman numerals, primes, Fibonacci |
| `think` | Stoic reflection questions, decision-making |
| `draw` | Canvas graphics, Islamic geometric patterns |

## Architecture

```
.ilma source → Lexer → Parser → AST → Code Generator → C → GCC → Native Binary
```

| Component | File | Description |
|-----------|------|-------------|
| Lexer | `src/lexer.c` | Tokenizes ILMA source with Unicode support |
| Parser | `src/parser.c` | Recursive descent parser for all three tiers |
| AST | `src/ast.c` | 22 node types covering the full language |
| Code Generator | `src/codegen.c` | Emits clean, readable C code |
| Runtime | `src/runtime/` | Tagged union values, bags, notebooks, objects |
| Web IDE | `web/` | Monaco editor + JS interpreter + 30 lessons |

## Test Suite

- **41 compiler tests** across all three tiers (Tier 1: 20, Tier 2: 13, Tier 3: 8)
- **30 lesson programs** verified against the JS interpreter
- Run with `make test` or `bash tests/run_tests.sh`

## Web Platform

The web platform at `web/` includes:

- Monaco editor with ILMA syntax highlighting and autocompletion
- In-browser JavaScript interpreter (no server needed)
- 30 structured lessons from "Hello World" to advanced recursion
- Socratic tutor that gives hints, never answers
- Teacher dashboard with progress tracking and CSV export
- PWA offline support
- Program sharing via URL

## Build a Web Server

With **IlmaWeb**, you can build web applications entirely in ILMA:

```
use web

web.route("/"):
    give back web.html("<h1>Hello from IlmaWeb!</h1>")

web.route("/greet"):
    remember name = web.query("name")
    give back web.html("<h1>Salaam, " + name + "!</h1>")

web.route("/api/status"):
    remember data = notebook["status": "running", "language": "ILMA"]
    give back web.json(data)

web.start(port: 3000)
```

See the [IlmaWeb Framework](frameworks/ilma-web/) for the full guide.

## Ecosystem

| Project | Description |
|---------|-------------|
| [ilmalang.dev](https://ilmalang.dev) | Official website — docs, playground, install guide |
| [VS Code Extension](vscode-extension/) | Syntax highlighting, snippets, and run command for VS Code |
| [IlmaWeb Framework](frameworks/ilma-web/) | Build web servers and APIs entirely in ILMA |
| [Web IDE](web/) | In-browser Monaco editor with 30 lessons and Socratic tutor |

## The Name

ILMA comes from the Arabic and Urdu word **ilm** — knowledge, science, wisdom. The first divine command in the Quran was **Iqra** — Read. This language carries that mission: make knowledge accessible to every child on Earth.

## License

MIT License. The ILMA compiler, runtime, and web platform are free forever.
