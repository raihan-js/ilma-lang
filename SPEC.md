# ILMA Language Specification

## Version 0.8.0

**Status:** Draft
**Date:** 2026-03-15
**Authors:** Akteruzzaman Raihan
**License:** MIT

---

### 1. Introduction

ILMA (from Arabic *ilm* --- knowledge) is a programming language designed for children
and lifelong learners. It compiles to C for native performance and can also be
interpreted in the browser via a JavaScript evaluator.

ILMA is not a toy language. It is a real, statically-compiled language whose syntax
happens to read like plain English. A five-year-old can write `say "Bismillah"` and
a university student can build a linked list with blueprints --- the same compiler
handles both.

#### 1.1 Design Principles

1. **Every keyword is a word a child already knows** --- `say`, `remember`, `recipe`,
   `blueprint`, `ask`, `give back`, `keep going while`.
2. **Three tiers grow with the learner** --- Seed (ages 3--7) uses `say` and `ask`;
   Sapling (ages 7--13) adds loops, recipes, and bags; Tree (ages 13+) unlocks
   blueprints, inheritance, modules, and error handling.
3. **Real-world knowledge built in** --- finance, science, quran, health, ethics,
   and trade modules ship with the compiler.
4. **Unicode from day one** --- Arabic and Urdu variable names, string literals, and
   comments are first-class citizens.
5. **No semicolons, no braces** --- indentation-based blocks, like Python.
6. **Compiles to C** --- every ILMA program produces a single `.c` file that any
   C compiler can build into a native binary.

#### 1.2 Compilation Targets

| Target        | Toolchain                           |
|---------------|-------------------------------------|
| Native binary | `.ilma` -> Lexer -> Parser -> AST -> CodeGen -> `.c` -> GCC/Clang -> binary |
| Web (WASM)    | `.ilma` -> Lexer -> Parser -> AST -> Evaluator (C compiled to WASM)        |
| Web (JS)      | `.ilma` -> JavaScript interpreter in the browser                           |

#### 1.3 Notational Conventions

- `UPPER_CASE` denotes a terminal token produced by the lexer.
- `lower_case` denotes a non-terminal in the grammar.
- `[...]` denotes an optional element.
- `{...}` denotes zero or more repetitions.
- `|` separates alternatives.
- Code examples use monospace blocks. Output is shown with `# =>` comments.

---

### 2. Lexical Structure

#### 2.1 Character Encoding

ILMA source files MUST be encoded in UTF-8. The byte-order mark (BOM) is permitted
but ignored. Any Unicode letter (category L) or Unicode digit (category Nd) may
appear in identifiers. All string literals support the full Unicode repertoire.

#### 2.2 Keywords

The following words are reserved. They cannot be used as identifiers.

| Keyword       | Token           | Category        |
|---------------|-----------------|-----------------|
| `say`         | TOK_SAY         | Output          |
| `ask`         | TOK_ASK         | Input           |
| `remember`    | TOK_REMEMBER    | Declaration     |
| `if`          | TOK_IF          | Control flow    |
| `otherwise`   | TOK_OTHERWISE   | Control flow    |
| `repeat`      | TOK_REPEAT      | Loop            |
| `keep`        | TOK_KEEP        | Loop            |
| `going`       | TOK_GOING       | Loop            |
| `while`       | TOK_WHILE       | Loop            |
| `for`         | TOK_FOR         | Loop            |
| `each`        | TOK_EACH        | Loop            |
| `in`          | TOK_IN          | Loop / Operator |
| `recipe`      | TOK_RECIPE      | Function        |
| `give`        | TOK_GIVE        | Return          |
| `back`        | TOK_BACK        | Return          |
| `blueprint`   | TOK_BLUEPRINT   | OOP             |
| `create`      | TOK_CREATE      | OOP             |
| `me`          | TOK_ME          | OOP             |
| `comes`       | TOK_COMES       | OOP             |
| `from`        | TOK_FROM        | OOP             |
| `comes_from`  | TOK_COMES_FROM  | OOP             |
| `use`         | TOK_USE         | Import          |
| `shout`       | TOK_SHOUT       | Error handling  |
| `try`         | TOK_TRY         | Error handling  |
| `when`        | TOK_WHEN        | Error handling / Pattern match |
| `wrong`       | TOK_WRONG       | Error handling  |
| `yes`         | TOK_YES         | Literal         |
| `no`          | TOK_NO          | Literal         |
| `empty`       | TOK_EMPTY       | Literal         |
| `and`         | TOK_AND         | Logic           |
| `or`          | TOK_OR          | Logic           |
| `not`         | TOK_NOT         | Logic           |
| `is`          | TOK_IS          | Comparison      |
| `bag`         | TOK_BAG         | Collection      |
| `notebook`    | TOK_NOTEBOOK    | Collection      |
| `check`       | TOK_CHECK       | Pattern match   |

Type keywords (used only in optional type annotations):

| Keyword       | Token              |
|---------------|--------------------|
| `whole`       | TOK_TYPE_WHOLE     |
| `decimal`     | TOK_TYPE_DECIMAL   |
| `text`        | TOK_TYPE_TEXT      |
| `truth`       | TOK_TYPE_TRUTH     |
| `anything`    | TOK_TYPE_ANYTHING  |

#### 2.3 Identifiers

An identifier starts with a Unicode letter or underscore (`_`), followed by zero or
more Unicode letters, Unicode digits, or underscores.

```
IDENT = (UNICODE_LETTER | "_") { UNICODE_LETTER | UNICODE_DIGIT | "_" }
```

Examples of valid identifiers:

```
name
_private
studentAge
عمر
نام
student_1
حساب_زکوٰة
```

Identifiers are case-sensitive: `Name` and `name` are different variables.

#### 2.4 Literals

**Integer literals** (whole numbers):

```
INT_LIT = ["-"] DIGIT { DIGIT }
```

Examples: `0`, `42`, `-7`, `1000000`

**Floating-point literals** (decimal numbers):

```
FLOAT_LIT = ["-"] DIGIT { DIGIT } "." DIGIT { DIGIT }
```

Examples: `3.14`, `0.5`, `-2.718`, `100.0`

**String literals** (text):

```
STRING_LIT = '"' { CHAR } '"'
           | "'" { CHAR } "'"
```

