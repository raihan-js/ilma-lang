/* ILMA Playground Interpreter — playground.js
   Handles: say, remember, arithmetic, strings, if/otherwise,
   repeat, for each, recipe/give back, bag, comparisons, comments. */

function runIlma(code) {
    var output = [], vars = {}, funcs = {}, error = null;
    try {
        var rawLines = code.split('\n'), lines = [];
        for (var i = 0; i < rawLines.length; i++) lines.push({ text: rawLines[i], num: i + 1 });
        execBlock(lines, vars, funcs, output, 0);
    } catch (e) { error = e.message || String(e); }
    return { output: output.join('\n'), error: error };
}

function getIndent(line) {
    var m = line.match(/^(\s*)/);
    return m ? m[1].length : 0;
}

function execBlock(lines, vars, funcs, output, baseIndent) {
    var i = 0;
    while (i < lines.length) {
        var line = lines[i], trimmed = line.text.trim();
        if (trimmed === '' || trimmed.startsWith('#')) { i++; continue; }
        var indent = getIndent(line.text);
        if (indent < baseIndent) break;
        var result = execLine(trimmed, line.num, lines, i, vars, funcs, output, indent);
        if (result && result.type === 'give_back') return result;
        i = (result && result.nextIndex !== undefined) ? result.nextIndex : i + 1;
    }
    return null;
}

function getBlock(lines, startIndex, blockIndent) {
    var block = [], i = startIndex;
    while (i < lines.length) {
        var lineIndent = getIndent(lines[i].text), trimmed = lines[i].text.trim();
        if (trimmed === '' || trimmed.startsWith('#')) { block.push(lines[i]); i++; continue; }
        if (lineIndent >= blockIndent) { block.push(lines[i]); i++; }
        else break;
    }
    return { block: block, endIndex: i };
}

function getBodyBlock(lines, index, currentIndent) {
    var bodyStart = index + 1, blockIndent = currentIndent + 4;
    if (bodyStart < lines.length) {
        var first = getIndent(lines[bodyStart].text);
        if (first > currentIndent) blockIndent = first;
    }
    return getBlock(lines, bodyStart, blockIndent);
}

