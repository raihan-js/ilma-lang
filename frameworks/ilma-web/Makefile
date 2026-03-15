# ═══════════════════════════════════════════════════════════
# IlmaWeb Framework — Makefile
# ═══════════════════════════════════════════════════════════

CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -Wno-unused-parameter -std=c11 -D_POSIX_C_SOURCE=200809L
LDFLAGS  = -lm

SRC_DIR     = src
RUNTIME_DIR = ../../src/runtime
EXAMPLE_DIR = examples/hello_world
BUILD_DIR   = build

SRCS = $(EXAMPLE_DIR)/app_generated.c \
       $(SRC_DIR)/ilma_http.c          \
       $(SRC_DIR)/ilma_web_bridge.c    \
       $(RUNTIME_DIR)/ilma_runtime.c

INCLUDES = -I$(SRC_DIR) -I$(RUNTIME_DIR)

TARGET = $(BUILD_DIR)/hello_world

.PHONY: all clean compile run example

all: compile

compile: $(TARGET)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(INCLUDES) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: compile
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

example:
	@echo "Build and run the IlmaWeb example:"
	@echo "  make compile"
	@echo "  make run"
	@echo "Then visit http://localhost:3000"