Escape sequences: `\"`, `\'`, `\\`, `\n`, `\t`.

Examples: `"Hello"`, `'world'`, `"She said \"hi\""`, `"line1\nline2"`

**String interpolation**:

Strings enclosed in double quotes may contain `{expression}` blocks, which are
evaluated at runtime and converted to text.

```
"Hello {name}, you are {age} years old"
```

The lexer produces a `TOK_STRING_INTERP` token containing alternating text
fragments and expression references.

**Boolean literals** (truth values):

```
yes    # true
no     # false
```

**Empty literal**:

```
empty  # null / nothing / absent value
```

#### 2.5 Operators

Listed from lowest to highest precedence:

| Precedence | Operators             | Associativity | Description          |
|------------|-----------------------|---------------|----------------------|
| 1          | `or`                  | Left          | Logical OR           |
| 2          | `and`                 | Left          | Logical AND          |
| 3          | `is`, `is not`        | Left          | Equality             |
| 4          | `<`, `>`, `<=`, `>=`  | Left          | Comparison           |
| 5          | `+`, `-`              | Left          | Addition, subtraction|
| 6          | `*`, `/`, `%`         | Left          | Multiply, divide, mod|
| 7          | `not`, `-` (unary)    | Right (unary) | Logical NOT, negation|
| 8          | `.`, `[]`, `()`       | Left          | Member, index, call  |

Additional operators:

| Operator | Token        | Description         |
|----------|--------------|---------------------|
| `=`      | TOK_ASSIGN   | Assignment          |
| `..`     | TOK_DOTDOT   | Range               |
| `.`      | TOK_DOT      | Member access       |
| `:`      | TOK_COLON    | Block opener        |
| `,`      | TOK_COMMA    | Separator           |
| `(`  `)` | TOK_LPAREN/RPAREN | Grouping / Call |
| `[`  `]` | TOK_LBRACKET/RBRACKET | Index / Literal |

#### 2.6 Comments

ILMA supports single-line comments only. A comment begins with `#` and extends
to the end of the line.

```
# This is a comment
say "Hello"  # This is also a comment
```

There are no multi-line or block comments.

#### 2.7 Indentation

ILMA uses indentation to delimit blocks, similar to Python.

- **4 spaces** is the recommended indentation unit.
- Tab characters are converted to 4 spaces during lexing.
- Indentation MUST be consistent within a block. Mixing indentation levels
  within the same block is a compile-time error.
- The lexer emits `TOK_INDENT` when indentation increases and `TOK_DEDENT`
  when it decreases. Multiple `TOK_DEDENT` tokens may be emitted when
  indentation drops by more than one level.
- The indentation stack supports up to 128 nesting levels.

#### 2.8 Line Continuation

A line ending with `:` (colon) opens a new indented block on the next line.
Blank lines within a block are ignored. There is no explicit line continuation
character; each statement occupies exactly one line unless it contains a block.

#### 2.9 Token Stream

The lexer produces the following structural tokens in addition to keyword,
literal, and operator tokens:

| Token        | Description                              |
|--------------|------------------------------------------|
| TOK_INDENT   | Indentation increased                    |
| TOK_DEDENT   | Indentation decreased                    |
| TOK_NEWLINE  | End of a logical line                    |
| TOK_EOF      | End of source file                       |

---

### 3. Type System

#### 3.1 Types

ILMA has eight value types. Every runtime value carries a type tag.

| ILMA Name  | Type Tag       | C Equivalent | Description                  |
|------------|----------------|--------------|------------------------------|
| `whole`    | ILMA_WHOLE     | `int64_t`    | 64-bit signed integer        |
| `decimal`  | ILMA_DECIMAL   | `double`     | 64-bit IEEE 754 float        |
| `text`     | ILMA_TEXT      | `char*`      | Heap-allocated UTF-8 string  |
| `truth`    | ILMA_TRUTH     | `int`        | Boolean: `yes` (1) / `no` (0) |
| `bag`      | ILMA_BAG       | `IlmaBag*`   | Ordered dynamic array        |
| `notebook` | ILMA_NOTEBOOK  | `IlmaBook*`  | Key-value hash map           |
| `object`   | ILMA_OBJECT    | `IlmaObj*`   | Blueprint instance           |
| `empty`    | ILMA_EMPTY     | ---          | Null / absence of value      |

#### 3.2 Type Inference

Variable types are inferred from their initialization expression. No explicit type
annotation is required.

```
remember age = 12           # whole
remember pi = 3.14          # decimal
remember name = "Yusuf"     # text
remember alive = yes        # truth
remember items = bag[1, 2]  # bag
```

Optional type annotations are supported for documentation and future static
analysis:

```
remember age: whole = 12
remember name: text = "Yusuf"
```

#### 3.3 Type Coercion

Implicit coercion follows these rules:

| Left Type | Operator | Right Type | Result Type |
|-----------|----------|------------|-------------|
| `whole`   | `+ - * / %` | `decimal` | `decimal`   |
| `decimal` | `+ - * / %` | `whole`   | `decimal`   |
| `text`    | `+`      | any        | `text` (concatenation via `ilma_to_string`) |
| any       | `+`      | `text`     | `text` (concatenation via `ilma_to_string`) |
| `truth`   | arithmetic | ---      | `whole` (`yes` = 1, `no` = 0) |

Division by zero produces a runtime error via `shout`.

#### 3.4 Truthiness

Any value can be tested for truthiness in conditions:

| Type       | Truthy                     | Falsy          |
|------------|----------------------------|----------------|
| `whole`    | Non-zero                   | `0`            |
| `decimal`  | Non-zero                   | `0.0`          |
| `text`     | Non-empty                  | `""`           |
| `truth`    | `yes`                      | `no`           |
| `bag`      | Non-empty                  | Empty bag      |
| `notebook` | Non-empty                  | Empty notebook |
| `object`   | Always truthy              | ---            |
| `empty`    | ---                        | Always falsy   |

---

### 4. Statements

#### 4.1 Variable Declaration

```
remember name = expression
remember name: type = expression   # with optional type annotation
```

The `remember` keyword declares a new variable in the current scope. The variable
is initialized immediately and its type is inferred from the right-hand side
(or validated against the annotation if one is provided).

```
remember age = 12
remember greeting: text = "Assalamu Alaikum"
```