function execLine(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent) {
    if (trimmed.startsWith('say ')) {
        output.push(String(evalExpr(trimmed.substring(4).trim(), vars, funcs, output)));
        return null;
    }
    if (trimmed.startsWith('remember ')) {
        var rest = trimmed.substring(9).trim(), eqIdx = rest.indexOf('=');
        if (eqIdx === -1) throw new Error('Line ' + lineNum + ': Expected = in remember statement');
        var varName = rest.substring(0, eqIdx).trim(), valExpr = rest.substring(eqIdx + 1).trim();
        if (valExpr.startsWith('ask ')) {
            var promptStr = evalExpr(valExpr.substring(4).trim(), vars, funcs, output);
            var userInput = prompt(String(promptStr));
            if (userInput === null) userInput = '';
            var asNum = Number(userInput);
            vars[varName] = isNaN(asNum) || userInput.trim() === '' ? userInput : asNum;
        } else {
            vars[varName] = evalExpr(valExpr, vars, funcs, output);
        }
        return null;
    }
    if (trimmed.startsWith('recipe ')) {
        var sig = trimmed.substring(7).trim();
        if (sig.endsWith(':')) sig = sig.slice(0, -1).trim();
        var po = sig.indexOf('('), pc = sig.indexOf(')');
        var funcName = sig.substring(0, po).trim();
        var paramStr = sig.substring(po + 1, pc).trim();
        var params = paramStr ? paramStr.split(',').map(function(p) { return p.trim(); }) : [];
        var bi = getBodyBlock(lines, index, currentIndent);
        var blockIndent = currentIndent + 4;
        if (index + 1 < lines.length) {
            var f = getIndent(lines[index + 1].text);
            if (f > currentIndent) blockIndent = f;
        }
        funcs[funcName] = { params: params, body: bi.block, bodyIndent: blockIndent };
        return { nextIndex: bi.endIndex };
    }
    if (trimmed.startsWith('give back ')) {
        return { type: 'give_back', value: evalExpr(trimmed.substring(10).trim(), vars, funcs, output) };
    }
    if (trimmed === 'give back') return { type: 'give_back', value: null };
    if (trimmed.startsWith('repeat ')) {
        var repRest = trimmed.substring(7).trim();
        if (repRest.endsWith(':')) repRest = repRest.slice(0, -1).trim();
        var times = Math.floor(Number(evalExpr(repRest, vars, funcs, output)));
        if (isNaN(times) || times < 0) throw new Error('Line ' + lineNum + ': repeat needs a positive number');
        var bi = getBodyBlock(lines, index, currentIndent);
        for (var r = 0; r < times; r++) {
            var lv = Object.assign({}, vars);
            var res = execBlock(bi.block.slice(), lv, funcs, output, bi.block.length ? getIndent(bi.block[0].text) : currentIndent + 4);
            Object.assign(vars, lv);
            if (res && res.type === 'give_back') return res;
        }
        return { nextIndex: bi.endIndex };
    }
    if (trimmed.startsWith('for each ')) {
        var feRest = trimmed.substring(9).trim(), inIdx = feRest.indexOf(' in ');
        if (inIdx === -1) throw new Error('Line ' + lineNum + ': Expected "in" in for each');
        var iterVar = feRest.substring(0, inIdx).trim();
        var collExpr = feRest.substring(inIdx + 4).trim();
        if (collExpr.endsWith(':')) collExpr = collExpr.slice(0, -1).trim();
        var collection = evalExpr(collExpr, vars, funcs, output);
        if (!Array.isArray(collection)) {
            if (typeof collection === 'string') collection = collection.split('');
            else throw new Error('Line ' + lineNum + ': for each requires a bag or text');
        }
        var bi = getBodyBlock(lines, index, currentIndent);
        var bIndent = bi.block.length ? getIndent(bi.block[0].text) : currentIndent + 4;
        for (var fi = 0; fi < collection.length; fi++) {
            vars[iterVar] = collection[fi];
            var res = execBlock(bi.block.slice(), vars, funcs, output, bIndent);
            if (res && res.type === 'give_back') return res;
        }
        return { nextIndex: bi.endIndex };
    }
    if (trimmed.startsWith('if ')) {
        return handleIf(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent);
    }
    if (trimmed.match(/^[a-zA-Z_]\w*\s*\(/) && !trimmed.startsWith('remember') && !trimmed.startsWith('say')) {
        evalExpr(trimmed, vars, funcs, output);
        return null;
    }
    if (trimmed.match(/^[a-zA-Z_]\w*\s*=/)) {
        var eqIdx = trimmed.indexOf('='), vName = trimmed.substring(0, eqIdx).trim();
        vars[vName] = evalExpr(trimmed.substring(eqIdx + 1).trim(), vars, funcs, output);
        return null;
    }
    if (trimmed.startsWith('check ') || trimmed === 'check:') {
        return handleCheck(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent);
    }
    if (trimmed.startsWith('keep going while ')) {
        var kwRest = trimmed.substring(17).trim();
        if (kwRest.endsWith(':')) kwRest = kwRest.slice(0, -1).trim();
        var bi = getBodyBlock(lines, index, currentIndent);
        var bIndent = bi.block.length ? getIndent(bi.block[0].text) : currentIndent + 4;
        var maxIter = 10000;
        while (maxIter-- > 0) {
            if (!isTruthy(evalExpr(kwRest, vars, funcs, output))) break;
            var lv2 = Object.assign({}, vars);
            var res = execBlock(bi.block.slice(), lv2, funcs, output, bIndent);
            Object.assign(vars, lv2);
            if (res && res.type === 'give_back') return res;
        }
        return { nextIndex: bi.endIndex };
    }
    if (trimmed.startsWith('shout ')) {
        var shoutMsg = evalExpr(trimmed.substring(6).trim(), vars, funcs, output);
        throw new Error(String(shoutMsg));
    }
    if (trimmed.startsWith('try:') || trimmed === 'try:') {
        return handleTry(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent);
    }
    if (trimmed.match(/^[a-zA-Z_][\w.]*\s*=/)) {
        var dotEqIdx = trimmed.indexOf('=');
        var lhsStr = trimmed.substring(0, dotEqIdx).trim();
        var rhsStr = trimmed.substring(dotEqIdx + 1).trim();
        var rhsVal = evalExpr(rhsStr, vars, funcs, output);
        if (lhsStr.indexOf('.') !== -1) {
            var parts2 = lhsStr.split('.');
            var obj2 = vars[parts2[0]];
            if (obj2 && typeof obj2 === 'object' && !Array.isArray(obj2)) {
                obj2[parts2.slice(1).join('.')] = rhsVal;
            }
        } else {
            vars[lhsStr] = rhsVal;
        }
        return null;
    }
    if (trimmed.startsWith('use ')) {
        var modName = trimmed.substring(4).trim();
        if (!vars.__modules) vars.__modules = {};
        vars.__modules[modName] = true;
        return null;
    }
    if (trimmed === 'otherwise:' || trimmed === 'otherwise') {
        return null; // handled by parent if block
    }
    throw new Error('Line ' + lineNum + ': I don\'t understand "' + trimmed + '"');
}

function handleIf(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent) {
    var condStr = trimmed.substring(3).trim();
    if (condStr.endsWith(':')) condStr = condStr.slice(0, -1).trim();
    var condition = evalExpr(condStr, vars, funcs, output);
    var bi = getBodyBlock(lines, index, currentIndent);
    var bIndent = bi.block.length ? getIndent(bi.block[0].text) : currentIndent + 4;
    var branches = [{ condition: condition, block: bi.block, indent: bIndent }];
    var nextI = bi.endIndex;
    while (nextI < lines.length) {
        var nt = lines[nextI].text.trim();
        if (nt.startsWith('otherwise if ')) {
            var oc = nt.substring(13).trim();
            if (oc.endsWith(':')) oc = oc.slice(0, -1).trim();
            var obi = getBodyBlock(lines, nextI, currentIndent);
            var obIndent = obi.block.length ? getIndent(obi.block[0].text) : currentIndent + 4;
            branches.push({ condition: evalExpr(oc, vars, funcs, output), block: obi.block, indent: obIndent });
            nextI = obi.endIndex;
        } else if (nt === 'otherwise:' || nt === 'otherwise') {
            var ebi = getBodyBlock(lines, nextI, currentIndent);
            var ebIndent = ebi.block.length ? getIndent(ebi.block[0].text) : currentIndent + 4;
            branches.push({ condition: true, block: ebi.block, indent: ebIndent });
            nextI = ebi.endIndex;
            break;
        } else { break; }
    }
    for (var b = 0; b < branches.length; b++) {
        if (isTruthy(branches[b].condition)) {
            var res = execBlock(branches[b].block.slice(), vars, funcs, output, branches[b].indent);
            if (res && res.type === 'give_back') return res;
            break;
        }
    }
    return { nextIndex: nextI };
}

function isTruthy(val) {
    if (val === false || val === null || val === undefined || val === 0 || val === '' || val === 'no' || val === 'empty') return false;
    if (val === 'yes') return true;
    return !!val;
}

function evalExpr(expr, vars, funcs, output) {
    expr = expr.trim();
    if (expr === '') return '';
    if (expr === 'yes') return true;
    if (expr === 'no') return false;
    if (expr === 'empty') return null;
    if ((expr.startsWith('"') && expr.endsWith('"')) || (expr.startsWith("'") && expr.endsWith("'"))) {
        var rawStr = expr.slice(1, -1);
        if (expr.startsWith('"') && rawStr.includes('{')) {
            return rawStr.replace(/\{([^}]+)\}/g, function(m, inner) {
                try { return String(evalExpr(inner.trim(), vars, funcs, output)); } catch(e) { return m; }
            });
        }
        return rawStr;
    }
    if (expr.match(/^-?\d+(\.\d+)?$/)) return Number(expr);
    if (expr.startsWith('bag[')) {
        var inner = expr.substring(4, expr.length - 1);
        return splitArgs(inner).map(function(item) { return evalExpr(item.trim(), vars, funcs, output); });
    }
    if (expr.startsWith('[') && expr.endsWith(']')) {
        var inner = expr.substring(1, expr.length - 1).trim();
        if (inner === '') return [];
        return splitArgs(inner).map(function(item) { return evalExpr(item.trim(), vars, funcs, output); });
    }
    if (expr.startsWith('(') && findMatchingParen(expr, 0) === expr.length - 1)
        return evalExpr(expr.slice(1, -1), vars, funcs, output);
    var compOps = [' is not ', ' is ', '!=', '==', '>=', '<=', '>', '<'];
    for (var ci = 0; ci < compOps.length; ci++) {
        var sp = findOperator(expr, compOps[ci]);
        if (sp !== -1) {
            var l = evalExpr(expr.substring(0, sp), vars, funcs, output);
            var r = evalExpr(expr.substring(sp + compOps[ci].length), vars, funcs, output);
            switch (compOps[ci].trim()) {
                case 'is not': case '!=': return l !== r;
                case 'is': case '==': return l === r;
                case '>=': return l >= r; case '<=': return l <= r;
                case '>': return l > r; case '<': return l < r;
            }
        }
    }
    var andIdx = findOperator(expr, ' and ');
    if (andIdx !== -1) return isTruthy(evalExpr(expr.substring(0, andIdx), vars, funcs, output)) && isTruthy(evalExpr(expr.substring(andIdx + 5), vars, funcs, output));
    var orIdx = findOperator(expr, ' or ');
    if (orIdx !== -1) return isTruthy(evalExpr(expr.substring(0, orIdx), vars, funcs, output)) || isTruthy(evalExpr(expr.substring(orIdx + 4), vars, funcs, output));
    if (expr.startsWith('not ')) return !isTruthy(evalExpr(expr.substring(4), vars, funcs, output));
    var addIdx = findOperatorRTL(expr, ['+', '-']);
    if (addIdx > 0) {
        var op = expr[addIdx], left = evalExpr(expr.substring(0, addIdx), vars, funcs, output);
        var right = evalExpr(expr.substring(addIdx + 1), vars, funcs, output);
        if (op === '+') {
            if (typeof left === 'string' || typeof right === 'string') return String(left) + String(right);
            return Number(left) + Number(right);
        }
        return Number(left) - Number(right);
    }
    var mulIdx = findOperatorRTL(expr, ['*', '/', '%']);
    if (mulIdx > 0) {
        var op = expr[mulIdx], left = evalExpr(expr.substring(0, mulIdx), vars, funcs, output);
        var right = evalExpr(expr.substring(mulIdx + 1), vars, funcs, output);
        if (op === '*') return Number(left) * Number(right);
        if (op === '/') { if (Number(right) === 0) throw new Error('Cannot divide by zero'); return Number(left) / Number(right); }
        if (op === '%') return Number(left) % Number(right);
    }
    // Module and method calls: obj.method(args) or module.func(args)
    var dotCallMatch = expr.match(/^([a-zA-Z_][\w.]*)\s*\((.*)\)\s*$/s);
    if (dotCallMatch && dotCallMatch[1].indexOf('.') !== -1) {
        var fullName = dotCallMatch[1];
        var argsStr2 = dotCallMatch[2].trim();
        var args2 = argsStr2 === '' ? [] : splitArgs(argsStr2);
        var ea2 = args2.map(function(a) { return evalExpr(a.trim(), vars, funcs, output); });
        var dotIdx = fullName.lastIndexOf('.');
        var objName = fullName.substring(0, dotIdx);
        var methodName = fullName.substring(dotIdx + 1);
        var objVal = evalExpr(objName, vars, funcs, output);

        // Bag higher-order methods
        if (Array.isArray(objVal)) {
            if (methodName === 'map') {
                var fn = ea2[0];
                return objVal.map(function(item) { return callLambda(fn, [item], vars, funcs, output); });
            }
            if (methodName === 'filter') {
                var fn = ea2[0];
                return objVal.filter(function(item) { return isTruthy(callLambda(fn, [item], vars, funcs, output)); });
            }
            if (methodName === 'each') {
                var fn = ea2[0];
                objVal.forEach(function(item) { callLambda(fn, [item], vars, funcs, output); });
                return null;
            }
            if (methodName === 'add') { objVal.push(ea2[0]); return null; }
            if (methodName === 'remove') {
                var ri = objVal.indexOf(ea2[0]);
                if (ri !== -1) objVal.splice(ri, 1);
                return null;
            }
            if (methodName === 'join') { return objVal.join(ea2[0] != null ? String(ea2[0]) : ', '); }
            if (methodName === 'sorted') { return objVal.slice().sort(function(a,b){ return a<b?-1:a>b?1:0; }); }
        }
        // String methods
        if (typeof objVal === 'string') {
            if (methodName === 'upper') return objVal.toUpperCase();
            if (methodName === 'lower') return objVal.toLowerCase();
            if (methodName === 'contains') return objVal.includes(String(ea2[0]));
            if (methodName === 'slice') return objVal.slice(Number(ea2[0]), Number(ea2[1]));
        }
        // Notebook methods
        if (objVal && typeof objVal === 'object' && !Array.isArray(objVal)) {
            if (methodName === 'keys') return Object.keys(objVal);
        }
        // Blueprint method call
        if (objVal && typeof objVal === 'object' && objVal.__blueprint) {
            return callBlueprintMethod(objVal, methodName, ea2, vars, funcs, output);
        }
        // Module functions
        return callModule(objName, methodName, ea2, vars, funcs, output);
    }
    // Member access (no call): obj.prop
    var dotAccessMatch = expr.match(/^([a-zA-Z_][\w]*(?:\.[a-zA-Z_]\w*)*)\.([a-zA-Z_]\w*)$/);
    if (dotAccessMatch) {
        var objStr = dotAccessMatch[1];
        var propName = dotAccessMatch[2];
        var objV = evalExpr(objStr, vars, funcs, output);
        if (Array.isArray(objV)) {
            if (propName === 'size' || propName === 'length') return objV.length;
        }
        if (typeof objV === 'string') {
            if (propName === 'length' || propName === 'size') return objV.length;
        }
        if (objV && typeof objV === 'object' && !Array.isArray(objV)) {
            if (propName === 'size') return Object.keys(objV).length;
            return objV[propName] !== undefined ? objV[propName] : null;
        }
        return null;
    }
    // Lambda: recipe(params): expr
    var lambdaMatch = expr.match(/^recipe\s*\(([^)]*)\)\s*:\s*(.+)$/s);
    if (lambdaMatch) {
        var lParams = lambdaMatch[1].trim() ? lambdaMatch[1].split(',').map(function(p){ return p.trim(); }) : [];
        var lExpr = lambdaMatch[2].trim();
        return { __lambda: true, params: lParams, body: lExpr, closure: Object.assign({}, vars) };
    }
    // Notebook literal: notebook[key: val, ...]
    if (expr.startsWith('notebook[')) {
        var inner2 = expr.substring(9, expr.length - 1).trim();
        var obj3 = {};
        if (inner2) {
            splitArgs(inner2).forEach(function(pair) {
                var ci = pair.indexOf(':');
                if (ci !== -1) {
                    var k = pair.substring(0, ci).trim();
                    var v = evalExpr(pair.substring(ci + 1).trim(), vars, funcs, output);
                    obj3[k] = v;
                }
            });
        }
        return obj3;
    }
    var funcMatch = expr.match(/^([a-zA-Z_]\w*)\s*\((.*)\)$/s);
    if (funcMatch) {
        var fname = funcMatch[1], argsStr = funcMatch[2].trim();
        var args = argsStr === '' ? [] : splitArgs(argsStr);
        var ea = args.map(function(a) { return evalExpr(a.trim(), vars, funcs, output); });
        if (fname === 'length') return ea[0] != null ? (Array.isArray(ea[0]) ? ea[0].length : String(ea[0]).length) : 0;
        if (fname === 'number') return Number(ea[0]);
        if (fname === 'text') return String(ea[0]);
        if (fname === 'round') return Math.round(ea[0]);
        if (fname === 'abs') return Math.abs(ea[0]);
        if (funcs[fname]) {
            var func = funcs[fname], lv = Object.assign({}, vars);
            for (var pi = 0; pi < func.params.length; pi++) lv[func.params[pi]] = ea[pi] !== undefined ? ea[pi] : null;
            var res = execBlock(func.body.slice(), lv, funcs, output, func.bodyIndent);
            if (res && res.type === 'give_back') return res.value;
            return null;
        }
        throw new Error('Unknown recipe: ' + fname);
    }
    if (expr.endsWith('.length')) {
        var obj = evalExpr(expr.slice(0, -7), vars, funcs, output);
        return Array.isArray(obj) ? obj.length : String(obj).length;
    }
    if (expr.match(/^[a-zA-Z_]\w*$/)) {
        if (expr in vars) return vars[expr];
        throw new Error('Unknown name: ' + expr);
    }
    throw new Error('Cannot understand: ' + expr);
}

