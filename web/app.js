// ILMA Web Platform — Main Application

let editor = null;
let interpreter = null;
let currentLesson = null;
let completedLessons = JSON.parse(localStorage.getItem('ilma_completed') || '[]');

// ── Monaco Editor Setup ────────────────────────────
require.config({ paths: { vs: 'https://cdn.jsdelivr.net/npm/monaco-editor@0.45.0/min/vs' } });

require(['vs/editor/editor.main'], function () {
    // Register ILMA language
    monaco.languages.register({ id: 'ilma' });

    monaco.languages.setMonarchTokensProvider('ilma', {
        keywords: [
            'say', 'ask', 'remember', 'if', 'otherwise', 'repeat',
            'keep', 'going', 'while', 'for', 'each', 'in',
            'recipe', 'give', 'back', 'blueprint', 'create', 'me',
            'comes', 'from', 'comes_from', 'use', 'shout',
            'try', 'when', 'wrong'
        ],
        literals: ['yes', 'no', 'empty'],
        builtins: ['bag', 'notebook', 'and', 'or', 'not', 'is'],
        operators: ['+', '-', '*', '/', '%', '=', '>', '<', '>=', '<='],

        tokenizer: {
            root: [
                [/#.*$/, 'comment'],
                [/"[^"]*"/, 'string'],
                [/'[^']*'/, 'string'],
                [/\b\d+(\.\d+)?\b/, 'number'],
                [/[a-zA-Z_\u0600-\u06FF][\w\u0600-\u06FF]*/, {
                    cases: {
                        '@keywords': 'keyword',
                        '@literals': 'constant',
                        '@builtins': 'type',
                        '@default': 'identifier'
                    }
                }],
                [/[+\-*/%=><]/, 'operator'],
                [/[()[\]:.,]/, 'delimiter'],
            ]
        }
    });

    // Define ILMA theme
    monaco.editor.defineTheme('ilma-dark', {
        base: 'vs-dark',
        inherit: true,
        rules: [
            { token: 'keyword', foreground: 'C792EA', fontStyle: 'bold' },
            { token: 'string', foreground: 'C3E88D' },
            { token: 'number', foreground: 'F78C6C' },
            { token: 'comment', foreground: '546E7A', fontStyle: 'italic' },
            { token: 'constant', foreground: 'FF5370' },
            { token: 'type', foreground: 'FFCB6B' },
            { token: 'identifier', foreground: 'EEFFFF' },
            { token: 'operator', foreground: '89DDFF' },
            { token: 'delimiter', foreground: '89DDFF' },
        ],
        colors: {
            'editor.background': '#1e2130',
            'editor.foreground': '#EEFFFF',
            'editor.lineHighlightBackground': '#23263a',
            'editorLineNumber.foreground': '#3d4158',
            'editorLineNumber.activeForeground': '#8b8fa3',
            'editor.selectionBackground': '#3d4f7f55',
            'editorCursor.foreground': '#FFCB6B',
        }
    });

    // Create editor
    editor = monaco.editor.create(document.getElementById('editor-container'), {
        value: getDefaultCode(),
        language: 'ilma',
        theme: 'ilma-dark',
        fontSize: 14,
        fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
        lineNumbers: 'on',
        minimap: { enabled: false },
        scrollBeyondLastLine: false,
        wordWrap: 'on',
        tabSize: 4,
        insertSpaces: true,
        automaticLayout: true,
        renderLineHighlight: 'line',
        padding: { top: 10 },
        suggest: { showWords: false },
    });

    // Ctrl+Enter to run
    editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter, runProgram);

    // ILMA autocompletion
    monaco.languages.registerCompletionItemProvider('ilma', {
        provideCompletionItems: (model, position) => {
            const word = model.getWordUntilPosition(position);
            const range = { startLineNumber: position.lineNumber, endLineNumber: position.lineNumber, startColumn: word.startColumn, endColumn: word.endColumn };
            const k = monaco.languages.CompletionItemKind;
            const items = [
                // Keywords
                ...[
                    ['say', 'say "${1:text}"', 'Print text to the screen'],
                    ['remember', 'remember ${1:name} = ${2:value}', 'Create a variable'],
                    ['ask', 'ask "${1:question}"', 'Ask the user for input'],
                    ['if', 'if ${1:condition}:\n    ${2:code}', 'Check a condition'],
                    ['otherwise', 'otherwise:\n    ${1:code}', 'Else branch'],
                    ['otherwise if', 'otherwise if ${1:condition}:\n    ${2:code}', 'Else-if branch'],
                    ['repeat', 'repeat ${1:5}:\n    ${2:code}', 'Repeat N times'],
                    ['keep going while', 'keep going while ${1:condition}:\n    ${2:code}', 'While loop'],
                    ['for each', 'for each ${1:item} in ${2:collection}:\n    ${3:code}', 'Loop over items'],
                    ['recipe', 'recipe ${1:name}(${2:params}):\n    ${3:code}', 'Define a function'],
                    ['give back', 'give back ${1:value}', 'Return a value'],
                    ['blueprint', 'blueprint ${1:Name}:\n\n    create(${2:params}):\n        me.${3:field} = ${4:value}\n\n    recipe ${5:method}():\n        ${6:code}', 'Define a class'],
                    ['try', 'try:\n    ${1:code}\nwhen wrong:\n    ${2:handle error}', 'Error handling'],
                    ['shout', 'shout "${1:error message}"', 'Raise an error'],
                    ['use', 'use ${1:module}', 'Import a module'],
                    ['bag', 'bag[${1:items}]', 'Create a list'],
                    ['notebook', 'notebook[${1:key}: ${2:value}]', 'Create a key-value store'],
                ].map(([label, insert, doc]) => ({
                    label, kind: k.Keyword, insertText: insert,
                    insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
                    documentation: doc, range
                })),
                // Modules
                ...[
                    ['finance.compound', 'finance.compound(${1:principal}, ${2:rate}, ${3:years})', 'Calculate compound interest'],
                    ['finance.zakat', 'finance.zakat(${1:wealth}, ${2:nisab})', 'Calculate zakat obligation'],
                    ['finance.profit', 'finance.profit(${1:cost}, ${2:revenue})', 'Calculate profit'],
                    ['finance.budget', 'finance.budget(${1:income})', 'Generate 50/30/20 budget'],
                    ['time.today', 'time.today()', 'Get today\'s date'],
                    ['time.to_hijri', 'time.to_hijri(${1:date})', 'Convert to Hijri calendar'],
                    ['quran.ayah_of_the_day', 'quran.ayah_of_the_day()', 'Get daily ayah'],
                    ['quran.search', 'quran.search("${1:word}")', 'Search Quran translations'],
                    ['number.to_binary', 'number.to_binary(${1:n})', 'Convert to binary'],
                    ['number.fibonacci', 'number.fibonacci(${1:n})', 'Get fibonacci sequence'],
                    ['science.gravity_fall', 'science.gravity_fall(${1:height_m})', 'Calculate fall time'],
                    ['body.bmi', 'body.bmi(${1:weight_kg}, ${2:height_cm})', 'Calculate BMI'],
                ].map(([label, insert, doc]) => ({
                    label, kind: k.Function, insertText: insert,
                    insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
                    documentation: doc, range
                })),
            ];
            return { suggestions: items };
        }
    });

    // Setup complete
    renderLessonList();
    loadFromURL();
    document.getElementById('status-text').textContent = 'Ready';
});