#### 4.2 Assignment

```
target = expression
```

Where `target` is an identifier, a member access (`object.field`), or an index
access (`bag[i]`, `notebook[key]`).

```
age = 13
me.name = "Yusuf"
items[0] = "updated"
```

#### 4.3 Output

```
say expression
```

Evaluates the expression, converts it to text via `ilma_to_string`, and prints
it to standard output followed by a newline.

```
say "Bismillah"
say 2 + 3           # => 5
say "Hi " + name    # => Hi Yusuf
```

#### 4.4 Input

```
remember answer = ask "prompt"
```

The `ask` keyword prints the prompt string (without a trailing newline), reads a
line from standard input, and returns it as a `text` value.

```
remember name = ask "What is your name? "
say "Hello " + name
```

#### 4.5 Conditionals

```
if condition:
    body

if condition:
    body
otherwise:
    body

if condition:
    body
otherwise if condition:
    body
otherwise:
    body
```

The `otherwise if` chain can be repeated any number of times. Each `condition`
is an expression tested for truthiness.

```
remember score = 85

if score >= 90:
    say "Excellent!"
otherwise if score >= 70:
    say "Good job!"
otherwise:
    say "Keep trying!"
```

#### 4.6 Pattern Matching

```
check expression:
    when value:
        body
    when low..high:
        body
    when val1 or val2:
        body
    otherwise:
        body
```

The `check` statement evaluates the subject expression once, then tests it
against each `when` pattern in order. The first matching pattern's body is
executed. If no pattern matches and an `otherwise` clause is present, its body
is executed.

Pattern types:
- **Value pattern**: exact equality match.
- **Range pattern**: `low..high` matches if `low <= subject <= high`.
- **Or pattern**: `val1 or val2` matches if either value matches.

```
remember day = 3
check day:
    when 1:
        say "Monday"
    when 2:
        say "Tuesday"
    when 3:
        say "Wednesday"
    when 6 or 7:
        say "Weekend!"
    otherwise:
        say "Another day"
```

#### 4.7 Loops

**Count loop:**

```
repeat count:
    body
```

Executes the body `count` times. The count expression must evaluate to a `whole`.

```
repeat 5:
    say "Alhamdulillah"
```

**Range loop:**

```
repeat variable in start..end:
    body
```

Iterates `variable` from `start` to `end - 1` (exclusive upper bound).

```
repeat i in 1..6:
    say i    # prints 1, 2, 3, 4, 5
```

**For-each loop:**

```
for each variable in collection:
    body
```

Iterates over each element in a bag.

```
remember fruits = bag["apple", "banana", "mango"]
for each fruit in fruits:
    say fruit
```

**While loop:**

```
keep going while condition:
    body
```

Executes the body repeatedly as long as the condition is truthy.

```
remember n = 10
keep going while n > 0:
    say n
    n = n - 1
```

#### 4.8 Error Handling

**Try / When Wrong:**

```
try:
    body
when wrong:
    body
```

If any statement in the `try` body calls `shout` (or causes a runtime error),
execution transfers to the `when wrong` body. The error stack supports nesting
up to 64 levels deep.

```
try:
    remember result = 10 / 0
when wrong:
    say "Something went wrong!"
```

**Shout (throw):**

```
shout expression
```

Raises a runtime error. If a `try` block is active, control transfers to its
`when wrong` handler. Otherwise, the program terminates with the error message.

```
recipe divide(a, b):
    if b is 0:
        shout "Cannot divide by zero!"
    give back a / b
```

#### 4.9 Module Import

```
use module_name
```

Imports a built-in module, making its functions available via dot notation:
`module_name.function_name(args)`.

```
use number
say number.pi()        # => 3.141593
say number.sqrt(16)    # => 4.000000
```

#### 4.10 Expression Statements

Any expression can be used as a statement. This is primarily useful for calling
recipes (functions) for their side effects.

```
cat.speak()
items.add(42)
```

---

### 5. Expressions

#### 5.1 Primary Expressions

| Form                              | Node Type          | Description              |
|-----------------------------------|--------------------|--------------------------|
| `42`                              | NODE_INT_LIT       | Integer literal          |
| `3.14`                            | NODE_FLOAT_LIT     | Float literal            |
| `"hello"`                         | NODE_STRING_LIT    | String literal           |
| `"hi {name}"`                     | NODE_STRING_INTERP | Interpolated string      |
| `yes` / `no`                      | NODE_BOOL_LIT      | Boolean literal          |
| `empty`                           | NODE_EMPTY_LIT     | Null literal             |
| `identifier`                      | NODE_IDENTIFIER    | Variable reference       |
| `me`                              | NODE_IDENTIFIER    | Self-reference in blueprint |
| `comes_from`                      | NODE_IDENTIFIER    | Parent-reference in blueprint |
| `ask expression`                  | NODE_ASK_EXPR      | User input               |
| `bag[e1, e2, ...]`               | NODE_BAG_LIT       | Bag literal              |
| `notebook[k1: v1, k2: v2, ...]`  | NODE_NOTEBOOK_LIT  | Notebook literal         |
| `(expression)`                    | ---                | Grouping                 |

#### 5.2 Binary Expressions

```
expression operator expression
```

See Section 2.5 for the complete precedence table. All binary operators are
left-associative.

```
2 + 3 * 4        # => 14 (multiplication binds tighter)
(2 + 3) * 4      # => 20 (parentheses override)
"Hello " + name  # => "Hello Yusuf" (string concatenation)
age >= 18 and has_id is yes  # logical expression
```

#### 5.3 Unary Expressions

```
not expression    # logical negation
-expression       # arithmetic negation
```

```
not yes           # => no
-42               # => -42
not (age > 18)    # => yes if age <= 18
```

#### 5.4 Call Expressions

```
callee(arg1, arg2, ...)
```

Where `callee` is an identifier or a member access expression.

```
fibonacci(10)
math.sqrt(16)
items.add(42)
Animal("Cat", "Meow")    # blueprint constructor call
```

#### 5.5 Member Access

```
expression.identifier
```

Accesses a field or method on an object, bag, text, or notebook.

```
me.name
animal.speak()
items.size
data.keys()
```

#### 5.6 Index Access

```
expression[expression]
```