function findOperator(expr, op) {
    var depth = 0, inStr = false, strChar = '';
    for (var i = 0; i < expr.length; i++) {
        var c = expr[i];
        if (inStr) { if (c === strChar) inStr = false; continue; }
        if (c === '"' || c === "'") { inStr = true; strChar = c; continue; }
        if (c === '(' || c === '[') { depth++; continue; }
        if (c === ')' || c === ']') { depth--; continue; }
        if (depth === 0 && expr.substring(i, i + op.length) === op) return i;
    }
    return -1;
}

function findOperatorRTL(expr, ops) {
    var depth = 0, inStr = false, strChar = '', found = -1;
    for (var i = 0; i < expr.length; i++) {
        var c = expr[i];
        if (inStr) { if (c === strChar) inStr = false; continue; }
        if (c === '"' || c === "'") { inStr = true; strChar = c; continue; }
        if (c === '(' || c === '[') { depth++; continue; }
        if (c === ')' || c === ']') { depth--; continue; }
        if (depth === 0) {
            for (var oi = 0; oi < ops.length; oi++) {
                if (c === ops[oi] && i > 0) {
                    var pc = expr[i - 1];
                    if (c === '-' && (i === 0 || '(,[+-*/%=<>!'.indexOf(pc.trim() || '(') !== -1)) continue;
                    found = i;
                }
            }
        }
    }
    return found;
}

