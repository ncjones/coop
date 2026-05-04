# Contributing

Thanks for your interest in Coop.h. This is a small single-author
project, so the bar for changes is high — but useful patches are
welcome.

## Development

Coop.h is a single header. The repo is laid out as:

- `coop.h` — the library
- `tests/coop_test.c` — unit tests using Unity
- `tests/main.c` — hand-rolled test runner (Unity entry point + crash
  handler)
- `vendor/unity/` — bundled copy of Unity (MIT)
- `Makefile` — builds and runs the test suite

### Build and Test

```sh
make test
```

This compiles `tests/coop_test.c` with sanitizers (AddressSanitizer +
UndefinedBehaviorSanitizer) and LeakSanitizer, runs the suite, and
reports pass/fail.

On macOS the Makefile auto-selects Homebrew Clang
(`/opt/homebrew/opt/llvm/bin/clang`) because Apple Clang ships a
compiler-rt without LeakSanitizer. Set `CC` explicitly to opt out.

### Other Targets

- `make build` — compile the test binary without running it
- `make clean` — remove the `build/` directory
- `make sanitize=false test` — skip sanitizers (faster, useful when
  attaching a debugger)
- `make rc_trace=true test` — define `COOP_RC_TRACE` so every retain,
  release, and `$new` prints to stdout

## Pull Requests

- Keep changes small and focused. One PR, one logical change.
- Add tests for any new public behavior. Existing tests in
  `tests/coop_test.c` are the model — small, single-purpose, named
  after the property they assert.
- Run `make test` before pushing. CI (`{gcc, clang} × ubuntu-latest`)
  will run the same suite with sanitizers; please make sure it passes
  there too.
- The library is intentionally small. Proposals to add new macros or
  expand the public surface should explain the validated use case;
  prefer recording a use-case in an issue first.

## License

By contributing, you agree that your contributions will be licensed
under the Apache License 2.0 (see `LICENSE`).
