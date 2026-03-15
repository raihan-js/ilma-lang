// ILMA In-Browser Interpreter
// Supports Tier 1 and Tier 2 features for the web platform

class IlmaInterpreter {
    constructor(outputFn, askFn) {
        this.output = outputFn;   // function(text) — display output
        this.askFn = askFn;       // function(prompt) → Promise<string>
        this.globals = {};
        this.recipes = {};
        this.modules = {};
        this.blueprints = {};
        this.stopped = false;
        this.stepCount = 0;
        this.maxSteps = 100000;
        this._initModules();
    }

    stop() { this.stopped = true; }

    // ── Standard Library Modules ───────────────────────
    _initModules() {
        // finance module
        this.modules.finance = {
            _type: 'module',
            compound: (principal, rate, years) => {
                const p = typeof principal === 'number' ? principal : 0;
                const r = typeof rate === 'number' ? rate : 0;
                const y = typeof years === 'number' ? years : 0;
                return Math.round(p * Math.pow(1 + r, y) * 100) / 100;
            },
            zakat: (wealth, nisab) => {
                const w = typeof wealth === 'number' ? wealth : 0;
                const n = typeof nisab === 'number' ? nisab : 595;
                if (w < n) return 0;
                return Math.round(w * 0.025 * 100) / 100;
            },
            profit: (cost, revenue) => {
                const c = typeof cost === 'number' ? cost : 0;
                const r = typeof revenue === 'number' ? revenue : 0;
                return r - c;
            },
            margin: (cost, revenue) => {
                const c = typeof cost === 'number' ? cost : 0;
                const r = typeof revenue === 'number' ? revenue : 0;
                if (r === 0) return 0;
                return Math.round((r - c) / r * 100 * 100) / 100;
            },
            budget: (income) => {
                const i = typeof income === 'number' ? income : 0;
                return {
                    _type: 'notebook',
                    entries: {
                        needs: Math.round(i * 0.50 * 100) / 100,
                        savings: Math.round(i * 0.30 * 100) / 100,
                        wants: Math.round(i * 0.20 * 100) / 100
                    }
                };
            }
        };

        // time module
        this.modules.time = {
            _type: 'module',
            today: () => {
                const d = new Date();
                return d.getFullYear() + '-' +
                    String(d.getMonth() + 1).padStart(2, '0') + '-' +
                    String(d.getDate()).padStart(2, '0');
            },
            to_hijri: (gregorian) => {
                // Approximate Hijri conversion
                const parts = String(gregorian).split('-').map(Number);
                if (parts.length < 3) return 'unknown';
                const jd = Math.floor(365.25 * (parts[0] + 4716)) +
                           Math.floor(30.6001 * (parts[1] + 1)) + parts[2] - 1524.5;
                const l = Math.floor(jd - 1948439.5 + 10632);
                const n = Math.floor((l - 1) / 10631);
                const ll = l - 10631 * n + 354;
                const j = Math.floor((10985 - ll) / 5316) * Math.floor((50 * ll) / 17719) +
                          Math.floor(ll / 5670) * Math.floor((43 * ll) / 15238);
                const ld = ll - Math.floor((30 - j) / 15) * Math.floor((17719 * j) / 50) -
                           Math.floor(j / 16) * Math.floor((15238 * j) / 43) + 29;
                const month = Math.floor((24 * ld) / 709);
                const day = ld - Math.floor((709 * month) / 24);
                const year = 30 * n + j - 30;
                return year + '-' + String(month).padStart(2, '0') + '-' + String(day).padStart(2, '0');
            },
            days_between: (d1, d2) => {
                const parse = (s) => { const p = String(s).split('-').map(Number); return new Date(p[0], p[1]-1, p[2]); };
                return Math.round(Math.abs(parse(d2) - parse(d1)) / 86400000);
            },
            year: () => new Date().getFullYear(),
            month: () => new Date().getMonth() + 1,
            day: () => new Date().getDate(),
        };

        // think module
        this.modules.think = {
            _type: 'module',
            stoic_question: () => {
                const questions = [
                    "What is one thing today that is within your control?",
                    "What would a wise person do in your situation?",
                    "Is this problem going to matter in five years?",
                    "What are you grateful for right now?",
                    "What is the most important thing you can do today?",
                    "Are you reacting, or are you choosing?",
                ];
                return questions[Math.floor(Math.random() * questions.length)];
            }
        };

        // body module
        this.modules.body = {
            _type: 'module',
            bmi: (weight_kg, height_cm) => {
                const h = height_cm / 100;
                return Math.round(weight_kg / (h * h) * 10) / 10;
            },
            bmi_category: (bmi) => {
                if (bmi < 18.5) return "underweight";
                if (bmi < 25) return "healthy";
                if (bmi < 30) return "overweight";
                return "obese";
            },
            daily_water: (weight_kg) => Math.round(weight_kg * 0.033 * 10) / 10,
        };

        // quran module — selected ayat for educational use
        const quranData = [
            { surah: "Al-Alaq", number: 1, arabic: "اقْرَأْ بِاسْمِ رَبِّكَ الَّذِي خَلَقَ", translation: "Read in the name of your Lord who created" },
            { surah: "Al-Baqarah", number: 286, arabic: "لَا يُكَلِّفُ اللَّهُ نَفْسًا إِلَّا وُسْعَهَا", translation: "Allah does not burden a soul beyond that it can bear" },
            { surah: "Al-Inshirah", number: 5, arabic: "فَإِنَّ مَعَ الْعُسْرِ يُسْرًا", translation: "Indeed, with hardship comes ease" },
            { surah: "Al-Hujurat", number: 13, arabic: "إِنَّ أَكْرَمَكُمْ عِندَ اللَّهِ أَتْقَاكُمْ", translation: "The most noble of you in the sight of Allah is the most righteous" },
            { surah: "Al-Asr", number: 1, arabic: "وَالْعَصْرِ إِنَّ الْإِنسَانَ لَفِي خُسْرٍ", translation: "By time, indeed mankind is in loss" },
            { surah: "Al-Ikhlas", number: 1, arabic: "قُلْ هُوَ اللَّهُ أَحَدٌ", translation: "Say: He is Allah, the One" },
            { surah: "Al-Fatiha", number: 1, arabic: "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ", translation: "In the name of Allah, the Most Gracious, the Most Merciful" },
            { surah: "Ar-Rahman", number: 13, arabic: "فَبِأَيِّ آلَاءِ رَبِّكُمَا تُكَذِّبَانِ", translation: "So which of the favours of your Lord would you deny?" },
            { surah: "Al-Baqarah", number: 255, arabic: "اللَّهُ لَا إِلَٰهَ إِلَّا هُوَ الْحَيُّ الْقَيُّومُ", translation: "Allah — there is no deity except Him, the Ever-Living" },
            { surah: "Al-Imran", number: 139, arabic: "وَلَا تَهِنُوا وَلَا تَحْزَنُوا وَأَنتُمُ الْأَعْلَوْنَ", translation: "Do not weaken and do not grieve, for you are superior" },
        ];

        this.modules.quran = {
            _type: 'module',
            ayah_of_the_day: () => {
                const idx = new Date().getDate() % quranData.length;
                const a = quranData[idx];
                return { _type: 'notebook', entries: { surah: a.surah, number: a.number, arabic: a.arabic, translation: a.translation } };
            },
            search: (term) => {
                const t = String(term).toLowerCase();
                const results = quranData.filter(a => a.translation.toLowerCase().includes(t));
                return { _type: 'bag', items: results.map(a => ({ _type: 'notebook', entries: { surah: a.surah, number: a.number, arabic: a.arabic, translation: a.translation } })) };
            },
            surah: (name) => {
                const a = quranData.find(x => x.surah.toLowerCase() === String(name).toLowerCase());
                if (!a) return null;
                return { _type: 'notebook', entries: { surah: a.surah, number: a.number, arabic: a.arabic, translation: a.translation } };
            },
        };

        // trade module
        this.modules.trade = {
            _type: 'module',
            profit: (cost, revenue) => revenue - cost,
            margin: (cost, revenue) => revenue === 0 ? 0 : Math.round((revenue - cost) / revenue * 100 * 100) / 100,
            halal_check: (involves_interest, involves_gambling) => {
                const permissible = !involves_interest && !involves_gambling;
                return { _type: 'notebook', entries: { permissible: permissible, reason: permissible ? "This trade is halal" : "This trade involves prohibited elements" } };
            },
            supply_demand: (product, demand_change) => {
                const base_price = 10;
                const change = typeof demand_change === 'number' ? demand_change : 0;
                const new_price = Math.round(base_price * (1 + change / 100) * 100) / 100;
                return { _type: 'notebook', entries: { product: String(product), base_price: base_price, demand_change: change, new_price: new_price } };
            },
        };

        // number module — number systems
        this.modules.number = {
            _type: 'module',
            to_binary: (n) => (typeof n === 'number' ? (n >>> 0).toString(2) : '0'),
            to_hex: (n) => (typeof n === 'number' ? n.toString(16).toUpperCase() : '0'),
            to_roman: (n) => {
                if (typeof n !== 'number' || n <= 0 || n > 3999) return '?';
                const vals = [1000,900,500,400,100,90,50,40,10,9,5,4,1];
                const syms = ['M','CM','D','CD','C','XC','L','XL','X','IX','V','IV','I'];
                let result = '', num = Math.floor(n);
                for (let i = 0; i < vals.length; i++) {
                    while (num >= vals[i]) { result += syms[i]; num -= vals[i]; }
                }
                return result;
            },
            from_binary: (s) => parseInt(String(s), 2) || 0,
            from_hex: (s) => parseInt(String(s), 16) || 0,
            is_prime: (n) => {
                if (typeof n !== 'number' || n < 2) return false;
                for (let i = 2; i * i <= n; i++) { if (n % i === 0) return false; }
                return true;
            },
            fibonacci: (n) => {
                if (typeof n !== 'number' || n <= 0) return { _type: 'bag', items: [] };
                const fib = [0, 1];
                for (let i = 2; i < n; i++) fib.push(fib[i-1] + fib[i-2]);
                return { _type: 'bag', items: fib.slice(0, n) };
            },
        };

        // science module
        this.modules.science = {
            _type: 'module',
            gravity_fall: (height_m) => {
                // time = sqrt(2h/g), g=9.81
                const t = Math.sqrt(2 * height_m / 9.81);
                return Math.round(t * 100) / 100;
            },
            speed_of_light: () => 299792458,
            celsius_to_fahrenheit: (c) => Math.round((c * 9/5 + 32) * 100) / 100,
            fahrenheit_to_celsius: (f) => Math.round((f - 32) * 5/9 * 100) / 100,
            kinetic_energy: (mass, velocity) => Math.round(0.5 * mass * velocity * velocity * 100) / 100,
            distance: (speed, time) => Math.round(speed * time * 100) / 100,
            pi: () => Math.PI,
        };

        // draw module — produces SVG rendering
        this.modules.draw = {
            _type: 'module',
            canvas: (width, height) => {
                const w = width || 400, h = height || 400;
                return {
                    _type: 'object', _blueprint: 'Canvas',
                    _fields: { width: w, height: h },
                    _shapes: [],
                    _w: w, _h: h,
                };
            },
            islamic_star: (points, size) => {
                // returns an SVG string for an islamic star
                const n = points || 8, s = size || 100;
                let path = '';
                for (let i = 0; i < n * 2; i++) {
                    const angle = (i * Math.PI) / n - Math.PI/2;
                    const r = i % 2 === 0 ? s : s * 0.4;
                    const x = Math.round(Math.cos(angle) * r);
                    const y = Math.round(Math.sin(angle) * r);
                    path += (i === 0 ? 'M' : 'L') + x + ',' + y;
                }
                path += 'Z';
                return { _type: 'notebook', entries: { type: 'star', path, points: n, size: s } };
            },
        };
    }