Accesses an element of a bag by integer index or a notebook by key.

```
items[0]
data["name"]
```

#### 5.7 Lambda Expressions

```
recipe(params): body
recipe(params): give back expression
```

Anonymous recipes can be used inline, particularly with higher-order bag methods.

```
remember doubled = items.map(recipe(n): give back n * 2)
remember evens = items.filter(recipe(n): give back n % 2 is 0)
```

---

### 6. Recipes (Functions)

A recipe is ILMA's term for a function.

#### 6.1 Declaration

```
recipe name(param1, param2, ...):
    body
    give back result
```

Recipes are declared with the `recipe` keyword. Parameters are positional and
untyped (types are determined at call time). The `give back` statement returns
a value to the caller. If no `give back` is executed, `empty` is returned.

```
recipe greet(name):
    say "Assalamu Alaikum, " + name + "!"

recipe add(a, b):
    give back a + b

recipe factorial(n):
    if n <= 1:
        give back 1
    give back n * factorial(n - 1)
```

#### 6.2 Calling Recipes

```
greet("Yusuf")
remember sum = add(3, 4)
say factorial(5)    # => 120
```

#### 6.3 Recipes as Values

Recipes can be assigned to variables and passed as arguments:

```
remember f = recipe(x): give back x * x
say f(5)    # => 25

remember items = bag[1, 2, 3, 4, 5]
remember squares = items.map(recipe(n): give back n * n)
```

#### 6.4 Scope

Each recipe call creates a new variable scope. Variables declared inside a recipe
are local to that call. Recipes can access variables from enclosing scopes
(lexical scoping).

---

### 7. Blueprints (Classes)

A blueprint is ILMA's term for a class.

#### 7.1 Declaration

```
blueprint Name:
    create(param1, param2):
        me.field1 = param1
        me.field2 = param2

    recipe method_name():
        body
```

The `create` block is the constructor. `me` refers to the current object instance
(equivalent to `this` or `self` in other languages). Fields are created dynamically
by assigning to `me.field_name` in the constructor or any method.

```
blueprint Animal:
    create(name, sound):
        me.name = name
        me.sound = sound

    recipe speak():
        say me.name + " says: " + me.sound

    recipe describe():
        say "I am a " + me.name
```

#### 7.2 Instantiation

```
remember instance = BlueprintName(arg1, arg2)
```

Calling a blueprint name as a function invokes its `create` constructor and
returns a new object.

```
remember cat = Animal("Cat", "Meow")
cat.speak()      # => Cat says: Meow
cat.describe()   # => I am a Cat
```

#### 7.3 Inheritance

```
blueprint Child comes from Parent:
    create(params):
        comes_from.create(params)
        me.extra_field = value

    recipe extra_method():
        body
```

A blueprint can inherit from exactly one parent blueprint using `comes from`.
Inside the child, `comes_from` refers to the parent blueprint, allowing the
child to call the parent's constructor and methods.

```
blueprint Dog comes from Animal:
    create(name):
        comes_from.create(name, "Woof")
        me.tricks = bag[]

    recipe learn(trick):
        me.tricks.add(trick)
        say me.name + " learned " + trick + "!"

remember rex = Dog("Rex")
rex.speak()          # => Rex says: Woof
rex.learn("sit")     # => Rex learned sit!
```

#### 7.4 Field Access

Object fields are accessed with dot notation: `object.field`. Fields can be read
and written from any code that holds a reference to the object.

```
say cat.name       # => Cat
cat.name = "Kitty"
say cat.name       # => Kitty
```

---

### 8. Collections

#### 8.1 Bags (Dynamic Arrays)

A bag is an ordered, dynamically-sized collection of values. Bags can hold
values of mixed types.

**Creation:**

```
remember items = bag[1, 2, 3]
remember empty_bag = bag[]
remember mixed = bag[1, "hello", yes, 3.14]
```

**Methods:**

| Method                        | Description                       | Returns     |
|-------------------------------|-----------------------------------|-------------|
| `bag.add(value)`              | Append a value to the end         | (nothing)   |
| `bag.remove(value)`           | Remove first occurrence of value  | (nothing)   |
| `bag.size`                    | Number of elements                | `whole`     |
| `bag.sorted()`                | Return a new sorted bag           | `bag`       |
| `bag.map(recipe)`             | Apply recipe to each, return new bag | `bag`    |
| `bag.filter(recipe)`          | Keep elements where recipe returns truthy | `bag` |
| `bag.each(recipe)`            | Apply recipe to each (no return)  | (nothing)   |
| `bag[index]`                  | Access element by zero-based index | value      |

**Examples:**

```
remember numbers = bag[5, 3, 8, 1, 9]
numbers.add(4)
say numbers.size              # => 6

remember sorted = numbers.sorted()
say sorted                    # => [1, 3, 4, 5, 8, 9]

remember doubled = numbers.map(recipe(n): give back n * 2)
remember big = numbers.filter(recipe(n): give back n > 5)
```

#### 8.2 Notebooks (Hash Maps)

A notebook is an unordered collection of key-value pairs. Keys are always text
strings.

**Creation:**

```
remember student = notebook[name: "Yusuf", age: 12, grade: "7th"]
remember empty_book = notebook[]
```

**Methods:**

| Method                             | Description                     | Returns     |
|------------------------------------|---------------------------------|-------------|
| `notebook[key]`                    | Get value by key                | value       |
| `notebook.keys()`                  | Return all keys as a bag        | `bag`       |
| `notebook.size`                    | Number of key-value pairs       | `whole`     |

**Setting values:**

```
remember data = notebook[name: "Yusuf"]
# Keys are accessed as identifiers, not strings, inside notebook literals
say data[name]     # => Yusuf
```

---

### 9. Modules

#### 9.1 Overview

ILMA ships with eleven built-in modules. Each module is imported with the
`use` statement and its functions are called via dot notation.

```
use finance
remember total = finance.compound(1000, 5, 10)
```

#### 9.2 finance Module

Functions for financial literacy.

| Function | Signature | Description |
|----------|-----------|-------------|
| `compound` | `compound(principal, rate, years)` | Compound interest: `P * (1 + r/100)^t` |
| `zakat` | `zakat(wealth, nisab)` | Calculate zakat (2.5% of wealth above nisab) |
| `profit` | `profit(cost, revenue)` | Absolute profit: `revenue - cost` |
| `margin` | `margin(cost, revenue)` | Profit margin as percentage |
| `simple_interest` | `simple_interest(principal, rate, years)` | Simple interest: `P * r * t / 100` |

