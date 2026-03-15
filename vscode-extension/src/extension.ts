import * as vscode from 'vscode';

let statusBarItem: vscode.StatusBarItem;
let terminal: vscode.Terminal | undefined;

export function activate(context: vscode.ExtensionContext): void {
    // Register the run command
    const runCommand = vscode.commands.registerCommand('ilma.runFile', () => {
        runIlmaFile();
    });
    context.subscriptions.push(runCommand);

    // Create status bar item on the left side
    statusBarItem = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        100
    );
    statusBarItem.text = '$(play) Run ILMA';
    statusBarItem.tooltip = 'Run current ILMA file (Ctrl+F5)';
    statusBarItem.command = 'ilma.runFile';
    context.subscriptions.push(statusBarItem);

    // Show/hide status bar based on active editor language
    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(updateStatusBarVisibility)
    );

    // Set initial visibility
    updateStatusBarVisibility(vscode.window.activeTextEditor);
}

function updateStatusBarVisibility(editor: vscode.TextEditor | undefined): void {
    if (editor && editor.document.languageId === 'ilma') {
        statusBarItem.show();
    } else {
        statusBarItem.hide();
    }
}

function getOrCreateTerminal(): vscode.Terminal {
    // Reuse existing terminal if it hasn't been disposed
    if (terminal) {
        const terminals = vscode.window.terminals;
        if (terminals.includes(terminal)) {
            return terminal;
        }
    }
    terminal = vscode.window.createTerminal('ILMA');
    return terminal;
}

function runIlmaFile(): void {
    const editor = vscode.window.activeTextEditor;

    if (!editor) {
        vscode.window.showErrorMessage('No active file to run.');
        return;
    }

    if (editor.document.languageId !== 'ilma') {
        vscode.window.showErrorMessage('The active file is not an ILMA file (.ilma).');
        return;
    }

    // Save the file before running
    editor.document.save().then((saved) => {
        if (!saved) {
            vscode.window.showWarningMessage('Could not save file before running.');
            return;
        }

        const filePath = editor.document.uri.fsPath;
        const term = getOrCreateTerminal();
        term.show(true);

        // Run the ILMA file; shell will report "command not found" if ilma is missing
        term.sendText(`ilma "${filePath}" || echo "\\n[ILMA] If 'ilma' is not found, install it from https://ilmalang.dev/install"`);
    });
}

export function deactivate(): void {
    if (statusBarItem) {
        statusBarItem.dispose();
    }
    if (terminal) {
        terminal.dispose();
    }
}