function getDefaultCode() {
    return `# Welcome to ILMA!
# Click "Run" or press Ctrl+Enter to run your program.

say "Bismillah"
say "Hello, World!"

remember name = "ILMA"
say name + " is a programming language for everyone."

repeat 3:
    say "SubhanAllah"
`;
}

// ── Output Console ─────────────────────────────────
const outputConsole = document.getElementById('output-console');

function appendOutput(text, isError = false) {
    // Check if the output is SVG (from draw module)
    if (typeof text === 'string' && text.startsWith('<svg')) {
        const div = document.createElement('div');
        div.innerHTML = text;
        div.style.margin = '8px 0';
        outputConsole.appendChild(div);
    } else {
        const span = document.createElement('span');
        span.textContent = text + '\n';
        if (isError) span.className = 'error';
        outputConsole.appendChild(span);
    }
    outputConsole.scrollTop = outputConsole.scrollHeight;
}

function clearOutput() {
    outputConsole.innerHTML = '';
}

// ── Ask (input) handling ───────────────────────────
function handleAsk(prompt) {
    return new Promise((resolve) => {
        appendOutput(prompt);
        const inputBar = document.getElementById('input-bar');
        const inputField = document.getElementById('input-field');
        const inputSubmit = document.getElementById('input-submit');

        inputBar.classList.remove('hidden');
        inputField.value = '';
        inputField.focus();

        const submit = () => {
            const value = inputField.value;
            inputBar.classList.add('hidden');
            appendOutput(value);
            inputField.removeEventListener('keydown', onKey);
            inputSubmit.removeEventListener('click', submit);
            resolve(value);
        };

        const onKey = (e) => { if (e.key === 'Enter') submit(); };
        inputField.addEventListener('keydown', onKey);
        inputSubmit.addEventListener('click', submit);
    });
}

// ── Run/Stop ───────────────────────────────────────
async function runProgram() {
    clearOutput();
    lastError = null;
    document.getElementById('btn-run').classList.add('hidden');
    document.getElementById('btn-stop').classList.remove('hidden');
    document.getElementById('status-text').textContent = 'Running...';

    interpreter = new IlmaInterpreter((text, isError) => {
        appendOutput(text, isError);
        if (isError) lastError = text;
    }, handleAsk);

    const code = editor.getValue();
    await interpreter.run(code);

    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    document.getElementById('status-text').textContent = lastError ? 'Error — ask the tutor!' : 'Done';
}

function stopProgram() {
    if (interpreter) interpreter.stop();
    document.getElementById('btn-run').classList.remove('hidden');
    document.getElementById('btn-stop').classList.add('hidden');
    document.getElementById('status-text').textContent = 'Stopped';
}

