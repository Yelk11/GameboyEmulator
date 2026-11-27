CC ?= cc
CPPFLAGS ?= -Iinclude -Itests -Ivendor/unity/src
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic
BUILD_DIR ?= build
UNITY_DIR := vendor/unity/src
APP_SOURCE ?= src/gb.c

APP_SOURCES := $(wildcard src/*.c)
PROJECT_SOURCES := $(filter-out $(APP_SOURCE),$(APP_SOURCES))
TEST_SOURCES := $(wildcard tests/*.c)
APP_BINARY := $(BUILD_DIR)/gameboy
TEST_BINARY := $(BUILD_DIR)/test_runner

ifeq ($(OS),Windows_NT)
MKDIR_P = powershell -NoProfile -Command "if (!(Test-Path -LiteralPath '$(BUILD_DIR)')) { New-Item -ItemType Directory -Path '$(BUILD_DIR)' | Out-Null }"
else
MKDIR_P = mkdir -p "$(BUILD_DIR)"
endif

.PHONY: all app test clean test-sanitized

all: app

app: $(APP_BINARY)

test: $(TEST_BINARY)
	./$(TEST_BINARY)

test-sanitized: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
test-sanitized: $(TEST_BINARY)
	./$(TEST_BINARY)

$(APP_BINARY): $(APP_SOURCES)
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP_SOURCES) -o $@

$(TEST_BINARY): $(TEST_SOURCES) $(PROJECT_SOURCES) $(UNITY_DIR)/unity.c $(UNITY_DIR)/unity.h $(UNITY_DIR)/unity_internals.h
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCES) $(PROJECT_SOURCES) $(UNITY_DIR)/unity.c -o $@

clean:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -Command "if (Test-Path -LiteralPath '$(BUILD_DIR)') { Remove-Item -LiteralPath '$(BUILD_DIR)' -Recurse -Force }"
else
	rm -rf $(BUILD_DIR)
endif