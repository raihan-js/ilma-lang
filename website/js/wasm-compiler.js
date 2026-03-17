// ILMA WASM Compiler Bridge
// Tries to load the real ILMA compiler as WASM (built with Emscripten)
// Falls back to playground.js JS interpreter if WASM unavailable

const IlmaWasm = {
    module: null,
    ready: false,

    async init() {
        try {
            const response = await fetch('/wasm/ilma-compiler.js');
            if (!response.ok) throw new Error('WASM not available');

            const mod = await import('/wasm/ilma-compiler.js');
            this.module = await mod.default();
            this.ready = true;

            const badge = document.getElementById('compiler-badge');
            if (badge) {
                badge.textContent = '\u26a1 Real ILMA compiler';
                badge.className = 'badge badge-green';
            }
            return true;
        } catch(e) {
            console.log('WASM not available, using JS interpreter fallback');
            const badge = document.getElementById('compiler-badge');
            if (badge) {
                badge.textContent = '\ud83d\udd04 Preview mode';
                badge.className = 'badge badge-yellow';
            }
            return false;
        }
    },

    run(code) {
        if (!this.ready || !this.module) return null;
        try {
            const result = this.module.ccall(
                'ilma_eval_wasm', 'string', ['string'], [code]
            );
            return JSON.parse(result);
        } catch(e) {
            return { output: '', error: String(e) };
        }
    }
};

function runIlmaCode(code) {
    // Try real WASM compiler first
    if (IlmaWasm.ready) {
        const result = IlmaWasm.run(code);
        if (result) return result;
    }
    // Fallback to JS interpreter
    if (typeof runIlma === 'function') {
        return runIlma(code);
    }
    return { output: '', error: 'No interpreter available' };
}

// Keep backward compat
function runWithWasm(code) { return IlmaWasm.run(code); }
function updateBadge(isWasm) {
    const badge = document.getElementById('compiler-badge');
    if (badge) {
        badge.textContent = isWasm ? '\u26a1 Real ILMA compiler' : '\ud83d\udd04 Preview mode';
        badge.className = isWasm ? 'badge badge-green' : 'badge badge-yellow';
    }
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', function() { IlmaWasm.init(); });
} else {
    IlmaWasm.init();
}
