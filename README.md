# GameboyEmulator
Gameboy emulator in c

## Running unit tests

The project uses [Unity](https://github.com/ThrowTheSwitch/Unity) for C unit
tests. A C compiler and GNU Make are required. Initialize Unity after cloning
the repository:

```sh
git submodule update --init --recursive
```

Run `make` to build the application, including `src/gb.c`, or `make test` to
run the test suite. The initial smoke test verifies the Unity test runner is
connected; add test functions under `tests/` and register them with `RUN_TEST`
in `tests/test_runner.c`. The test executable includes project sources except
the application entry point (`src/gb.c`) to avoid linking its `main` alongside
the test runner.

Load a ROM from the command line:

```sh
./build/gameboy path/to/game.gb
```

Run AddressSanitizer and UndefinedBehaviorSanitizer checks with:

```sh
make test-sanitized
```

Run an external ROM validation harness:

```sh
make rom-test ROM=path/to/test.gb FRAMES=7200
```

The harness runs at most the requested number of frames (default: 7200),
captures Blargg-style serial output, and reports pass, fail, hanging, or
illegal-opcode results.

## Controls

| Game Boy button | Keyboard |
| --- | --- |
| D-pad | Arrow keys |
| A | Z |
| B | X |
| Start | Enter |
| Select | Left/Right Shift |
| Exit | Escape |

## Debugger shortcuts

| Action | Keyboard |
| --- | --- |
| Pause/resume | F9 |
| Step one instruction | F10 |
| Step one frame | F11 |
| Opcode trace | Enable through the debugger API or trace mode |
| Save state slot 1 | F5 |
| Load state slot 1 | F6 |

Start the text debugger with:

```sh
make debugger ROM=path/to/game.gb
```

Commands include `step`, `frame`, `run`, `breakpoint XXXX`, `cpu`,
`ppu`, `mem XXXX`, `trace`, and `quit`.