function findMatchingParen(expr, start) {
    var depth = 0;
    for (var i = start; i < expr.length; i++) {
        if (expr[i] === '(') depth++;
        else if (expr[i] === ')') { depth--; if (depth === 0) return i; }
    }
    return -1;
}

function splitArgs(str) {
    var args = [], depth = 0, current = '', inStr = false, strChar = '';
    for (var i = 0; i < str.length; i++) {
        var c = str[i];
        if (inStr) { current += c; if (c === strChar) inStr = false; continue; }
        if (c === '"' || c === "'") { inStr = true; strChar = c; current += c; continue; }
        if (c === '(' || c === '[') { depth++; current += c; continue; }
        if (c === ')' || c === ']') { depth--; current += c; continue; }
        if (c === ',' && depth === 0) { args.push(current); current = ''; continue; }
        current += c;
    }
    if (current.trim() !== '') args.push(current);
    return args;
}

function callLambda(fn, args, vars, funcs, output) {
    if (!fn) return null;
    if (typeof fn === 'function') return fn.apply(null, args);
    if (fn && fn.__lambda) {
        var lv = Object.assign({}, fn.closure || {}, vars);
        fn.params.forEach(function(p, i) { lv[p] = args[i] !== undefined ? args[i] : null; });
        // Evaluate body expression
        try { return evalExpr(fn.body, lv, funcs, output); } catch(e) { return null; }
    }
    if (fn && fn.params !== undefined) {
        // Named function stored as object
        var lv2 = Object.assign({}, vars);
        fn.params.forEach(function(p, i) { lv2[p] = args[i] !== undefined ? args[i] : null; });
        var res = execBlock(fn.body.slice(), lv2, funcs, output, fn.bodyIndent || 4);
        if (res && res.type === 'give_back') return res.value;
        return null;
    }
    return null;
}