#### 9.3 time Module

Functions for date and calendar operations.

| Function | Signature | Description |
|----------|-----------|-------------|
| `today_str` | `today_str()` | Current date as "YYYY-MM-DD" text |
| `to_hijri` | `to_hijri(gregorian_str)` | Approximate Hijri calendar conversion |
| `days_between` | `days_between(date1, date2)` | Number of days between two date strings |

#### 9.4 body Module

Functions for health and wellness education.

| Function | Signature | Description |
|----------|-----------|-------------|
| `bmi` | `bmi(weight_kg, height_cm)` | Body Mass Index calculation |
| `bmi_category` | `bmi_category(bmi_value)` | Category text: underweight / normal / overweight / obese |
| `daily_water` | `daily_water(weight_kg)` | Recommended daily water intake in liters |
| `sleep_advice` | `sleep_advice(hours, age)` | Sleep quality feedback text |

#### 9.5 think Module

Functions for critical thinking and ethical reasoning.

| Function | Signature | Description |
|----------|-----------|-------------|
| `stoic_question` | `stoic_question()` | Random Stoic reflection question |
| `pros_cons_new` | `pros_cons_new(question)` | Create a new pros/cons analysis object |
| `weigh_result` | `weigh_result(pros_bag, cons_bag)` | Summarize and weigh a pros/cons analysis |

#### 9.6 quran Module

Functions for Quranic reference and learning.

| Function | Signature | Description |
|----------|-----------|-------------|
| `ayah_of_the_day` | `ayah_of_the_day()` | Return a daily ayah (verse) as text |
| `search` | `search(keyword)` | Search for ayahs containing a keyword |
| `surah` | `surah(name)` | Return information about a surah by name |

#### 9.7 draw Module

Functions for creating SVG drawings.

| Function | Signature | Description |
|----------|-----------|-------------|
| `canvas` | `canvas(width, height)` | Create a new drawing canvas |
| `circle` | `circle(canvas, cx, cy, r, color)` | Draw a circle |
| `rect` | `rect(canvas, x, y, w, h, color)` | Draw a rectangle |
| `line` | `line(canvas, x1, y1, x2, y2, color)` | Draw a line |
| `text` | `text(canvas, x, y, text_val, color)` | Draw text on canvas |
| `polygon` | `polygon(canvas, points_bag, color)` | Draw a polygon from points |
| `islamic_star` | `islamic_star(canvas, cx, cy, size, points, color)` | Draw an Islamic star pattern |
| `save` | `save(canvas)` | Save canvas to SVG file |

#### 9.8 number Module

Mathematical functions and constants.

| Function | Signature | Description |
|----------|-----------|-------------|
| `sqrt` | `sqrt(n)` | Square root |
| `abs` | `abs(n)` | Absolute value |
| `floor` | `floor(n)` | Round down to nearest integer |
| `ceil` | `ceil(n)` | Round up to nearest integer |
| `round` | `round(n)` | Round to nearest integer |
| `power` | `power(base, exp)` | Exponentiation |
| `random` | `random(min, max)` | Random integer in range [min, max] |
| `is_prime` | `is_prime(n)` | Primality test, returns truth |
| `fibonacci` | `fibonacci(n)` | Nth Fibonacci number |
| `to_binary` | `to_binary(n)` | Binary string representation |
| `to_hex` | `to_hex(n)` | Hexadecimal string representation |
| `pi` | `pi()` | The constant pi (3.14159...) |
| `e` | `e()` | Euler's number (2.71828...) |

#### 9.9 science Module

Functions for physics and chemistry education.

| Function | Signature | Description |
|----------|-----------|-------------|
| `gravity` | `gravity(mass_kg)` | Weight force: `mass * 9.81` N |
| `kinetic_energy` | `kinetic_energy(mass_kg, velocity_ms)` | KE = 0.5 * m * v^2 |
| `potential_energy` | `potential_energy(mass_kg, height_m)` | PE = m * g * h |
| `speed` | `speed(distance_m, time_s)` | Speed = distance / time |
| `celsius_to_fahrenheit` | `celsius_to_fahrenheit(c)` | C to F conversion |
| `fahrenheit_to_celsius` | `fahrenheit_to_celsius(f)` | F to C conversion |
| `celsius_to_kelvin` | `celsius_to_kelvin(c)` | C to K conversion |
| `ohms_law_voltage` | `ohms_law_voltage(current, resistance)` | V = I * R |
| `ohms_law_current` | `ohms_law_current(voltage, resistance)` | I = V / R |
| `light_years_to_km` | `light_years_to_km(ly)` | Light-years to kilometers |
| `atom_count` | `atom_count(mass_g, molar_mass)` | Number of atoms via Avogadro |

#### 9.10 trade Module

Functions for commerce and business education.

| Function | Signature | Description |
|----------|-----------|-------------|
| `profit` | `profit(cost, revenue)` | Absolute profit: `revenue - cost` |
| `margin` | `margin(cost, revenue)` | Profit margin as percentage |
| `markup` | `markup(cost, selling_price)` | Markup percentage |
| `break_even` | `break_even(fixed_costs, price_per_unit, cost_per_unit)` | Break-even units |
| `supply_demand_price` | `supply_demand_price(base, demand_pct, supply_pct)` | Price adjustment |
| `vat` | `vat(price, vat_rate_pct)` | Price with VAT added |
| `discount` | `discount(price, discount_pct)` | Price after discount |
| `halal_check` | `halal_check(interest, gambling, alcohol)` | Halal compliance check |

#### 9.11 http Module

Functions for making outbound HTTP requests.

| Function | Signature | Description |
|----------|-----------|-------------|
| `get` | `get(url)` | HTTP GET — returns `notebook[status, body, headers]` |
| `post` | `post(url, body, content_type)` | HTTP POST — returns response notebook |
| `get_json` | `get_json(url)` | GET then parse JSON body |
| `post_json` | `post_json(url, data)` | POST JSON, parse response |

#### 9.12 web Module

Build HTTP web servers using an event-loop pattern. No external dependencies — pure POSIX sockets.

