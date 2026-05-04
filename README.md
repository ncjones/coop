# Coop.h

A minimal macro vocabulary for writing object-like code in C. Single header, no
dependencies beyond `stdlib.h` and `assert.h`. Supports encapsulation and
polymorphism through method dispatch, information hiding through struct
extension, and decoupled object lifecycles through reference counting.

## Installation

Coop is a single header file. Copy `coop.h` from this repo into your tree at
whatever path you prefer (e.g. `third_party/coop/coop.h`), and adjust your
include path accordingly.

## Usage

The following small program demonstrates information hiding, automatic
reference counting, and abstract method dispatch with Coop.h:

```c
// Account.h — public interface
#include <coop/coop.h>

$struct(Account)
  int (*get_balance)(Account *self);
  void (*deposit)(Account *self, int amount);
$end

Account *Account_new(int initial);
```

```c
// Account.c — private implementation
#include "Account.h"

$struct(AccountImpl, Account)
  int balance;
$end

static int Account_get_balance(Account *self) {
  return ((AccountImpl *)self)->balance;
}

static void Account_deposit(Account *self, int amount) {
  ((AccountImpl *)self)->balance += amount;
}

Account *Account_new(int initial) {
  AccountImpl *impl = $new(AccountImpl,
    .$BASE.get_balance = Account_get_balance,
    .$BASE.deposit = Account_deposit,
    .balance = initial,
  );
  return (Account *)impl;
}
```

```c
// main.c
void demo(void) {
  $auto Account *a = Account_new(100);
  $(a, deposit, 50);
  printf("%d\n", $(a, get_balance));   // prints 150
}
```

Code outside `Account.c` sees only `Account` — it cannot access or even
know the size of `AccountImpl`'s private `balance` field. The `$(...)`
syntax dispatches `deposit` and `get_balance` polymorphically through the
abstract `Account` interface, and `$auto` releases `a` when `demo`
returns.

## Portability

Coop.h is built on GNU C extensions and works with GCC and Clang. It is not
portable to MSVC. Specifically:

- **`$`-prefixed identifiers** require `-fdollars-in-identifiers` (on by
  default in GCC/Clang). Compiling under `-Wpedantic` flags this and the
  other GNU extensions coop relies on (`({...})` statement expressions,
  `, ##__VA_ARGS__`); compile coop with `-Wall -Wextra` instead, or
  whitelist the relevant extension warnings.
- **`$auto`** uses the `cleanup` variable attribute, which MSVC does not
  support.

## Macros

Coop.h's API is a small set of macros grouped into three features: method
dispatch, struct declaration, and reference counting.

### Method Dispatch: `$call`, `$`

In plain C, calling a function-pointer member on a struct requires
mentioning the receiver twice — once to dereference the pointer, once as
the explicit `self` argument: `parser->parse(parser, input)`.

`$call(obj, method, args...)` collapses this to a single mention,
automatically passing the receiver as the first argument. `$` is a
shorthand alias:

```c
$(parser, parse, input);
// expands to: parser->parse(parser, input)
```

Eliminating the redundant receiver decouples callers from concrete
implementations and enables polymorphism and encapsulation with minimal
syntactical burden.

The macro asserts that `obj` is non-`NULL` and that the named method is
bound, and evaluates the receiver expression exactly once. It works on any
struct with self-first-arg function pointers; the receiver does not need to
be a `$struct`.

### Structs: `$struct`, `$end`, `$BASE`

Declare struct types using a leaner syntax than vanilla C, with a typedef
that can be self-referenced in method declarations and a public/private
split that supports information hiding.

`$struct(Name)` declares a reference-counted struct type and a typedef of
the same name. The body lists fields and method pointers, terminated with
`$end`:

```c
$struct(Foo)
  int x;
  void (*bar)(Foo *self);
$end
```

`$struct(Name, Base)` declares a struct that extends another, embedding the
base struct as its first member, accessible via `$BASE`:

```c
$struct(FooImpl, Foo)
  int private_field;
$end
```

The extending struct inherits the base's reference-count and destructor
infrastructure automatically. Because the base is the first member, a
pointer to the extending struct can be cast to a pointer to the base:

```c
FooImpl *impl = ...;
Foo *foo = (Foo *)impl;   // safe — $BASE is the first member
```

Method implementations on the extending struct downcast `self` back to the
extending type to access private fields.

Extension is only ergonomic for a public/private split — extending one
concrete type with another forces every base member through `$BASE`.

### Reference Counting: `$new`, `$retain`, `$release`, `$auto`, `$DESTROY`

Track object references and free objects automatically when the last
reference is released, decoupling an object's lifecycle from the code that
uses it.

`$new(Type, .field = value, ...)` allocates a `$struct` instance with
designated-initializer syntax and returns an owned reference (refcount = 1).
The optional `.$DESTROY` field assigns a destructor function pointer.

`$retain(obj)` increments the refcount and returns the receiver, so it can
be used inline (e.g. `.field = $retain(arg)`).

`$release(obj)` decrements the refcount and, when it reaches zero, calls
`$DESTROY` (if set) and frees the struct. `$release` is not NULL-safe;
guard with `if (obj) $release(obj)` when the value may be NULL.