function callModule(modName, funcName, args, vars, funcs, output) {
    var modules = {
        science: {
            gravity: function(m) { return m * 9.81; },
            celsius_to_fahrenheit: function(c) { return (c * 9/5) + 32; },
            celsius_to_kelvin: function(c) { return c + 273.15; },
            speed: function(d, t) { return d / t; },
            kinetic_energy: function(m, v) { return 0.5 * m * v * v; }
        },
        trade: {
            profit: function(cost, rev) { return rev - cost; },
            margin: function(cost, rev) { return ((rev - cost) / rev) * 100; },
            discount: function(price, pct) { return price * (1 - pct / 100); },
            vat: function(price, rate) { return price * (1 + rate / 100); },
            halal_check: function(interest, gambling, alcohol) {
                var ok = !interest && !gambling && !alcohol;
                return { permissible: ok, reason: ok ? 'All conditions met' : 'Contains prohibited element' };
            }
        },
        finance: {
            zakat: function(wealth, nisab) { return wealth >= nisab ? wealth * 0.025 : 0; },
            compound: function(p, r, t) { return p * Math.pow(1 + r/100, t); },
            profit: function(c, r) { return r - c; },
            margin: function(c, r) { return ((r-c)/r)*100; },
            simple_interest: function(p, r, t) { return p * r * t / 100; }
        },
        number: {
            sqrt: function(n) { return Math.sqrt(n); },
            abs: function(n) { return Math.abs(n); },
            round: function(n) { return Math.round(n); },
            floor: function(n) { return Math.floor(n); },
            ceil: function(n) { return Math.ceil(n); },
            is_prime: function(n) {
                if (n < 2) return false;
                for (var i = 2; i*i <= n; i++) if (n%i===0) return false;
                return true;
            },
            fibonacci: function(n) {
                if (n <= 1) return n;
                var a=0,b=1;
                for (var i=2;i<=n;i++){var t=a+b;a=b;b=t;}
                return b;
            }
        },
        body: {
            bmi: function(weight, height) { return weight / (height * height); },
            daily_water: function(weight) { return weight * 0.033; }
        },
        think: {
            stoic_question: function() { return 'Is this in your control?'; }
        }
    };
    var mod = modules[modName];
    if (mod && mod[funcName]) {
        return mod[funcName].apply(null, args);
    }
    // Try vars for user-defined modules
    if (vars[modName] && typeof vars[modName] === 'object') {
        var mObj = vars[modName];
        if (typeof mObj[funcName] === 'function') return mObj[funcName].apply(null, args);
    }
    return null;
}