```
use web

web.listen(3000)
keep going while web.accept():
    remember path = web.path()
    if path is "/":
        web.send(web.html("<h1>Hello!</h1>"))
    otherwise if path is "/api/status":
        remember data = notebook[status: "ok", version: "0.8.0"]
        web.send(web.json(data))
    otherwise:
        web.status(404)
        web.send(web.html("<h1>404 Not Found</h1>"))
```

| Function | Signature | Description |
|----------|-----------|-------------|
| `listen` | `listen(port)` | Bind and start listening on the given port |
| `accept` | `accept()` | Accept next connection; returns `yes` on success |
| `path` | `path()` | Request path, e.g. `"/about"` |
| `method` | `method()` | HTTP method, e.g. `"GET"` |
| `query` | `query(key)` | Query-string parameter value, or `empty` |
| `body` | `body()` | Raw request body text |
| `header` | `header(name)` | Request header value by name (case-insensitive) |
| `html` | `html(content)` | Wrap content as `text/html` response |
| `json` | `json(data)` | Serialize notebook/bag to `application/json` response |
| `text` | `text(content)` | Wrap content as `text/plain` response |
| `send` | `send(response)` | Send response and close connection |
| `status` | `status(code)` | Set HTTP status code for next `send()` (default 200) |
| `redirect` | `redirect(url)` | Send 302 redirect |

#### 9.13 Package Manager

ILMA includes a built-in package manager for community packages.

```
ilma get package_name           # Install a package
ilma packages                   # List installed packages
ilma packages --available       # List available packages from registry
```

Packages are stored in `~/.ilma/packages/` on Unix systems and
`%USERPROFILE%\.ilma\packages\` on Windows.

---

### 10. Built-in Functions

These functions are available globally without any `use` statement.

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `read_file` | `read_file(path)` | `text` | Read entire file contents as text |
| `write_file` | `write_file(path, content)` | `truth` | Write text to file, returns `yes` on success |
| `file_exists` | `file_exists(path)` | `truth` | Check if a file exists at the given path |

#### 10.1 Text Methods

Text values have built-in methods accessible via dot notation:

| Method | Description | Returns |
|--------|-------------|---------|
| `text.length` | Number of characters | `whole` |
| `text.upper()` | Convert to uppercase | `text` |
| `text.lower()` | Convert to lowercase | `text` |
| `text.contains(search)` | Check if text contains substring | `truth` |
| `text.slice(start, end)` | Extract substring by index | `text` |
| `text.join(bag)` | Join bag elements with text as separator | `text` |

```
remember name = "Yusuf"
say name.length          # => 5
say name.upper()         # => YUSUF
say name.lower()         # => yusuf
say name.contains("us")  # => yes

remember items = bag["a", "b", "c"]
say ", ".join(items)     # => a, b, c
```

---

### 11. Compilation Pipeline

#### 11.1 Overview

```
                +-------+      +--------+      +-------+
  .ilma file -> | Lexer | ---> | Parser | ---> | AST   |
                +-------+      +--------+      +-------+
                  tokens                           |
                                                   v
                                             +---------+      +-----+
                                             | CodeGen | ---> | .c  |
                                             +---------+      +-----+
                                                                 |
                                                                 v
                                                           +-----------+
                                                           | GCC/Clang |
                                                           +-----------+
                                                                 |
                                                                 v
                                                            +--------+
                                                            | Binary |
                                                            +--------+
```

#### 11.2 Lexer Phase

The lexer (`src/lexer.c`) reads the UTF-8 source and produces a flat array of
tokens. It tracks indentation with a stack of up to 128 levels, emitting
`TOK_INDENT` and `TOK_DEDENT` tokens as indentation changes. The two-word keyword
`is not` is recognized as the single token `TOK_IS_NOT`.

#### 11.3 Parser Phase

The parser (`src/parser.c`) consumes the token array and produces an abstract syntax
tree (AST). It uses recursive descent with the precedence climbing method for
expressions. The parser reports syntax errors with line and column numbers.

#### 11.4 Code Generation Phase

The code generator (`src/codegen.c`) walks the AST and emits C source code. It:

- Wraps the program in a `main()` function.
- Includes the ILMA runtime header (`ilma_runtime.h`).
- Includes module headers for any `use` statements.
- Emits `IlmaValue`-typed variables for all `remember` declarations.
- Generates blueprint structs and constructor functions.
- Manages a temporary variable counter for complex expressions.

#### 11.5 C Compilation Phase

The generated `.c` file is compiled with GCC (or Clang) together with the runtime
library (`src/runtime/ilma_runtime.c`) and any module files referenced by `use`
statements.

#### 11.6 Interpreter Mode

When run with the `--eval` flag or in WASM mode, ILMA uses the evaluator
(`src/evaluator.c`) to directly walk the AST without generating C code. The
evaluator supports the same language features as the compiler pipeline but
executes them in a tree-walking interpreter.

---

### 12. Grammar (EBNF)

The complete formal grammar of ILMA in Extended Backus-Naur Form:

```ebnf
(* ===== Program structure ===== *)

program         = { statement NEWLINE } EOF ;

(* ===== Statements ===== *)

statement       = say_stmt
                | remember_stmt
                | assign_stmt
                | if_stmt
                | check_stmt
                | repeat_stmt
                | range_stmt
                | for_each_stmt
                | while_stmt
                | recipe_decl
                | blueprint_decl
                | use_stmt
                | give_back_stmt
                | try_stmt
                | shout_stmt
                | expr_stmt ;

say_stmt        = "say" expression ;

remember_stmt   = "remember" IDENT [ ":" type ] "=" expression ;

assign_stmt     = target "=" expression ;
target          = IDENT
                | member_access
                | index_access ;

if_stmt         = "if" expression ":" block
                  { "otherwise" "if" expression ":" block }
                  [ "otherwise" ":" block ] ;

check_stmt      = "check" expression ":" INDENT
                  { "when" pattern ":" block }
                  [ "otherwise" ":" block ]
                  DEDENT ;

pattern         = expression [ ".." expression ]
                  { "or" expression [ ".." expression ] } ;

repeat_stmt     = "repeat" expression ":" block ;

range_stmt      = "repeat" IDENT "in" expression ".." expression ":" block ;

