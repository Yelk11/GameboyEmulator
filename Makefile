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
ROM_HARNESS_BINARY := $(BUILD_DIR)/rom_harness
DEBUGGER_BINARY := $(BUILD_DIR)/debugger
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(OS),Windows_NT)
MKDIR_P = powershell -NoProfile -Command "if (!(Test-Path -LiteralPath '$(BUILD_DIR)')) { New-Item -ItemType Directory -Path '$(BUILD_DIR)' | Out-Null }"
else
MKDIR_P = mkdir -p "$(BUILD_DIR)"
endif

.PHONY: all app test clean test-sanitized rom-test rom-list rom-test-suite debugger

all: app

app: $(APP_BINARY)

test: $(TEST_BINARY)
	./$(TEST_BINARY)

test-sanitized: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
test-sanitized: $(TEST_BINARY)
	./$(TEST_BINARY)

rom-test: $(ROM_HARNESS_BINARY)
	./$(ROM_HARNESS_BINARY) "$(ROM)" $(FRAMES)

rom-list:
	find vendor/gb-test-roms -type f -name '*.gb' | sort

rom-test-suite: $(ROM_HARNESS_BINARY)
	@status=0; \
	for rom in \
		vendor/gb-test-roms/cpu_instrs/cpu_instrs.gb \
		vendor/gb-test-roms/instr_timing/instr_timing.gb \
		vendor/gb-test-roms/mem_timing/mem_timing.gb \
		vendor/gb-test-roms/mem_timing-2/mem_timing.gb \
		vendor/gb-test-roms/interrupt_time/interrupt_time.gb \
		vendor/gb-test-roms/halt_bug.gb \
		vendor/gb-test-roms/oam_bug/oam_bug.gb; do \
		echo "Running $$rom"; \
		./$(ROM_HARNESS_BINARY) "$$rom" $(if $(FRAMES),$(FRAMES),7200) >/tmp/gb_suite.out 2>&1 || rc=$$?; \
		rc=$${rc:-0}; echo "Exit=$$rc"; tail -n 5 /tmp/gb_suite.out; \
		if [ $$rc -ne 0 ]; then status=1; fi; rc=; \
	done; exit $$status

rom-test-cpu-individual: $(ROM_HARNESS_BINARY)
	@status=0; \
	for rom in vendor/gb-test-roms/cpu_instrs/individual/*.gb; do \
		echo "Running $$rom"; \
		./$(ROM_HARNESS_BINARY) "$$rom" $(if $(FRAMES),$(FRAMES),720) >/tmp/gb_rom.out 2>&1 || rc=$$?; \
		rc=$${rc:-0}; echo "Exit=$$rc"; tail -n 4 /tmp/gb_rom.out; \
		if [ $$rc -ne 0 ]; then status=1; fi; rc=; \
	done; exit $$status

debugger: $(DEBUGGER_BINARY)
	./$(DEBUGGER_BINARY) "$(ROM)"

$(APP_BINARY): $(APP_SOURCES)
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL_CFLAGS) $(APP_SOURCES) $(SDL_LIBS) -o $@

$(TEST_BINARY): $(TEST_SOURCES) $(PROJECT_SOURCES) $(UNITY_DIR)/unity.c $(UNITY_DIR)/unity.h $(UNITY_DIR)/unity_internals.h
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCES) $(PROJECT_SOURCES) $(UNITY_DIR)/unity.c -o $@

$(ROM_HARNESS_BINARY): tools/rom_harness.c $(PROJECT_SOURCES)
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/rom_harness.c $(PROJECT_SOURCES) -o $@

$(DEBUGGER_BINARY): tools/debugger.c $(PROJECT_SOURCES)
	$(MKDIR_P)
	$(CC) $(CPPFLAGS) $(CFLAGS) tools/debugger.c $(PROJECT_SOURCES) -o $@

clean:
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -Command "if (Test-Path -LiteralPath '$(BUILD_DIR)') { Remove-Item -LiteralPath '$(BUILD_DIR)' -Recurse -Force }"
else
	rm -rf $(BUILD_DIR)
endif