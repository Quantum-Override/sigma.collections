CC = gcc
STD = c2x
LD  = ld

# Sibling repos this package depends on. Headers only (compile-time) — the
# actual Allocator/Application implementations are consumed as prebuilt
# package objects, not recompiled from source; see test/unit/Makefile for
# the same convention on the test side.
SIGMA_CORE = ../sigma.core

DEPS = .deps
INCLUDE = -Iinclude -I$(DEPS)

# -MMD -MP generate a per-object .d file listing every header it includes,
# so a header-only change correctly triggers a rebuild of every object that
# includes it, not just the .c files touched directly.
CFLAGS = -Wall -Wextra -g -fPIC -std=$(STD) $(INCLUDE) -MMD -MP

SRC_DIR   = src
BUILD_DIR = build/lib
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# The established package convention here (see sigma.core, sigma.memory,
# sigma.test): one relocatable object combining every source, produced via
# `ld -r`, consumed directly by rtest/other packages — not an ar archive.
PACKAGE_OBJ = package/sigma.collections.o

.PHONY: all package deps clean test coverage

all: package

package: $(PACKAGE_OBJ)

$(PACKAGE_OBJ): $(OBJS)
	@mkdir -p $(dir $@)
	$(LD) -r -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | deps
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# sigma.core's package/include splits headers between its own root
# (types.h, guid.h, ...) and an inner sigma.core/ subfolder (allocator.h,
# application.h, ...) — real consumers only see these merged into one
# directory via `cbuild install` to /usr/local/include/sigma.core/, which
# isn't available in every environment this repo is built in. This target
# reproduces that merge locally (symlinks, not copies) so
# `#include <sigma.core/whatever.h>` resolves via a single -I$(DEPS).
deps: $(DEPS)/sigma.core/.stamp

$(DEPS)/sigma.core/.stamp:
	@mkdir -p $(DEPS)/sigma.core
	@for f in $(abspath $(SIGMA_CORE))/package/include/*.h \
	          $(abspath $(SIGMA_CORE))/package/include/sigma.core/*.h; do \
		ln -sf "$$f" $(DEPS)/sigma.core/; \
	done
	@touch $@

-include $(OBJS:.o=.d)

# Delegates to test/unit's own Makefile — see that file for why the test
# suites live in their own small, task-specific Makefile rather than being
# folded into this one. For a single suite, or valgrind, or build-only,
# use test/unit/Makefile directly (e.g. `make -C test/unit list val`).
test:
	$(MAKE) -C test/unit test-all

coverage:
	$(MAKE) -C test/unit coverage

clean:
	rm -rf build $(DEPS)
	rm -f $(PACKAGE_OBJ)
