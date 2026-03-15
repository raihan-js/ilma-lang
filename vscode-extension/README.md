# ILMA Language — VS Code Extension

Full language support for the [ILMA programming language](https://ilmalang.dev) in Visual Studio Code.

ILMA is a programming language designed for children ages 3+ that grows with them to adulthood. It uses original English keywords like `say`, `remember`, `recipe`, and `blueprint` to make programming intuitive and accessible.

## Features

### Syntax Highlighting

Complete TextMate grammar covering all ILMA keywords, including multi-word keywords like `give back`, `keep going while`, `for each`, `when wrong`, `otherwise if`, and `comes from`.

### Snippets

Quickly scaffold common patterns with built-in snippets:

| Prefix | Description |
|--------|-------------|
| `hello` | Hello World program with greeting |
| `recipe` | Function definition with return |
| `blueprint` | Class with constructor and method |
| `if` | If / otherwise conditional |
| `repeat` | Repeat N times loop |
| `foreach` | For each item in collection |
| `while` | Keep going while loop |
| `try` | Try / when wrong error handling |
| `use-finance` | Finance module with compound interest |
| `use-time` | Time module example |

### Run Command

Run ILMA files directly from the editor:

- Click the **Run ILMA** button in the status bar
- Press **Ctrl+F5** (or **Cmd+F5** on macOS)
- Right-click and select **Run ILMA File**

The extension runs `ilma <file>` in the integrated terminal.

## Installation

### From the Marketplace

Search for **ILMA Language** in the VS Code Extensions panel, or run:

```
ext install ilmalang.ilma-language
```

### Manual Install via VSIX

1. Download the `.vsix` file from the [releases page](https://github.com/raihan-js/ilma-lang/releases).
2. Install it:

```bash
code --install-extension ilma-language-0.1.0.vsix
```

Or in VS Code: **Extensions** > **...** menu > **Install from VSIX...**

## Requirements

To run ILMA files, you need the ILMA compiler installed on your system. Install it from [ilmalang.dev/install](https://ilmalang.dev/install) or via:

```bash
curl -fsSL https://ilmalang.dev/install.sh | bash
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+F5` / `Cmd+F5` | Run current ILMA file |

## File Association

The extension automatically associates `.ilma` files with the ILMA language mode, providing syntax highlighting, snippets, and language-specific features.

## Links

- [ILMA Website](https://ilmalang.dev)
- [GitHub Repository](https://github.com/raihan-js/ilma-lang)
- [ILMA Documentation](https://ilmalang.dev/docs)
- [Report Issues](https://github.com/raihan-js/ilma-lang/issues)

## License

MIT
