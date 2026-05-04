# Changelog

All notable changes to this project will be documented in this file. The
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.1.0] — Unreleased

Initial public release. Extracted from the byol Lisp project, where the
header had been developed and used as the runtime substrate for an
interpreter.

### Public API

- `$struct(Name)` / `$struct(Name, Base)` / `$end` — declare
  reference-counted structs, with optional single-base extension for
  public/private splits.
- `$new(Type, .field = ...)` — heap-allocate with refcount 1 and
  designated-initializer fields.
- `$retain(obj)` / `$release(obj)` — manual refcount management.
- `$auto` — variable attribute that releases on scope exit.
- `$call(obj, method, args...)` and the `$(...)` shorthand — method
  dispatch that passes the receiver as the first argument exactly once.
- `$DESTROY` / `$BASE` — destructor and embedded-base member-name macros.
- `$rc_retains` / `$rc_releases` — global counters for leak detection in
  tests.
- `COOP_VERSION_MAJOR`, `_MINOR`, `_PATCH`, and combined `COOP_VERSION`
  integer macros for compile-time version checks.
- `-DCOOP_RC_TRACE` — opt-in stdout trace of every retain, release, and
  `$new`.

[0.1.0]: https://github.com/ncjones/coop/releases/tag/v0.1.0