function callBlueprintMethod(obj, method, args, vars, funcs, output) {
    if (obj.__methods && obj.__methods[method]) {
        return callLambda(obj.__methods[method], args, Object.assign({me: obj}, vars), funcs, output);
    }
    return null;
}

function handleCheck(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent) {
    var subjStr = trimmed.replace(/^check\s*/, '').replace(/:$/, '').trim();
    var subject = subjStr ? evalExpr(subjStr, vars, funcs, output) : null;
    var bi = getBodyBlock(lines, index, currentIndent);
    var whenIndent = currentIndent + 4;
    if (bi.block.length) whenIndent = getIndent(bi.block[0].text);

    var i = 0, matched = false;
    while (i < bi.block.length) {
        var t = bi.block[i].text.trim();
        if (t === '' || t.startsWith('#')) { i++; continue; }
        if (t.startsWith('when ')) {
            var patStr = t.substring(5).trim();
            if (patStr.endsWith(':')) patStr = patStr.slice(0, -1).trim();
            var bodyBlock = getBodyBlock(bi.block, i, whenIndent);
            var bInd = bodyBlock.block.length ? getIndent(bodyBlock.block[0].text) : whenIndent + 4;
            if (!matched && matchPattern(subject, patStr, vars, funcs, output)) {
                matched = true;
                var res = execBlock(bodyBlock.block.slice(), vars, funcs, output, bInd);
                if (res && res.type === 'give_back') return res;
            }
            i = bodyBlock.endIndex;
        } else if (t.startsWith('otherwise')) {
            var obBlock = getBodyBlock(bi.block, i, whenIndent);
            var obInd = obBlock.block.length ? getIndent(obBlock.block[0].text) : whenIndent + 4;
            if (!matched) {
                var res2 = execBlock(obBlock.block.slice(), vars, funcs, output, obInd);
                if (res2 && res2.type === 'give_back') return res2;
            }
            i = obBlock.endIndex;
        } else { i++; }
    }
    return { nextIndex: bi.endIndex };
}

