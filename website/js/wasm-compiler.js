// ILMA WASM Compiler Bridge
// Tries to load the real ILMA compiler as WASM
// Falls back to playground.js interpreter if WASM unavailable

let wasmModule = null;
let wasmReady = false;

async function initWasm() {
    try {
        const response = await fetch('/wasm/ilma-compiler.js');
        if (!response.ok) throw new Error('WASM not available');

        // Dynamic import of the Emscripten module
        const script = document.createElement('script');
        script.src = '/wasm/ilma-compiler.js';
        document.head.appendChild(script);

        await new Promise((resolve, reject) => {
            script.onload = resolve;
            script.onerror = reject;
        });

        wasmModule = await IlmaCompiler();
        wasmReady = true;
        updateBadge(true);
    } catch (e) {
        console.log('WASM not available, using JS interpreter fallback');
        wasmReady = false;
        updateBadge(false);
    }
}

function runWithWasm(code) {
    if (!wasmReady || !wasmModule) {
        return { output: '', error: 'WASM not loaded' };
    }
    try {
        const resultPtr = wasmModule.ccall('compile_and_run', 'string', ['string'], [code]);
        const result = JSON.parse(resultPtr);
        return result;
    } catch (e) {
        return { output: '', error: e.message };
    }
}

function runIlmaCode(code) {
    if (wasmReady) {
        return runWithWasm(code);
    }
    // Fallback to JS interpreter
    if (typeof runIlma === 'function') {
        return runIlma(code);
    }
    return { output: '', error: 'No interpreter available' };
}

function updateBadge(isWasm) {
    const badge = document.getElementById('compiler-badge');
    if (badge) {
        badge.textContent = isWasm ? '\u26a1 Real ILMA compiler' : '\ud83d\udcdd Preview mode';
        badge.className = isWasm ? 'badge badge-wasm' : 'badge badge-preview';
    }
}

// Try to load WASM on page load
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initWasm);
} else {
    initWasm();
}