`$auto` is a variable attribute that calls `$release` automatically when the
variable leaves scope. NULL values at scope exit are skipped. If the
variable is reassigned during the scope, only the final value is released;
earlier values must be released manually before reassignment.

`$DESTROY` is the destructor function-pointer field on every `$struct`.
Its signature is `void (*)(void *self)`; the implementation downcasts
`self` to the concrete type before touching its fields. Assign it in
`$new` to release owned fields or clean up other resources. If left
`NULL`, `$release` simply frees the struct.

The destructor takes `void *` rather than the concrete type to avoid
calling through an incompatible function-pointer type during `$auto`
cleanup, which the C standard considers undefined behavior even when
the underlying call ABI works in practice.

## Reference Counting Protocol

The rules below govern when references must be retained and released.
Coop.h's safety properties depend on every function in the chain agreeing
on these rules — they are checked locally at each function boundary, not
enforced by the compiler.

### Owned vs Borrowed References

Reference counting obligations follow a single rule: **owned** references
must be released, **borrowed** references must not.

1. Objects returned from a function are **owned** by the caller. The caller
   must eventually release the object, or transfer that responsibility by
   returning it onward.

2. Every other reference is **borrowed** — function arguments, struct
   fields, array elements, globals. The receiver must not release them
   unless it has first retained them.

### Returning Owned References

A returned reference must be retained on behalf of the caller before it is
returned. If the returner still holds the reference after returning, an
explicit `$retain` is required. If the returner releases the returned
reference then ownership is effectively *transferred*; the retain-for-return
and the release-of-local cancel out so no additional retain/release pair is
needed.

```c
Foo *make_foo(void) {
  Foo *foo = $new(Foo, ...);   // local owns one reference (refcount = 1)
  return foo;                  // ownership transferred, no retain needed
}
```

### Storing Borrowed References

Storing a borrowed reference into a heap-allocated location — a struct
field, a collection slot, a global — requires calling `$retain` to take
ownership of the new reference, and then `$release` when that reference is
overwritten or the containing object is destroyed.

```c
void Container_set_item(Container *self, Item *item) {
  $retain(item);
  if (self->item) $release(self->item);
  self->item = item;
}
```

### What Is Not Reference Counted

Reference counting applies only to `$struct` types. Everything else — plain
structs, raw arrays, C strings — is outside the system.

Plain structs lack the reference-count field that `$retain` and `$release`
rely on; the macros fail to compile against them. When defining a plain
struct, document its intended memory model in a header comment so callers
know how its lifetime should be managed (stack-scoped, arena-bound,
caller-owned).

Raw arrays, C strings, and other non-`$struct` data do not participate in
reference counting at all. To share such data across reference boundaries,
either copy it or wrap it in a `$struct` whose destructor frees it.

## Recipes

Tips for how to use Coop.h effectively.

### Information Hiding via Extension

Declare the public interface as a `$struct` in a header file, then extend
it privately in the implementation file. The factory function allocates
the extending struct with `$new` and wires the public method pointers
through `.$BASE`, returning a pointer cast to the public type.

```c
// Counter.h — public interface
$struct(Counter)
  int (*get)(Counter *self);
  void (*increment)(Counter *self);
$end

Counter *Counter_new(void);
```

```c
// Counter.c — private implementation
#include "Counter.h"

$struct(CounterImpl, Counter)
  int value;
$end

static int Counter_get(Counter *self) {
  return ((CounterImpl *)self)->value;
}

static void Counter_increment(Counter *self) {
  ((CounterImpl *)self)->value++;
}

Counter *Counter_new(void) {
  CounterImpl *impl = $new(CounterImpl,
    .$BASE.get = Counter_get,
    .$BASE.increment = Counter_increment,
  );
  return (Counter *)impl;
}
```

Code that includes only `Counter.h` sees the public type and its method
pointers but knows nothing about `CounterImpl`'s private `value` field.
Method implementations downcast `self` back to `CounterImpl` to read or
mutate it.

### Plain Struct Receivers

`$call` doesn't require its receiver to be a `$struct`. Any struct with a
function-pointer member whose first parameter matches the struct type
works:

```c
typedef struct Greeter Greeter;
struct Greeter {
  const char *name;
  void (*hello)(Greeter *self);
};

static void Greeter_hello(Greeter *self) {
  printf("Hello, %s\n", self->name);
}

void demo(void) {
  Greeter g = { .name = "world", .hello = Greeter_hello };
  $(&g, hello);   // prints "Hello, world"
}
```

This is useful for stack-, arena-, or caller-managed receivers.

### Releasing Owned References in a Destructor

If a `$struct` holds retained references in its fields, it must release
them in its destructor. Forgetting `$DESTROY` on a struct that owns
references silently leaks them.

```c
$struct(Container)
  Item *item;   // owned reference
$end

static void Container_destroy(void *self_) {
  Container *self = self_;
  if (self->item) $release(self->item);
}

Container *Container_new(Item *item) {
  return $new(Container,
    .$DESTROY = Container_destroy,
    .item = $retain(item),
  );
}
```