function matchPattern(subject, patStr, vars, funcs, output) {
    // Range pattern: 90..99
    var rangeMatch = patStr.match(/^(-?\d+(?:\.\d+)?)\s*\.\.\s*(-?\d+(?:\.\d+)?)$/);
    if (rangeMatch) {
        var lo = Number(rangeMatch[1]), hi = Number(rangeMatch[2]);
        var sv = Number(subject);
        return sv >= lo && sv <= hi;
    }
    // Or-pattern: "cat" or "dog"
    if (patStr.indexOf(' or ') !== -1) {
        var parts = patStr.split(' or ').map(function(p) { return p.trim(); });
        return parts.some(function(p) { return matchPattern(subject, p, vars, funcs, output); });
    }
    // Exact match
    var patVal = evalExpr(patStr, vars, funcs, output);
    return subject == patVal || subject === patVal;
}

function handleTry(trimmed, lineNum, lines, index, vars, funcs, output, currentIndent) {
    var tryBlock = getBodyBlock(lines, index, currentIndent);
    var bInd = tryBlock.block.length ? getIndent(tryBlock.block[0].text) : currentIndent + 4;
    var nextI = tryBlock.endIndex;
    var errMsg = null;
    try {
        execBlock(tryBlock.block.slice(), vars, funcs, output, bInd);
    } catch(e) {
        errMsg = e.message || String(e);
    }
    // Check for 'when wrong:'
    if (nextI < lines.length) {
        var nt = lines[nextI].text.trim();
        if (nt === 'when wrong:' || nt === 'when wrong') {
            var wBlock = getBodyBlock(lines, nextI, currentIndent);
            var wInd = wBlock.block.length ? getIndent(wBlock.block[0].text) : currentIndent + 4;
            if (errMsg !== null) {
                vars['error'] = errMsg;
                execBlock(wBlock.block.slice(), vars, funcs, output, wInd);
            }
            nextI = wBlock.endIndex;
        }
    }
    return { nextIndex: nextI };
}
