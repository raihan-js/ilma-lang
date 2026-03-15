# ILMA Programming Language — Makefile
# ilma-lang.dev

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
LIBDIR  = $(PREFIX)/lib/ilma/runtime

SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/lexer.c \
          $(SRC_DIR)/ast.c \
          $(SRC_DIR)/parser.c \
          $(SRC_DIR)/codegen.c \
          $(SRC_DIR)/runtime/ilma_runtime.c

TARGET  = $(BUILD_DIR)/ilma
ILMA_DEV = ILMA_HOME=$(shell pwd)/src

LSP_SOURCES = $(SRC_DIR)/lsp/lsp_main.c \
              $(SRC_DIR)/lsp/ilma_lsp.c \
              $(SRC_DIR)/lexer.c
LSP_TARGET  = $(BUILD_DIR)/ilma-lsp

.PHONY: all clean install uninstall test test-verbose memcheck lsp

all: $(TARGET)

lsp: $(LSP_TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) -I$(SRC_DIR)/runtime $(LDFLAGS)

$(LSP_TARGET): $(LSP_SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(LSP_SOURCES) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	find tests/ \( -name "*.c" -o -name "*.out" \) -delete 2>/dev/null || true
	find examples/ -name "*.c" -delete 2>/dev/null || true

# ── Installation ──────────────────────────────────────────────────
install: $(TARGET) lsp
	@echo "Installing ilma to $(BINDIR)..."
	@mkdir -p $(BINDIR) $(LIBDIR) $(LIBDIR)/modules
	install -m 755 $(TARGET) $(BINDIR)/ilma
	install -m 755 $(LSP_TARGET) $(BINDIR)/ilma-lsp
	install -m 644 src/runtime/ilma_runtime.c $(LIBDIR)/ilma_runtime.c
	install -m 644 src/runtime/ilma_runtime.h $(LIBDIR)/ilma_runtime.h
	install -m 644 src/runtime/modules/*.c $(LIBDIR)/modules/
	install -m 644 src/runtime/modules/*.h $(LIBDIR)/modules/
	@echo ""
	@echo "ILMA installed successfully!"
	@echo "Run:  ilma --version"
	@echo "Try:  ilma examples/hello.ilma"

uninstall:
	rm -f $(BINDIR)/ilma
	rm -rf $(PREFIX)/lib/ilma
	@echo "ILMA uninstalled."

# ── Testing ──────────────────────────────────────────────────────
test: $(TARGET)
	@echo "Running ILMA test suite..."
	@bash tests/run_tests.sh

test-verbose: $(TARGET)
	@bash tests/run_tests.sh

memcheck: $(TARGET)
	@echo "Running memory checks..."
	@for f in tests/tier1/*.ilma; do \
		echo "Checking $$f..."; \
		valgrind --error-exitcode=1 --leak-check=full $(TARGET) --c "$$f" > /dev/null 2>&1 || \
		echo "  MEMORY ISSUE in $$f"; \
	done
