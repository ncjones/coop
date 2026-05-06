/*
 * Copyright 2026 Nathan Jones
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/*
 * Coop.h — Conventions for object-like C structs.
 *
 * Provides two independent facilities:
 *   1. Reference-counted structs: $struct, $new, $retain, $release, $auto
 *   2. Method dispatch: $call, $
 *
 * See README.md for the conceptual overview, reference-counting conventions,
 * usage examples, and leak detection guidance.
 */

#include <stdlib.h>
#include <assert.h>


/* === Version ============================================================= */

#define COOP_VERSION_MAJOR 0
#define COOP_VERSION_MINOR 1
#define COOP_VERSION_PATCH 0
#define COOP_VERSION \
  (COOP_VERSION_MAJOR * 10000 + COOP_VERSION_MINOR * 100 + COOP_VERSION_PATCH)


/* === Member Name Macros ================================================== */

#define $_BASE_MEMBER _base
#define $_DESTROY_MEMBER _destroy
#define $_REF_COUNT_MEMBER _refs

/** Embedded base struct in an extending $struct. */
#define $BASE $_BASE_MEMBER

/** Optional destructor function pointer. Signature: void (*)(void *self).
 *  Implementations downcast self to the concrete type. */
#define $DESTROY $_DESTROY_MEMBER


/* === $struct ============================================================= */

#define $_STRUCT_NARG(...) $_STRUCT_NARG_(__VA_ARGS__, 2, 1)
#define $_STRUCT_NARG_(_1, _2, N, ...) N

#define $_STRUCT_CAT(a, b) $_STRUCT_CAT_(a, b)
#define $_STRUCT_CAT_(a, b) a##b

#define $_STRUCT_1(name) \
  typedef struct name name; \
  struct name { \
    int $_REF_COUNT_MEMBER; \
    void (*$_DESTROY_MEMBER)(void *self);

#define $_STRUCT_2(name, base_name) \
  typedef struct name name; \
  struct name { base_name $_BASE_MEMBER;

/**
 * Declare a reference-counted struct. Close the body with $end.
 *
 *   $struct(Foo)              — root struct with refcount and $DESTROY.
 *   $struct(FooImpl, Foo)     — extending struct, embeds Foo as $BASE.
 */
#define $struct(...) \
  $_STRUCT_CAT($_STRUCT_, $_STRUCT_NARG(__VA_ARGS__))(__VA_ARGS__)

/** Closes a $struct body. */
#define $end };


/* === Allocation and Initialization ======================================= */

/* In-place designated-initializer assignment. Does not touch the reference
 * count. Internal helper for $new; not part of the public API. */
#define $_init(object, object_type, ...) ({ \
  object_type *_obj = (object); \
  _Pragma("GCC diagnostic push") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-braces\"") \
  _Pragma("GCC diagnostic ignored \"-Wmissing-field-initializers\"") \
  *_obj = (object_type) { \
    __VA_ARGS__ \
  }; \
  _Pragma("GCC diagnostic pop") \
  _obj; \
})

/**
 * Allocate and initialize a new $struct on the heap with refcount 1. The
 * caller owns the initial reference.
 *
 *   Foo *foo = $new(Foo, .method = Foo_method, .$DESTROY = Foo_destroy);
 */
#define $new(object_type, ...) ({ \
  object_type *_new_obj = $_init( \
    malloc(sizeof(object_type)), \
    object_type, \
    ##__VA_ARGS__ \
  ); \
  $_rc_track_retain(); \
  $_rc_trace("New", _new_obj); \
  (($_RcHeader *)_new_obj)->$_REF_COUNT_MEMBER = 1; \
  _new_obj; \
})


/* === Reference Counting ================================================== */

/* Layout shared by every $struct's leading bytes; used by $release and
 * $auto to access the refcount and destructor through a generic pointer. */
typedef struct {
  int $_REF_COUNT_MEMBER;
  void (*$_DESTROY_MEMBER)(void *self);
} $_RcHeader;

/**
 * Global counters incremented by every retain (including $new) and release.
 * Used by tests to detect imbalanced reference counts. See README.md for a
 * Unity setUp/tearDown example.
 */
__attribute__((weak)) int $rc_retains;
__attribute__((weak)) int $rc_releases;

#define $_rc_track_retain()  ($rc_retains++)
#define $_rc_track_release() ($rc_releases++)

/* Compile with -DCOOP_RC_TRACE to print every retain/release/new with file,
 * line, pointer, and resulting count. See README.md for sample output. */
#ifdef COOP_RC_TRACE
#include <stdio.h>
#define $_rc_trace(tag, obj) \
  printf("RC %s  %s:%d  %p  count=%d\n", \
    tag, __FILE__, __LINE__, (void*)(obj), \
    (($_RcHeader*)obj)->$_REF_COUNT_MEMBER)
#else
#define $_rc_trace(tag, obj)
#endif

/**
 * Increment an object's reference count and return the object. Use when
 * storing a borrowed reference into a heap location, or keeping a reference
 * past its current scope.
 */
#define $retain(obj) ({ \
  __typeof__(obj) _obj = (obj); \
  assert(_obj != NULL && "Retain called on null pointer"); \
  $_rc_trace("Retain", _obj); \
  $_rc_track_retain(); \
  (_obj)->$_REF_COUNT_MEMBER++; \
  (_obj); \
})

/**
 * Decrement an object's reference count. When the count reaches zero, calls
 * $DESTROY (if non-NULL) and frees the object. Asserts that the count is
 * positive on entry. NULL is permitted and is a no-op.
 */
#define $release(obj) do { \
  __typeof__(obj) _obj = (obj); \
  if (_obj) { \
    $_rc_trace("Release", _obj); \
    assert(_obj->$_REF_COUNT_MEMBER > 0 && "Release called on unretained object"); \
    $_rc_track_release(); \
    if (--_obj->$_REF_COUNT_MEMBER < 1) { \
      if (_obj->$_DESTROY_MEMBER) _obj->$_DESTROY_MEMBER(_obj); \
      free(_obj); \
    } \
  } \
} while (0)

/**
 * Variable attribute that calls $release on the variable's value when the
 * variable leaves scope. NULL values are skipped.
 *
 *   $auto Foo *foo = $new(Foo, ...);
 */
#define $auto __attribute__((cleanup($_auto_release)))

static inline void $_auto_release(void *pp) {
  void **p = pp;
  $release(($_RcHeader *) *p);
}


/* === Method dispatch ===================================================== */

/**
 * Invoke a method on obj, passing obj as the first argument. Asserts that
 * obj and the method pointer are non-NULL. The receiver is evaluated once.
 *
 *   $call(parser, parse, input);   // parser->parse(parser, input)
 *
 * Works on any struct with a function-pointer member whose first parameter
 * matches the struct type — does not require the struct to be a $struct.
 */
#define $call(obj, method, ...) ({ \
  __typeof__(obj) _obj = (obj); \
  assert(_obj != NULL && "Object is null"); \
  assert(_obj->method != NULL && "Method is not bound"); \
  _obj->method(_obj, ##__VA_ARGS__); \
})

/** Shorthand alias for $call. */
#define $(obj, method, ...) $call(obj, method, ##__VA_ARGS__)
