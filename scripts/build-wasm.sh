#!/bin/bash
# Build ILMA compiler to WebAssembly using Emscripten
# Requires: emcc (from Emscripten SDK)
# Install: https://emscripten.org/docs/getting_started/
set -e

EMCC=$(which emcc 2>/dev/null || echo "")
if [ -z "$EMCC" ]; then
    echo "Error: Emscripten (emcc) not found."
    echo "Install from: https://emscripten.org/docs/getting_started/"
    echo ""
    echo "Quick install:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

echo "Building ILMA compiler to WebAssembly..."

mkdir -p website/wasm

emcc src/evaluator.c src/lexer.c src/parser.c src/ast.c \
  src/runtime/ilma_runtime.c \
  src/runtime/modules/finance.c \
  src/runtime/modules/think.c \
  src/runtime/modules/body.c \
  src/runtime/modules/time_mod.c \
  src/runtime/modules/quran.c \
  src/runtime/modules/number.c \
  src/runtime/modules/draw.c \
  src/runtime/modules/science.c \
  src/runtime/modules/trade.c \
  -I src/runtime \
  -O2 \
  -s WASM=1 \
  -s EXPORTED_FUNCTIONS='["_ilma_eval_wasm","_wasm_run_ilma","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='IlmaCompiler' \
  -s NO_EXIT_RUNTIME=1 \
  -s ENVIRONMENT='web' \
  -o website/wasm/ilma-compiler.js

echo ""
echo "WASM build complete!"
echo "  JS:   website/wasm/ilma-compiler.js"
echo "  WASM: website/wasm/ilma-compiler.wasm"
echo ""
echo "Deploy the website/ folder to serve the compiler in-browser."
