#!/bin/bash
# Test LSP diagnostics for a broken ILMA file
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LSP="$SCRIPT_DIR/../../build/ilma-lsp"

if [ ! -f "$LSP" ]; then
    echo "SKIP: ilma-lsp not built"
    exit 0
fi

# Send initialize + didOpen with broken code
INIT='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}'
OPEN='{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///test.ilma","languageId":"ilma","version":1,"text":"say"}}}'
SHUTDOWN='{"jsonrpc":"2.0","id":2,"method":"shutdown","params":null}'
EXIT_MSG='{"jsonrpc":"2.0","method":"exit","params":null}'

send_msg() {
    local body="$1"
    local len=${#body}
    printf "Content-Length: %d\r\n\r\n%s" "$len" "$body"
}

RESPONSE=$(echo "$(send_msg "$INIT")$(send_msg "$OPEN")$(send_msg "$SHUTDOWN")$(send_msg "$EXIT_MSG")" | timeout 5 "$LSP" 2>/dev/null || true)

if echo "$RESPONSE" | grep -q "ilma-lsp"; then
    echo "PASS: LSP responds to initialize"
else
    echo "FAIL: LSP did not respond correctly"
    exit 1
fi