for_each_stmt   = "for" "each" IDENT "in" expression ":" block ;

while_stmt      = "keep" "going" "while" expression ":" block ;

recipe_decl     = "recipe" IDENT "(" [ params ] ")" ":" block ;

params          = IDENT { "," IDENT } ;

give_back_stmt  = "give" "back" [ expression ] ;

blueprint_decl  = "blueprint" IDENT [ "comes" "from" IDENT ] ":" INDENT
                  { constructor | recipe_decl }
                  DEDENT ;

constructor     = "create" "(" [ params ] ")" ":" block ;

use_stmt        = "use" IDENT ;

try_stmt        = "try" ":" block "when" "wrong" ":" block ;

shout_stmt      = "shout" expression ;

expr_stmt       = expression ;

(* ===== Blocks ===== *)

block           = INDENT { statement NEWLINE } DEDENT ;

(* ===== Expressions (ordered by precedence, lowest first) ===== *)

expression      = or_expr ;

or_expr         = and_expr { "or" and_expr } ;

and_expr        = eq_expr { "and" eq_expr } ;

eq_expr         = comp_expr { ( "is" | "is" "not" ) comp_expr } ;

comp_expr       = add_expr { ( "<" | ">" | "<=" | ">=" ) add_expr } ;

add_expr        = mul_expr { ( "+" | "-" ) mul_expr } ;

mul_expr        = unary_expr { ( "*" | "/" | "%" ) unary_expr } ;

unary_expr      = ( "not" | "-" ) unary_expr
                | postfix_expr ;

postfix_expr    = primary { member_suffix | index_suffix | call_suffix } ;

member_suffix   = "." IDENT ;
index_suffix    = "[" expression "]" ;
call_suffix     = "(" [ args ] ")" ;

args            = expression { "," expression } ;

(* ===== Primary expressions ===== *)

primary         = INT_LIT
                | FLOAT_LIT
                | STRING_LIT
                | STRING_INTERP
                | "yes"
                | "no"
                | "empty"
                | IDENT
                | "me"
                | "comes_from"
                | ask_expr
                | bag_lit
                | notebook_lit
                | lambda_expr
                | "(" expression ")" ;

ask_expr        = "ask" expression ;

bag_lit         = "bag" "[" [ args ] "]" ;

notebook_lit    = "notebook" "[" [ nb_entries ] "]" ;
nb_entries      = IDENT ":" expression { "," IDENT ":" expression } ;

lambda_expr     = "recipe" "(" [ params ] ")" ":" ( block | statement ) ;

(* ===== Type annotations ===== *)

type            = "whole" | "decimal" | "text" | "truth" | "anything" ;

(* ===== Lexical terminals ===== *)

INT_LIT         = DIGIT { DIGIT } ;
FLOAT_LIT       = DIGIT { DIGIT } "." DIGIT { DIGIT } ;
STRING_LIT      = '"' { CHAR } '"' | "'" { CHAR } "'" ;
STRING_INTERP   = '"' { CHAR | "{" expression "}" } '"' ;
IDENT           = ( LETTER | "_" ) { LETTER | DIGIT | "_" } ;
NEWLINE         = "\n" ;
INDENT          = (* increase in leading whitespace *) ;
DEDENT          = (* decrease in leading whitespace *) ;
EOF             = (* end of input *) ;
```

---

### 13. Tier System

ILMA's three tiers define recommended feature subsets for different age groups.
The compiler accepts all features regardless of tier; tiers are a pedagogical
guideline only.

#### 13.1 Seed Tier (Ages 3--7)

Intended features:
- `say` --- output text and numbers
- `ask` --- read input
- `remember` --- declare variables
- String literals and integer literals
- Basic arithmetic: `+`, `-`, `*`
- String concatenation with `+`

**Example Seed program:**

```
say "Bismillah"
remember name = ask "What is your name? "
say "Assalamu Alaikum, " + name + "!"
say 2 + 3
```

#### 13.2 Sapling Tier (Ages 7--13)

All Seed features plus:
- `if` / `otherwise` conditionals
- `repeat`, `repeat...in`, `for each`, `keep going while` loops
- `recipe` declarations and calls
- `bag` collections and methods
- Comparison and logic operators
- `check` / `when` pattern matching

**Example Sapling program:**

```
recipe fizzbuzz(n):
    repeat i in 1..n:
        if i % 15 is 0:
            say "FizzBuzz"
        otherwise if i % 3 is 0:
            say "Fizz"
        otherwise if i % 5 is 0:
            say "Buzz"
        otherwise:
            say i

fizzbuzz(30)
```

#### 13.3 Tree Tier (Ages 13+)

All Sapling features plus:
- `blueprint` declarations with `create`, `me`, inheritance via `comes from`
- `notebook` collections
- `try` / `when wrong` / `shout` error handling
- `use` module imports (all 9 knowledge modules)
- Lambda expressions
- Higher-order functions (`map`, `filter`, `each`)
- File I/O (`read_file`, `write_file`, `file_exists`)
- String interpolation
- Text methods (`upper`, `lower`, `contains`, `slice`, `join`)

**Example Tree program:**

```
use science
use draw

blueprint Planet:
    create(name, mass_kg, radius_km):
        me.name = name
        me.mass = mass_kg
        me.radius = radius_km

    recipe gravity():
        give back science.gravity(me.mass)

    recipe describe():
        say "{me.name}: mass={me.mass}kg, surface gravity={me.gravity()}N"

remember earth = Planet("Earth", 5.97, 6371)
remember mars = Planet("Mars", 0.642, 3389)

remember planets = bag[earth, mars]
for each planet in planets:
    planet.describe()

remember canvas = draw.canvas(400, 300)
draw.circle(canvas, 200, 150, 50, "blue")
draw.text(canvas, 180, 155, "Earth", "white")
draw.save(canvas)
```

---

### 14. Error Reporting

The compiler and interpreter report errors with file name, line number, and a
descriptive message.

**Lexer errors:**
```
Error on line 5: unexpected character '@'
```

**Parser errors:**
```
Error on line 12: expected ':' after if condition
Error on line 8: expected expression after 'say'
```

**Runtime errors (interpreter / compiled program):**
```
Runtime error on line 15: division by zero
Runtime error on line 22: index 10 out of range (bag has 5 elements)
```

**Shout errors:**
```
Error on line 7: Cannot divide by zero!
```

---

### 15. Command-Line Interface

```
Usage: ilma [options] <file.ilma>

