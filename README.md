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