    checkStop() {
        if (this.stopped) throw new IlmaStop();
        if (++this.stepCount > this.maxSteps) throw new IlmaError("Program took too long. Infinite loop?", 0);
    }

    // ── Lexer ──────────────────────────────────────────
    tokenize(source) {
        const tokens = [];
        const lines = source.split('\n');
        const indentStack = [0];
        const keywords = new Set([
            'say', 'ask', 'remember', 'if', 'otherwise', 'repeat',
            'keep', 'going', 'while', 'for', 'each', 'in',
            'recipe', 'give', 'back', 'blueprint', 'create', 'me',
            'comes', 'from', 'comes_from', 'use', 'shout',
            'try', 'when', 'wrong', 'yes', 'no', 'empty',
            'and', 'or', 'not', 'is', 'bag', 'notebook'
        ]);

        for (let lineNum = 0; lineNum < lines.length; lineNum++) {
            const line = lines[lineNum];
            const stripped = line.replace(/^\s*#.*$/, '');  // remove comments
            if (stripped.trim() === '') continue;

            // Indentation
            const indent = line.search(/\S/);
            if (indent < 0) continue;

            const currentIndent = indentStack[indentStack.length - 1];
            if (indent > currentIndent) {
                indentStack.push(indent);
                tokens.push({ type: 'INDENT', line: lineNum + 1 });
            } else {
                while (indentStack.length > 1 && indent < indentStack[indentStack.length - 1]) {
                    indentStack.pop();
                    tokens.push({ type: 'DEDENT', line: lineNum + 1 });
                }
            }

            // Tokenize the line content
            let pos = indent;
            while (pos < line.length) {
                const ch = line[pos];
                if (ch === '#') break;  // comment
                if (ch === ' ' || ch === '\t') { pos++; continue; }

                // String
                if (ch === '"' || ch === "'") {
                    const quote = ch;
                    let str = '';
                    pos++;
                    while (pos < line.length && line[pos] !== quote) {
                        if (line[pos] === '\\' && pos + 1 < line.length) {
                            pos++;
                            const esc = { n: '\n', t: '\t', '\\': '\\', '"': '"', "'": "'" };
                            str += esc[line[pos]] || line[pos];
                        } else {
                            str += line[pos];
                        }
                        pos++;
                    }
                    pos++; // closing quote
                    tokens.push({ type: 'STRING', value: str, line: lineNum + 1 });
                    continue;
                }

                // Number
                if (/\d/.test(ch)) {
                    let num = '';
                    while (pos < line.length && /[\d.]/.test(line[pos])) {
                        num += line[pos++];
                    }
                    tokens.push({ type: 'NUMBER', value: num.includes('.') ? parseFloat(num) : parseInt(num), line: lineNum + 1 });
                    continue;
                }

                // Identifier / keyword
                if (/[a-zA-Z_\u0600-\u06FF\u0750-\u077F]/.test(ch)) {
                    let word = '';
                    while (pos < line.length && /[a-zA-Z0-9_\u0600-\u06FF\u0750-\u077F]/.test(line[pos])) {
                        word += line[pos++];
                    }

                    // Handle "is not"
                    if (word === 'is') {
                        let saved = pos;
                        while (pos < line.length && line[pos] === ' ') pos++;
                        if (line.substr(pos, 3) === 'not' && (pos + 3 >= line.length || !/[a-zA-Z0-9_]/.test(line[pos + 3]))) {
                            pos += 3;
                            tokens.push({ type: 'IS_NOT', line: lineNum + 1 });
                            continue;
                        }
                        pos = saved;
                    }

                    if (keywords.has(word)) {
                        tokens.push({ type: word.toUpperCase(), value: word, line: lineNum + 1 });
                    } else {
                        tokens.push({ type: 'IDENT', value: word, line: lineNum + 1 });
                    }
                    continue;
                }

                // Operators
                const twoChar = line.substr(pos, 2);
                if (['>=', '<=', '!='].includes(twoChar)) {
                    tokens.push({ type: twoChar, line: lineNum + 1 });
                    pos += 2;
                    continue;
                }

                const ops = { '+': 'PLUS', '-': 'MINUS', '*': 'STAR', '/': 'SLASH', '%': 'PERCENT',
                              '=': 'ASSIGN', ':': 'COLON', '.': 'DOT', ',': 'COMMA',
                              '(': 'LPAREN', ')': 'RPAREN', '[': 'LBRACKET', ']': 'RBRACKET',
                              '<': 'LT', '>': 'GT' };
                if (ops[ch]) {
                    tokens.push({ type: ops[ch], line: lineNum + 1 });
                    pos++;
                    continue;
                }

                pos++; // skip unknown
            }
            tokens.push({ type: 'NEWLINE', line: lineNum + 1 });
        }

        // Final dedents
        while (indentStack.length > 1) {
            indentStack.pop();
            tokens.push({ type: 'DEDENT', line: lines.length });
        }
        tokens.push({ type: 'EOF', line: lines.length });
        return tokens;
    }

    // ── Parser ─────────────────────────────────────────
    parse(tokens) {
        let pos = 0;
        const current = () => tokens[pos] || { type: 'EOF' };
        const peek = () => tokens[pos + 1] || { type: 'EOF' };
        const at = (t) => current().type === t;
        const advance = () => tokens[pos++];
        const expect = (t) => {
            if (!at(t)) throw new IlmaError(`Expected ${t} but found ${current().type}`, current().line);
            return advance();
        };
        const skipNL = () => { while (at('NEWLINE')) advance(); };

        const parseExpr = () => parseOr();

        const parseOr = () => {
            let left = parseAnd();
            while (at('OR')) { advance(); const right = parseAnd(); left = { type: 'binop', op: 'or', left, right }; }
            return left;
        };

        const parseAnd = () => {
            let left = parseEquality();
            while (at('AND')) { advance(); const right = parseEquality(); left = { type: 'binop', op: 'and', left, right }; }
            return left;
        };

        const parseEquality = () => {
            let left = parseComparison();
            while (at('IS') || at('IS_NOT')) {
                const op = at('IS') ? 'is' : 'is not';
                advance();
                const right = parseComparison();
                left = { type: 'binop', op, left, right };
            }
            return left;
        };

        const parseComparison = () => {
            let left = parseTerm();
            while (at('LT') || at('GT') || at('>=') || at('<=')) {
                const opMap = { LT: '<', GT: '>', '>=': '>=', '<=': '<=' };
                const tok = advance();
                const op = opMap[tok.type];
                const right = parseTerm();
                left = { type: 'binop', op, left, right };
            }
            return left;
        };

        const parseTerm = () => {
            let left = parseFactor();
            while (at('PLUS') || at('MINUS')) {
                const op = advance().type === 'PLUS' ? '+' : '-';
                const right = parseFactor();
                left = { type: 'binop', op, left, right };
            }
            return left;
        };

        const parseFactor = () => {
            let left = parseUnary();
            while (at('STAR') || at('SLASH') || at('PERCENT')) {
                const opMap = { STAR: '*', SLASH: '/', PERCENT: '%' };
                const tok = advance();
                const op = opMap[tok.type];
                const right = parseUnary();
                left = { type: 'binop', op, left, right };
            }
            return left;
        };

        const parseUnary = () => {
            if (at('NOT')) { advance(); return { type: 'unary', op: 'not', operand: parseUnary() }; }
            if (at('MINUS')) { advance(); return { type: 'unary', op: '-', operand: parseUnary() }; }
            return parsePostfix();
        };

        const parsePostfix = () => {
            let node = parsePrimary();
            while (true) {
                if (at('DOT')) {
                    advance();
                    // After dot, accept IDENT or CREATE keyword
                    let member;
                    if (at('CREATE')) { member = advance().value || 'create'; }
                    else { member = expect('IDENT').value; }
                    if (at('LPAREN')) {
                        advance();
                        const args = [];
                        if (!at('RPAREN')) {
                            args.push(parseExpr());
                            while (at('COMMA')) { advance(); args.push(parseExpr()); }
                        }
                        expect('RPAREN');
                        node = { type: 'method_call', object: node, method: member, args };
                    } else {
                        node = { type: 'member', object: node, member };
                    }
                } else if (at('LBRACKET')) {
                    advance();
                    let idx;
                    if (at('IDENT') && peek().type === 'RBRACKET') {
                        idx = { type: 'string', value: advance().value };
                    } else {
                        idx = parseExpr();
                    }
                    expect('RBRACKET');
                    node = { type: 'index', object: node, index: idx };
                } else if (at('LPAREN')) {
                    advance();
                    const args = [];
                    if (!at('RPAREN')) {
                        args.push(parseExpr());
                        while (at('COMMA')) { advance(); args.push(parseExpr()); }
                    }
                    expect('RPAREN');
                    node = { type: 'call', callee: node, args };
                } else break;
            }
            return node;
        };

        const parsePrimary = () => {
            if (at('NUMBER')) return { type: 'number', value: advance().value };
            if (at('STRING')) return { type: 'string', value: advance().value };
            if (at('YES')) { advance(); return { type: 'bool', value: true }; }
            if (at('NO')) { advance(); return { type: 'bool', value: false }; }
            if (at('EMPTY')) { advance(); return { type: 'empty' }; }
            if (at('ASK')) { advance(); return { type: 'ask', prompt: parseExpr() }; }
            if (at('BAG')) {
                advance(); expect('LBRACKET');
                const elements = [];
                if (!at('RBRACKET')) {
                    elements.push(parseExpr());
                    while (at('COMMA')) { advance(); elements.push(parseExpr()); }
                }
                expect('RBRACKET');
                return { type: 'bag', elements };
            }
            if (at('NOTEBOOK')) {
                advance(); expect('LBRACKET');
                const keys = [], values = [];
                if (!at('RBRACKET')) {
                    keys.push(expect('IDENT').value);
                    expect('COLON');
                    values.push(parseExpr());
                    while (at('COMMA')) {
                        advance();
                        keys.push(expect('IDENT').value);
                        expect('COLON');
                        values.push(parseExpr());
                    }
                }
                expect('RBRACKET');
                return { type: 'notebook', keys, values };
            }
            if (at('ME')) { advance(); return { type: 'ident', name: 'me' }; }
            if (at('COMES_FROM')) { advance(); return { type: 'ident', name: 'comes_from' }; }
            if (at('IDENT')) return { type: 'ident', name: advance().value };
            if (at('LPAREN')) { advance(); const e = parseExpr(); expect('RPAREN'); return e; }
            throw new IlmaError(`Unexpected ${current().type}`, current().line);
        };

        const parseBlock = () => {
            skipNL(); expect('INDENT');
            const stmts = [];
            while (!at('DEDENT') && !at('EOF')) {
                skipNL();
                if (at('DEDENT') || at('EOF')) break;
                stmts.push(parseStatement());
                skipNL();
            }
            if (at('DEDENT')) advance();
            return stmts;
        };

        const parseStatement = () => {
            skipNL();
            const line = current().line;

            if (at('SAY')) { advance(); return { type: 'say', expr: parseExpr(), line }; }

            if (at('REMEMBER')) {
                advance();
                const name = expect('IDENT').value;
                expect('ASSIGN');
                return { type: 'remember', name, value: parseExpr(), line };
            }

            if (at('IF')) {
                advance();
                const condition = parseExpr();
                expect('COLON');
                const thenBlock = parseBlock();
                skipNL();
                let elseBlock = null;
                if (at('OTHERWISE')) {
                    advance();
                    if (at('IF')) {
                        elseBlock = [parseStatement()];
                    } else {
                        expect('COLON');
                        elseBlock = parseBlock();
                    }
                }
                return { type: 'if', condition, thenBlock, elseBlock, line };
            }

            if (at('REPEAT')) {
                advance();
                const count = parseExpr();
                expect('COLON');
                return { type: 'repeat', count, body: parseBlock(), line };
            }

            if (at('KEEP')) {
                advance(); expect('GOING'); expect('WHILE');
                const condition = parseExpr();
                expect('COLON');
                return { type: 'while', condition, body: parseBlock(), line };
            }

            if (at('FOR')) {
                advance(); expect('EACH');
                const varName = expect('IDENT').value;
                expect('IN');
                const iterable = parseExpr();
                expect('COLON');
                return { type: 'for_each', varName, iterable, body: parseBlock(), line };
            }

            if (at('RECIPE')) {
                advance();
                const name = expect('IDENT').value;
                expect('LPAREN');
                const params = [];
                if (!at('RPAREN')) {
                    params.push(expect('IDENT').value);
                    while (at('COMMA')) { advance(); params.push(expect('IDENT').value); }
                }
                expect('RPAREN');
                expect('COLON');
                return { type: 'recipe', name, params, body: parseBlock(), line };
            }

            if (at('GIVE')) {
                advance(); expect('BACK');
                let value = null;
                if (!at('NEWLINE') && !at('DEDENT') && !at('EOF')) {
                    value = parseExpr();
                }
                return { type: 'give_back', value, line };
            }

            if (at('SHOUT')) {
                advance();
                return { type: 'shout', message: parseExpr(), line };
            }

            if (at('TRY')) {
                advance(); expect('COLON');
                const tryBlock = parseBlock();
                skipNL();
                expect('WHEN'); expect('WRONG'); expect('COLON');
                const catchBlock = parseBlock();
                return { type: 'try', tryBlock, catchBlock, line };
            }

            if (at('USE')) { advance(); const mod = expect('IDENT').value; return { type: 'use', module: mod, line }; }

            if (at('BLUEPRINT')) {
                advance();
                const name = expect('IDENT').value;
                let parent = null;
                if (at('COMES')) {
                    advance(); expect('FROM');
                    parent = expect('IDENT').value;
                }
                expect('COLON');
                skipNL(); expect('INDENT');
                const members = [];
                while (!at('DEDENT') && !at('EOF')) {
                    skipNL();
                    if (at('DEDENT') || at('EOF')) break;
                    if (at('CREATE')) {
                        const ct = advance();
                        expect('LPAREN');
                        const params = [];
                        if (!at('RPAREN')) {
                            params.push(expect('IDENT').value);
                            while (at('COMMA')) { advance(); params.push(expect('IDENT').value); }
                        }
                        expect('RPAREN'); expect('COLON');
                        members.push({ type: 'recipe', name: 'create', params, body: parseBlock(), line: ct.line });
                    } else if (at('RECIPE')) {
                        members.push(parseStatement());
                    } else {
                        advance(); // skip unknown
                    }
                    skipNL();
                }
                if (at('DEDENT')) advance();
                return { type: 'blueprint', name, parent, members, line };
            }

            // Expression statement (assignment or call)
            const expr = parseExpr();
            if (at('ASSIGN')) {
                advance();
                return { type: 'assign', target: expr, value: parseExpr(), line };
            }
            return { type: 'expr_stmt', expr, line };
        };

        const program = [];
        skipNL();
        while (!at('EOF')) {
            program.push(parseStatement());
            skipNL();
        }
        return program;
    }

    // ── Evaluator ──────────────────────────────────────
    async eval(node, env) {
        this.checkStop();

        switch (node.type) {
            case 'number': return node.value;
            case 'string': return node.value;
            case 'bool': return node.value;
            case 'empty': return null;

            case 'ident': {
                const name = node.name;
                if (name === 'comes_from') return '__comes_from__';
                if (name in env) return env[name];
                if (name in this.globals) return this.globals[name];
                throw new IlmaError(`I don't know what "${name}" is. Did you forget to use "remember"?`, 0);
            }

            case 'ask': {
                const prompt = this.toString(await this.eval(node.prompt, env));
                return await this.askFn(prompt);
            }

            case 'bag': {
                const elements = [];
                for (const el of node.elements) elements.push(await this.eval(el, env));
                return { _type: 'bag', items: elements };
            }

            case 'notebook': {
                const nb = { _type: 'notebook', entries: {} };
                for (let i = 0; i < node.keys.length; i++) {
                    nb.entries[node.keys[i]] = await this.eval(node.values[i], env);
                }
                return nb;
            }

            case 'binop': {
                const left = await this.eval(node.left, env);
                const right = await this.eval(node.right, env);
                return this.binop(node.op, left, right);
            }

            case 'unary': {
                const val = await this.eval(node.operand, env);
                if (node.op === 'not') return !this.isTruthy(val);
                if (node.op === '-') return -val;
                return val;
            }

            case 'call': {
                const name = node.callee.name || node.callee.value;
                const args = [];
                for (const a of node.args) args.push(await this.eval(a, env));

                // Blueprint constructor: ClassName(args)
                const bp = this.blueprints[name];
                if (bp) {
                    return await this._createObject(bp, args);
                }

                const recipe = this.recipes[name];
                if (!recipe) throw new IlmaError(`I don't know the recipe "${name}".`, 0);
                const localEnv = { ...this.globals };
                for (let i = 0; i < recipe.params.length; i++) {
                    localEnv[recipe.params[i]] = args[i] !== undefined ? args[i] : null;
                }
                try {
                    await this.execBlock(recipe.body, localEnv);
                } catch (e) {
                    if (e instanceof IlmaReturn) return e.value;
                    throw e;
                }
                return null;
            }

            case 'method_call': {
                const obj = await this.eval(node.object, env);
                const method = node.method;
                const args = [];
                for (const a of node.args) args.push(await this.eval(a, env));

                if (obj && obj._type === 'bag') {
                    if (method === 'add') { obj.items.push(args[0]); return null; }
                    if (method === 'remove') {
                        const idx = obj.items.findIndex(x => this.toString(x) === this.toString(args[0]));
                        if (idx >= 0) obj.items.splice(idx, 1);
                        return null;
                    }
                    if (method === 'sorted') {
                        const sorted = [...obj.items].sort((a, b) => {
                            const sa = this.toString(a), sb = this.toString(b);
                            if (typeof a === 'number' && typeof b === 'number') return a - b;
                            return sa < sb ? -1 : sa > sb ? 1 : 0;
                        });
                        return { _type: 'bag', items: sorted };
                    }
                }

                // Module function calls: finance.compound(...)
                if (obj && obj._type === 'module' && typeof obj[method] === 'function') {
                    return obj[method](...args);
                }

                if (typeof obj === 'string') {
                    if (method === 'join') {
                        const bag = args[0];
                        if (bag && bag._type === 'bag') return bag.items.map(x => this.toString(x)).join(obj);
                        return '';
                    }
                    if (method === 'upper') return obj.toUpperCase();
                    if (method === 'lower') return obj.toLowerCase();
                    if (method === 'contains') return obj.includes(this.toString(args[0]));
                    if (method === 'slice') return obj.slice(args[0], args[1]);
                }

                // Canvas drawing methods
                if (obj && obj._type === 'object' && obj._blueprint === 'Canvas') {
                    if (method === 'circle') {
                        const [cx, cy, r, color] = [args[0]||0, args[1]||0, args[2]||50, args[3]||'teal'];
                        obj._shapes.push(`<circle cx="${cx}" cy="${cy}" r="${r}" fill="${color}" opacity="0.8"/>`);
                        return null;
                    }
                    if (method === 'square') {
                        const [x, y, s, color] = [args[0]||0, args[1]||0, args[2]||50, args[3]||'purple'];
                        obj._shapes.push(`<rect x="${x}" y="${y}" width="${s}" height="${s}" fill="${color}" opacity="0.8"/>`);
                        return null;
                    }
                    if (method === 'line') {
                        const [x1,y1,x2,y2,color] = [args[0]||0, args[1]||0, args[2]||100, args[3]||100, args[4]||'white'];
                        obj._shapes.push(`<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="${color}" stroke-width="2"/>`);
                        return null;
                    }
                    if (method === 'place') {
                        const shape = args[0], x = args[1]||0, y = args[2]||0;
                        if (shape && shape._type === 'notebook' && shape.entries.type === 'star') {
                            obj._shapes.push(`<path d="${shape.entries.path}" transform="translate(${x},${y})" fill="gold" stroke="white" stroke-width="1"/>`);
                        }
                        return null;
                    }
                    if (method === 'text') {
                        const [x, y, txt, color, size] = [args[0]||0, args[1]||0, args[2]||'', args[3]||'white', args[4]||16];
                        obj._shapes.push(`<text x="${x}" y="${y}" fill="${color}" font-size="${size}" font-family="sans-serif">${txt}</text>`);
                        return null;
                    }
                    if (method === 'show') {
                        const svg = `<svg width="${obj._w}" height="${obj._h}" viewBox="0 0 ${obj._w} ${obj._h}" style="background:#1a1d27;border-radius:8px;max-width:100%">${obj._shapes.join('')}</svg>`;
                        this.output(svg);
                        return null;
                    }
                }

                // Blueprint object method call: obj.method(args)
                if (obj && obj._type === 'object') {
                    return await this._callMethod(obj, method, args);
                }

                // comes_from.create(args) or comes_from.method(args)
                if (obj === '__comes_from__') {
                    const meObj = env['me'];
                    if (meObj && meObj._type === 'object') {
                        const parentBp = this.blueprints[meObj._parent];
                        if (parentBp && parentBp.members[method]) {
                            return await this._callBpMethod(parentBp, method, meObj, args);
                        }
                    }
                    return null;
                }

                throw new IlmaError(`I don't know how to do ".${method}" on that.`, 0);
            }

            case 'member': {
                const obj = await this.eval(node.object, env);
                if (obj && obj._type === 'bag' && node.member === 'size') return obj.items.length;
                if (typeof obj === 'string' && node.member === 'length') return obj.length;
                if (obj && obj._type === 'notebook' && node.member in obj.entries) return obj.entries[node.member];
                // Object field: me.name, obj.field
                if (obj && obj._type === 'object') {
                    return obj._fields[node.member] !== undefined ? obj._fields[node.member] : null;
                }
                throw new IlmaError(`I can't find ".${node.member}" on that.`, 0);
            }

            case 'index': {
                const obj = await this.eval(node.object, env);
                const idx = await this.eval(node.index, env);
                if (obj && obj._type === 'bag') return obj.items[idx];
                if (obj && obj._type === 'notebook') {
                    const key = this.toString(idx);
                    return obj.entries[key] !== undefined ? obj.entries[key] : null;
                }
                return null;
            }

            default:
                throw new IlmaError(`Unknown expression: ${node.type}`, 0);
        }
    }

    async execBlock(stmts, env) {
        for (const stmt of stmts) {
            this.checkStop();
            await this.execStmt(stmt, env);
        }
    }

    async execStmt(stmt, env) {
        this.checkStop();

        switch (stmt.type) {
            case 'say': {
                const val = await this.eval(stmt.expr, env);
                this.output(this.toString(val));
                break;
            }

            case 'remember': {
                const val = await this.eval(stmt.value, env);
                env[stmt.name] = val;
                this.globals[stmt.name] = val;
                break;
            }

            case 'assign': {
                const val = await this.eval(stmt.value, env);
                if (stmt.target.type === 'ident') {
                    env[stmt.target.name] = val;
                    this.globals[stmt.target.name] = val;
                } else if (stmt.target.type === 'member') {
                    const obj = await this.eval(stmt.target.object, env);
                    if (obj && obj._type === 'notebook') {
                        obj.entries[stmt.target.member] = val;
                    } else if (obj && obj._type === 'object') {
                        obj._fields[stmt.target.member] = val;
                    }
                }
                break;
            }

            case 'if': {
                const cond = await this.eval(stmt.condition, env);
                if (this.isTruthy(cond)) {
                    await this.execBlock(stmt.thenBlock, env);
                } else if (stmt.elseBlock) {
                    await this.execBlock(stmt.elseBlock, env);
                }
                break;
            }

            case 'repeat': {
                const count = await this.eval(stmt.count, env);
                for (let i = 0; i < count; i++) {
                    this.checkStop();
                    await this.execBlock(stmt.body, env);
                }
                break;
            }

            case 'while': {
                while (this.isTruthy(await this.eval(stmt.condition, env))) {
                    this.checkStop();
                    await this.execBlock(stmt.body, env);
                }
                break;
            }

            case 'for_each': {
                const iterable = await this.eval(stmt.iterable, env);
                let items;
                if (iterable && iterable._type === 'bag') items = iterable.items;
                else if (iterable && iterable._type === 'notebook') items = Object.keys(iterable.entries);
                else throw new IlmaError("I can only go through bags and notebooks with 'for each'.", stmt.line);

                for (const item of items) {
                    this.checkStop();
                    env[stmt.varName] = item;
                    this.globals[stmt.varName] = item;
                    await this.execBlock(stmt.body, env);
                }
                break;
            }

            case 'recipe':
                this.recipes[stmt.name] = stmt;
                break;

            case 'blueprint': {
                const bp = {
                    name: stmt.name,
                    parent: stmt.parent,
                    members: {},
                };
                for (const m of stmt.members) {
                    bp.members[m.name] = m;
                }
                this.blueprints[stmt.name] = bp;
                break;
            }

            case 'give_back': {
                const val = stmt.value ? await this.eval(stmt.value, env) : null;
                throw new IlmaReturn(val);
            }

            case 'shout': {
                const msg = this.toString(await this.eval(stmt.message, env));
                throw new IlmaShout(msg, stmt.line);
            }

            case 'try': {
                try {
                    await this.execBlock(stmt.tryBlock, env);
                } catch (e) {
                    if (e instanceof IlmaShout) {
                        await this.execBlock(stmt.catchBlock, env);
                    } else {
                        throw e;
                    }
                }
                break;
            }

            case 'use': {
                const mod = this.modules[stmt.module];
                if (!mod) throw new IlmaError(`I don't know the module "${stmt.module}". Available: finance, time, think, body`, stmt.line);
                env[stmt.module] = mod;
                this.globals[stmt.module] = mod;
                break;
            }

            case 'expr_stmt':
                await this.eval(stmt.expr, env);
                break;
        }
    }

    // ── Helpers ────────────────────────────────────────
    isTruthy(v) {
        if (v === null || v === undefined || v === false || v === 0 || v === '') return false;
        if (v && v._type === 'bag' && v.items.length === 0) return false;
        return true;
    }

    toString(v) {
        if (v === null || v === undefined) return 'empty';
        if (v === true) return 'yes';
        if (v === false) return 'no';
        if (typeof v === 'number') {
            if (Number.isInteger(v)) return String(v);
            // Match C %g behavior
            return String(v);
        }
        if (typeof v === 'string') return v;
        if (v._type === 'bag') {
            return 'bag[' + v.items.map(x => this.toString(x)).join(', ') + ']';
        }
        if (v._type === 'notebook') {
            const pairs = Object.entries(v.entries).map(([k, val]) => `${k}: ${this.toString(val)}`);
            return 'notebook[' + pairs.join(', ') + ']';
        }
        if (v._type === 'object') {
            return '[' + (v._blueprint || 'object') + ']';
        }
        return String(v);
    }

    // ── Blueprint helpers ───────────────────────────────
    async _createObject(bp, args) {
        const obj = { _type: 'object', _blueprint: bp.name, _parent: bp.parent, _fields: {} };

        // If has parent, copy parent's create first via comes_from chain
        const createMethod = bp.members['create'];
        if (createMethod) {
            const localEnv = { me: obj };
            for (let i = 0; i < createMethod.params.length; i++) {
                localEnv[createMethod.params[i]] = args[i] !== undefined ? args[i] : null;
            }
            try { await this.execBlock(createMethod.body, localEnv); }
            catch (e) { if (!(e instanceof IlmaReturn)) throw e; }
        }
        return obj;
    }

    async _callMethod(obj, method, args) {
        // Find the method: check own blueprint first, then parent chain
        let bpName = obj._blueprint;
        while (bpName) {
            const bp = this.blueprints[bpName];
            if (!bp) break;
            if (bp.members[method]) {
                return await this._callBpMethod(bp, method, obj, args);
            }
            bpName = bp.parent;
        }
        // Check if it's a bag method on a field
        throw new IlmaError(`I don't know the method "${method}" on this ${obj._blueprint}.`, 0);
    }

    async _callBpMethod(bp, methodName, obj, args) {
        const method = bp.members[methodName];
        if (!method) throw new IlmaError(`Blueprint ${bp.name} has no method "${methodName}".`, 0);

        const localEnv = { me: obj };
        if (methodName === 'create') {
            // For comes_from.create() calls — don't return, just init fields
            for (let i = 0; i < method.params.length; i++) {
                localEnv[method.params[i]] = args[i] !== undefined ? args[i] : null;
            }
            try { await this.execBlock(method.body, localEnv); }
            catch (e) { if (!(e instanceof IlmaReturn)) throw e; }
            return null;
        }

        for (let i = 0; i < method.params.length; i++) {
            localEnv[method.params[i]] = args[i] !== undefined ? args[i] : null;
        }
        try {
            await this.execBlock(method.body, localEnv);
        } catch (e) {
            if (e instanceof IlmaReturn) return e.value;
            throw e;
        }
        return null;
    }

    binop(op, left, right) {
        // String concat
        if (op === '+' && (typeof left === 'string' || typeof right === 'string')) {
            return this.toString(left) + this.toString(right);
        }

        switch (op) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/':
                if (right === 0) throw new IlmaError("You can't divide by zero!", 0);
                return typeof left === 'number' && typeof right === 'number' &&
                       Number.isInteger(left) && Number.isInteger(right) && left % right === 0
                    ? left / right : left / right;
            case '%': return left % right;
            case 'is': return this.isEqual(left, right);
            case 'is not': return !this.isEqual(left, right);
            case '<': return left < right;
            case '>': return left > right;
            case '<=': return left <= right;
            case '>=': return left >= right;
            case 'and': return this.isTruthy(left) && this.isTruthy(right);
            case 'or': return this.isTruthy(left) || this.isTruthy(right);
        }
        return null;
    }

    isEqual(a, b) {
        if (a === null && b === null) return true;
        if (a === null || b === null) return false;
        return a === b || this.toString(a) === this.toString(b);
    }

    // ── Run ────────────────────────────────────────────
    async run(source) {
        this.globals = {};
        this.recipes = {};
        this.blueprints = {};
        this.stopped = false;
        this.stepCount = 0;

        try {
            const tokens = this.tokenize(source);
            const program = this.parse(tokens);
            await this.execBlock(program, this.globals);
        } catch (e) {
            if (e instanceof IlmaStop) return;
            if (e instanceof IlmaError) {
                this.output(`\n[Error] ${e.message}`, true);
            } else if (e instanceof IlmaShout) {
                this.output(`\n[Error on line ${e.line}] ${e.message}`, true);
            } else {
                this.output(`\n[Error] ${e.message}`, true);
            }
        }
    }
}

// Error types
class IlmaError extends Error {
    constructor(message, line) { super(message); this.line = line; }
}
class IlmaReturn {
    constructor(value) { this.value = value; }
}
class IlmaShout extends Error {
    constructor(message, line) { super(message); this.line = line; }
}
class IlmaStop extends Error {
    constructor() { super('Stopped'); }
}