Options:
  --version        Print version and exit
  --help           Print usage information
  --tokens         Dump lexer tokens (debug)
  --ast            Dump AST (debug)
  --c-only         Generate C code but do not compile
  --eval           Interpret directly (do not compile to C)
  --repl           Start interactive REPL

Package management:
  ilma get <name>           Install a package
  ilma packages             List installed packages
  ilma packages --available List available packages
```

---

### 16. Platform Support

| Platform         | Status    | Install Method                        |
|------------------|-----------|---------------------------------------|
| Linux (x86_64)   | Supported | `curl -fsSL https://ilma-lang.dev/install.sh \| bash` |
| Linux (ARM64)    | Supported | Same as above                         |
| macOS (Apple Silicon) | Supported | Same as above (universal binary)  |
| macOS (Intel)    | Supported | Same as above (universal binary)      |
| Windows (x86_64) | Supported | `irm https://ilma-lang.dev/install.ps1 \| iex` |
| Web (WASM)       | Supported | Via the online playground at ilma-lang.dev |
| Homebrew (macOS) | Supported | `brew install raihan-js/tap/ilma`     |

---

### 17. Future Directions

The following features are planned but not yet specified:

- **Concurrency:** `together` blocks for parallel execution.
- **Pattern matching enhancements:** Destructuring bags and notebooks in `when` clauses.
- **Module system:** User-defined modules with `share` and `use` for multi-file projects.
- **Type checker:** Optional static type checking pass before code generation.
- **Standard library expansion:** Networking, JSON parsing, database access.
- **REPL improvements:** Multi-line editing, history, tab completion.

---

### Appendix A: Complete Keyword Table

| Keyword      | Purpose                        | Tier    |
|--------------|--------------------------------|---------|
| `say`        | Print output                   | Seed    |
| `ask`        | Read input                     | Seed    |
| `remember`   | Declare variable               | Seed    |
| `if`         | Conditional                    | Sapling |
| `otherwise`  | Else / else-if                 | Sapling |
| `repeat`     | Count or range loop            | Sapling |
| `keep`       | While loop (part 1)            | Sapling |
| `going`      | While loop (part 2)            | Sapling |
| `while`      | While loop (part 3)            | Sapling |
| `for`        | For-each loop (part 1)         | Sapling |
| `each`       | For-each loop (part 2)         | Sapling |
| `in`         | Loop iterator / range          | Sapling |
| `recipe`     | Function declaration           | Sapling |
| `give`       | Return (part 1)                | Sapling |
| `back`       | Return (part 2)                | Sapling |
| `check`      | Pattern match                  | Sapling |
| `when`       | Pattern match case / error handler | Sapling |
| `bag`        | Array literal                  | Sapling |
| `and`        | Logical AND                    | Sapling |
| `or`         | Logical OR                     | Sapling |
| `not`        | Logical NOT                    | Sapling |
| `is`         | Equality comparison            | Sapling |
| `yes`        | Boolean true                   | Seed    |
| `no`         | Boolean false                  | Seed    |
| `empty`      | Null value                     | Sapling |
| `blueprint`  | Class declaration              | Tree    |
| `create`     | Constructor                    | Tree    |
| `me`         | Self-reference                 | Tree    |
| `comes`      | Inheritance (part 1)           | Tree    |
| `from`       | Inheritance (part 2)           | Tree    |
| `comes_from` | Parent reference               | Tree    |
| `notebook`   | Hash map literal               | Tree    |
| `use`        | Module import                  | Tree    |
| `try`        | Error handling                 | Tree    |
| `wrong`      | Error handler body             | Tree    |
| `shout`      | Throw error                    | Tree    |

---

### Appendix B: Operator Quick Reference

| Operator | Name              | Example            | Result Type |
|----------|-------------------|--------------------|-------------|
| `+`      | Add / Concat      | `3 + 4`, `"a" + "b"` | whole/decimal/text |
| `-`      | Subtract / Negate | `10 - 3`, `-5`     | whole/decimal |
| `*`      | Multiply          | `6 * 7`            | whole/decimal |
| `/`      | Divide            | `10 / 3`           | whole/decimal |
| `%`      | Modulo            | `10 % 3`           | whole        |
| `is`     | Equals            | `x is 5`           | truth       |
| `is not` | Not equals        | `x is not 5`       | truth       |
| `<`      | Less than         | `x < 10`           | truth       |
| `>`      | Greater than      | `x > 10`           | truth       |
| `<=`     | Less or equal     | `x <= 10`          | truth       |
| `>=`     | Greater or equal  | `x >= 10`          | truth       |
| `and`    | Logical AND       | `a and b`          | truth       |
| `or`     | Logical OR        | `a or b`           | truth       |
| `not`    | Logical NOT       | `not a`            | truth       |
| `=`      | Assignment        | `x = 5`            | ---         |
| `..`     | Range             | `1..10`            | ---         |
| `.`      | Member access     | `obj.name`         | any         |
| `[]`     | Index access      | `bag[0]`           | any         |
| `()`     | Function call     | `f(x)`             | any         |

---

### Appendix C: Runtime Value Layout

The core runtime value is a tagged union in C:

```c
typedef struct IlmaValue {
    IlmaType type;       /* enum: WHOLE, DECIMAL, TEXT, TRUTH, BAG, NOTEBOOK, OBJECT, EMPTY */
    union {
        int64_t    as_whole;
        double     as_decimal;
        char*      as_text;
        int        as_truth;
        IlmaBag*   as_bag;
        IlmaBook*  as_notebook;
        IlmaObj*   as_object;
    };
} IlmaValue;
```

Bag is a dynamically-resizing array:

```c
struct IlmaBag {
    IlmaValue* items;
    int        count;
    int        capacity;
};
```

Notebook is an open-addressing hash map:

```c
struct IlmaBook {
    IlmaBookEntry* entries;
    int             capacity;
    int             count;
};
```

Object holds a blueprint name and a notebook of fields:

```c
struct IlmaObj {
    IlmaBook*   fields;
    const char* blueprint;
};
```

---

*End of ILMA Language Specification v0.5.0*
