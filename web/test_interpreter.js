const fs = require('fs');

// Load interpreter and lessons
eval(fs.readFileSync(__dirname + '/interpreter.js', 'utf8'));
eval(fs.readFileSync(__dirname + '/lessons.js', 'utf8').replace('const LESSONS', 'var LESSONS'));

async function test(name, code, expected) {
    let output = [];
    const interp = new IlmaInterpreter(
        (text) => output.push(text),
        (prompt) => Promise.resolve('test')
    );
    try {
        await interp.run(code);
        const got = output.join('\n');
        if (expected && got === expected) {
            console.log('  PASS  ' + name);
            return true;
        } else if (!expected) {
            console.log('  SKIP  ' + name + ' (interactive)');
            return true;
        } else {
            console.log('  FAIL  ' + name);
            const expLines = expected.split('\n');
            const gotLines = got.split('\n');
            for (let i = 0; i < Math.max(expLines.length, gotLines.length); i++) {
                if (expLines[i] !== gotLines[i]) {
                    console.log('    Line ' + (i+1) + ':');
                    console.log('      Exp: ' + JSON.stringify(expLines[i]));
                    console.log('      Got: ' + JSON.stringify(gotLines[i]));
                    break;
                }
            }
            return false;
        }
    } catch(e) {
        console.log('  ERR   ' + name + ' — ' + e.message);
        return false;
    }
}

(async () => {
    console.log('=== ILMA JS Interpreter Tests ===\n');
    let pass = 0, fail = 0;
    for (const lesson of LESSONS) {
        const ok = await test('L' + lesson.id + ': ' + lesson.title, lesson.code.trim(), lesson.expected);
        if (ok) pass++; else fail++;
    }
    console.log('\n=== Results: ' + pass + ' pass, ' + fail + ' fail ===');
    process.exit(fail > 0 ? 1 : 0);
})();