// ── Lessons ────────────────────────────────────────
function renderLessonList() {
    const list = document.getElementById('lesson-list');
    list.innerHTML = '';
    LESSONS.forEach((lesson) => {
        const div = document.createElement('div');
        div.className = 'lesson-item' +
            (completedLessons.includes(lesson.id) ? ' completed' : '') +
            (currentLesson === lesson.id ? ' active' : '');
        div.innerHTML = `
            <div class="lesson-num">${completedLessons.includes(lesson.id) ? '&#10003;' : lesson.id}</div>
            <div class="lesson-info">
                <div class="title">${lesson.title}</div>
                <div class="subtitle">${lesson.subtitle}</div>
            </div>
        `;
        div.addEventListener('click', () => openLesson(lesson.id));
        list.appendChild(div);
    });
}

function openLesson(id) {
    const lesson = LESSONS.find(l => l.id === id);
    if (!lesson) return;

    currentLesson = id;
    document.getElementById('lesson-list').classList.add('hidden');
    document.getElementById('lesson-content').classList.remove('hidden');
    document.getElementById('lesson-title').textContent = `${lesson.id}. ${lesson.title}`;
    document.getElementById('lesson-body').innerHTML = lesson.body;

    // Load starter code
    if (editor) editor.setValue(lesson.code.trim());

    // Nav buttons
    document.getElementById('btn-prev-lesson').disabled = (id <= 1);
    document.getElementById('btn-next-lesson').disabled = (id >= LESSONS.length);

    renderLessonList();
}

function closeLessonContent() {
    document.getElementById('lesson-list').classList.remove('hidden');
    document.getElementById('lesson-content').classList.add('hidden');
    currentLesson = null;
    renderLessonList();
}

function markLessonComplete(id) {
    if (!completedLessons.includes(id)) {
        completedLessons.push(id);
        localStorage.setItem('ilma_completed', JSON.stringify(completedLessons));
        renderLessonList();
    }
}

// ── Event Listeners ────────────────────────────────
document.getElementById('btn-run').addEventListener('click', runProgram);
document.getElementById('btn-stop').addEventListener('click', stopProgram);
document.getElementById('btn-clear').addEventListener('click', clearOutput);
document.getElementById('btn-back-to-list').addEventListener('click', closeLessonContent);

document.getElementById('btn-prev-lesson').addEventListener('click', () => {
    if (currentLesson > 1) openLesson(currentLesson - 1);
});

document.getElementById('btn-next-lesson').addEventListener('click', () => {
    if (currentLesson && currentLesson < LESSONS.length) {
        markLessonComplete(currentLesson);
        openLesson(currentLesson + 1);
    }
});

document.getElementById('btn-toggle-sidebar').addEventListener('click', () => {
    document.getElementById('sidebar').classList.toggle('collapsed');
});

// ── Share ──────────────────────────────────────────
document.getElementById('btn-share').addEventListener('click', () => {
    if (!editor) return;
    const code = editor.getValue();
    const encoded = btoa(unescape(encodeURIComponent(code)));
    const url = window.location.origin + window.location.pathname + '#code=' + encoded;
    navigator.clipboard.writeText(url).then(() => {
        document.getElementById('status-text').textContent = 'Link copied!';
        setTimeout(() => { document.getElementById('status-text').textContent = ''; }, 3000);
    }).catch(() => {
        prompt('Copy this link to share your program:', url);
    });
});

// Load shared program from URL
function loadFromURL() {
    const hash = window.location.hash;
    if (hash.startsWith('#code=')) {
        try {
            const encoded = hash.slice(6);
            const code = decodeURIComponent(escape(atob(encoded)));
            if (editor) editor.setValue(code);
            else setTimeout(() => { if (editor) editor.setValue(code); }, 1000);
        } catch(e) { /* ignore bad URLs */ }
    }
}
window.addEventListener('hashchange', loadFromURL);

// ── Tutor ──────────────────────────────────────────
const tutor = new IlmaTutor();
let lastError = null;

// Override appendOutput to capture errors
const origAppendOutput = appendOutput;

document.getElementById('btn-tutor').addEventListener('click', () => {
    document.getElementById('tutor-panel').classList.toggle('hidden');
});

document.getElementById('btn-close-tutor').addEventListener('click', () => {
    document.getElementById('tutor-panel').classList.add('hidden');
});

document.getElementById('btn-hint').addEventListener('click', () => {
    const code = editor ? editor.getValue() : '';
    const hint = tutor.analyze(code, lastError);
    const msgs = document.getElementById('tutor-messages');
    const div = document.createElement('div');
    div.className = 'tutor-msg';
    div.innerHTML = `<div class="tutor-title">${hint.title}</div><div class="tutor-body">${hint.message}</div>`;
    msgs.appendChild(div);
    msgs.scrollTop = msgs.scrollHeight;
});

// Handle window resize for Monaco
window.addEventListener('resize', () => {
    if (editor) editor.layout();
});
