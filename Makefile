.PHONY: all build test clean

sanitize ?= true
rc_trace ?=

# Apple clang ships compiler-rt without LeakSanitizer; auto-select Homebrew
# clang on Darwin so `make sanitize=true test` includes LSan. Override CC
# explicitly to opt out.
ifeq ($(shell uname -s),Darwin)
  ifeq ($(origin CC),default)
    CC := /opt/homebrew/opt/llvm/bin/clang
  endif
endif

# -Wpedantic is intentionally omitted: coop is built on GNU extensions
# ($-identifiers, ({...}) statement expressions, ##__VA_ARGS__) by design.
# See README.md for the portability story.
CFLAGS  = -g -O0 -Wall -Wextra -Werror \
          -DUNITY_INCLUDE_CONFIG_H \
          -I. -Ivendor -Ivendor/unity
LDFLAGS =

ifeq ($(sanitize),true)
  CFLAGS  += -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
  # Apple clang's ASan rejects detect_leaks (no LSan); export only on Linux.
  # Homebrew clang on macOS enables LSan by default.
  ifeq ($(shell uname -s),Linux)
    export ASAN_OPTIONS = detect_leaks=1
  endif
endif

ifeq ($(rc_trace),true)
  CFLAGS += -DCOOP_RC_TRACE
endif

units = tests/main.c tests/coop_test.c vendor/unity/unity.c
objs  = $(units:%.c=build/%.o)

all: test

build: build/coop_test

test: build/coop_test
	./$<

build/coop_test: $(objs)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@

# Vendor TUs (Unity) compile without -Werror -Wpedantic; the upstream sources
# trip benign warnings under our strict flag set and aren't ours to fix.
build/vendor/unity/%.o: vendor/unity/%.c
	@mkdir -p $(dir $@)
	$(CC) $(filter-out -Werror -Wpedantic,$(CFLAGS)) -Wno-deprecated-declarations -c $< -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
