#!/bin/bash
# ILMA Test Runner
# Runs all .ilma test files that have matching .expected files

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ILMA="$PROJECT_DIR/build/ilma"

if [ ! -f "$ILMA" ]; then
    echo "Error: ILMA compiler not found at $ILMA"
    echo "Run 'make' first to build the compiler."
    exit 1
fi

PASS=0
FAIL=0
SKIP=0
ERRORS=""

run_test() {
    local test_file="$1"
    local expected_file="${test_file%.ilma}.expected"
    local test_name="$(basename "$test_file" .ilma)"

    if [ ! -f "$expected_file" ]; then
        echo "  SKIP  $test_name (no .expected file)"
        SKIP=$((SKIP + 1))
        return
    fi

    # Generate C, compile, and run
    local temp_c="/tmp/ilma_test_$$.c"
    local temp_bin="/tmp/ilma_test_$$"

    # Generate C code
    "$ILMA" --c "$test_file" > "$temp_c" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL  $test_name (compilation to C failed)"
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS\n  FAIL: $test_name (compilation to C failed)"
        rm -f "$temp_c"
        return
    fi

    # Detect module source files needed from generated C
    local module_srcs=""
    local runtime_dir="$PROJECT_DIR/src/runtime"
    if grep -q '#include "modules/finance.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/finance.c"
    fi
    if grep -q '#include "modules/think.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/think.c"
    fi
    if grep -q '#include "modules/body.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/body.c"
    fi
    if grep -q '#include "modules/time_mod.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/time_mod.c"
    fi
    if grep -q '#include "modules/quran.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/quran.c"
    fi
    if grep -q '#include "modules/draw.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/draw.c"
    fi
    if grep -q '#include "modules/number.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/number.c"
    fi
    if grep -q '#include "modules/science.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/science.c"
    fi
    if grep -q '#include "modules/trade.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/trade.c"
    fi
    if grep -q '#include "modules/json.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/json.c"
    fi
    if grep -q '#include "modules/http.h"' "$temp_c" 2>/dev/null; then
        module_srcs="$module_srcs $runtime_dir/modules/http.c"
    fi

    # Compile C code
    gcc -o "$temp_bin" "$temp_c" "$PROJECT_DIR/src/runtime/ilma_runtime.c" \
        $module_srcs -I"$PROJECT_DIR/src/runtime" -lm -lpthread 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL  $test_name (GCC compilation failed)"
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS\n  FAIL: $test_name (GCC compilation failed)"
        rm -f "$temp_c"
        return
    fi

    # Run and capture output
    local actual
    actual=$("$temp_bin" 2>&1)
    local expected
    expected=$(cat "$expected_file")

    # Compare
    if [ "$actual" = "$expected" ]; then
        echo "  PASS  $test_name"
        PASS=$((PASS + 1))
    else
        echo "  FAIL  $test_name"
        echo "        Expected: $(echo "$expected" | head -3)"
        echo "        Got:      $(echo "$actual" | head -3)"
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS\n  FAIL: $test_name"
    fi

    rm -f "$temp_c" "$temp_bin"
}

echo "=== ILMA Test Suite ==="
echo ""

# Run tier 1 tests
echo "--- Tier 1 (Seed) ---"
for f in "$SCRIPT_DIR"/tier1/*.ilma; do
    [ -f "$f" ] && run_test "$f"
done

# Run tier 2 tests if any
if ls "$SCRIPT_DIR"/tier2/*.ilma 1>/dev/null 2>&1; then
    echo ""
    echo "--- Tier 2 (Sapling) ---"
    for f in "$SCRIPT_DIR"/tier2/*.ilma; do
        [ -f "$f" ] && run_test "$f"
    done
fi

# Run tier 3 tests if any
if ls "$SCRIPT_DIR"/tier3/*.ilma 1>/dev/null 2>&1; then
    echo ""
    echo "--- Tier 3 (Tree) ---"
    for f in "$SCRIPT_DIR"/tier3/*.ilma; do
        [ -f "$f" ] && run_test "$f"
    done
fi

echo ""
echo "=== Results ==="
echo "  Passed:  $PASS"
echo "  Failed:  $FAIL"
echo "  Skipped: $SKIP"

if [ $FAIL -gt 0 ]; then
    echo ""
    echo "Failures:"
    echo -e "$ERRORS"
    exit 1
fi

echo ""
echo "All tests passed!"
exit 0
