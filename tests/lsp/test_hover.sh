#!/bin/bash
# Test LSP hover documentation
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LSP="$SCRIPT_DIR/../../build/ilma-lsp"

if [ ! -f "$LSP" ]; then
    echo "SKIP: ilma-lsp not built"
    exit 0
fi

INIT='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}'
OPEN='{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///test.ilma","languageId":"ilma","version":1,"text":"say \"hello\""}}}'
HOVER='{"jsonrpc":"2.0","id":2,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///test.ilma"},"position":{"line":0,"character":1}}}'
SHUTDOWN='{"jsonrpc":"2.0","id":3,"method":"shutdown","params":null}'
EXIT_MSG='{"jsonrpc":"2.0","method":"exit","params":null}'

send_msg() {
    local body="$1"
    local len=${#body}
    printf "Content-Length: %d\r\n\r\n%s" "$len" "$body"
}

RESPONSE=$(echo "$(send_msg "$INIT")$(send_msg "$OPEN")$(send_msg "$HOVER")$(send_msg "$SHUTDOWN")$(send_msg "$EXIT_MSG")" | timeout 5 "$LSP" 2>/dev/null || true)

if echo "$RESPONSE" | grep -q "Print a value"; then
    echo "PASS: LSP hover returns documentation for 'say'"
else
    echo "FAIL: LSP hover did not return expected docs"
    exit 1
fi