### Freeing Non-RC Resources in a Destructor

A destructor is also where non-reference-counted resources are released —
malloc'd buffers, open file handles, anything outside the RC system.

```c
$struct(Logger)
  FILE *file;
  char *buffer;   // malloc'd, owned by Logger
$end

static void Logger_destroy(void *self_) {
  Logger *self = self_;
  if (self->file) fclose(self->file);
  free(self->buffer);
}

Logger *Logger_new(const char *path) {
  return $new(Logger,
    .$DESTROY = Logger_destroy,
    .file = fopen(path, "w"),
    .buffer = malloc(4096),
  );
}
```

### Avoiding Cycles with Borrowed Back-Pointers

Pure reference counting cannot reclaim cycles, so any back-pointer from a
child to its parent must be a borrowed reference rather than a retained
one. The child relies on the parent outliving it; the parent owns the
child outright.

```c
$struct(Parent)
  Child *child;   // owned
$end

$struct(Child)
  Parent *parent;   // borrowed — not retained
$end

static void Parent_destroy(void *self_) {
  Parent *self = self_;
  if (self->child) $release(self->child);
}

// Child has no $DESTROY — it does not own its parent reference.
```

This pattern is safe only when the child's lifetime is bounded by the
parent's. If the child can outlive the parent, the back-pointer becomes a
dangling pointer.

### Auto-Release for Locals

`$auto` is the preferred way to manage local owned references — it
eliminates manual release at every exit path.

```c
void demo(void) {
  $auto Account *a = Account_new(100);
  $(a, deposit, 50);
  // $release(a) called automatically here
}
```

## Diagnostics

Built-in tooling for catching leaks and tracing reference activity during
development.

### Leak Detection

The `$rc_retains` and `$rc_releases` global counters are incremented by
every retain (including `$new`) and every release. Reset them in `setUp`
and assert equality in `tearDown`:

```c
#include "unity/unity.h"
#include <coop/coop.h>

void setUp(void) {
  $rc_retains = 0;
  $rc_releases = 0;
}

void tearDown(void) {
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    $rc_retains,
    $rc_releases,
    "Unreleased object detected — missing $release?"
  );
}
```

The counters track totals across all `$struct` types and are not
thread-safe. They are intended for single-threaded unit tests, not
production diagnostics.

### Tracing

Compile with `-DCOOP_RC_TRACE` to print every retain, release, and `$new`
to stdout with file, line, pointer, and resulting count:

```log
RC New      Account.c:42  0x7fb8a4001230  count=1
RC Retain   main.c:17     0x7fb8a4001230  count=2
RC Release  main.c:23     0x7fb8a4001230  count=1
RC Release  main.c:25     0x7fb8a4001230  count=0
```

Useful for diagnosing imbalanced counts after a leak-detection assertion
fails — the output tells you where each retain happened, so you can find
the one without a matching release.

## Versioning

`coop.h` exposes its version as integer macros for compile-time checks:

```c
#include <coop/coop.h>

#if COOP_VERSION < 100   // i.e. < 0.1.0
#error "coop ≥ 0.1.0 required"
#endif
```

`COOP_VERSION` packs major, minor, and patch as
`MAJOR * 10000 + MINOR * 100 + PATCH`. The individual components are
also exposed as `COOP_VERSION_MAJOR`, `COOP_VERSION_MINOR`, and
`COOP_VERSION_PATCH`.

## Limitations

**Ref-Counted Cycles Leak** Pure reference counting cannot reclaim cyclic
references. If object A retains B and B retains A, releasing both external
references leaves the cycle alive in memory. Avoid cycles by using borrowed
(non-retained) references for back-pointers.

**Not Thread-Safe** Refcount mutation is non-atomic. Sharing `$struct`
objects across threads requires external synchronization.

**MSVC Not Supported** Coop relies on GNU C extensions (the `cleanup`
variable attribute, statement expressions, dollar identifiers, and
zero-argument variadic macros). GCC and Clang both support all of them;
MSVC supports none of them.

## Why Reference Counting?

Heap allocation is essential when an object needs to outlive its creating
context or when its clients cannot know its concrete type (as is the case
with Coop.h's information hiding). Heap allocation brings challenges though
for managing object lifecycles. Adding a new dependency on a heap-allocated
object can break prior assumptions about when the object should be
destroyed. Those assumptions are frequently located far away from the code
being changed which makes them particularly error-prone.

The object lifecycle problem can be solved with arenas, garbage collection
(GC), or reference counting (RC). Arenas are excellent for transient
allocations bounded by a known scope, but objects with shared ownership or
unpredictable lifetimes outlive any single arena. Tracing GC handles
arbitrary lifetimes and cycles but adds runtime infrastructure and
unpredictable pause behavior.

Coop.h uses reference counting rather than arenas or tracing GC. It provides
predictable per-operation cost, no runtime, and ownership rules that can be
checked locally at each function boundary. The cost is that memory
management is spread throughout the codebase — every function that holds a
reference participates in the protocol — rather than pushed to a boundary
(arenas) or eliminated from application code (GC).

## Legal

Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

Copyright 2026 Nathan Jones.
