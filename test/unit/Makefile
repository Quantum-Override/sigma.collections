CC  = gcc
STD = c2x

# Sibling repos this test suite depends on. sigma.core is a compile-time
# header dependency (see the `deps` target below); sigma.memory and
# sigma.test are consumed as prebuilt package objects — genuinely separate
# packages, not vendored source, so they're linked, not recompiled here.
SIGMA_COLLECTIONS = ../..
SIGMA_CORE        = ../../../sigma.core
SIGMA_MEMORY      = ../../../sigma.memory
SIGMA_TEST        = ../../../sigma.test

DEPS = .deps
INCLUDE = -I$(SIGMA_COLLECTIONS)/include -I$(DEPS)
CFLAGS = -Wall -Wextra -g -std=$(STD) $(INCLUDE) -DTSTDBG

LIB_SRCS = $(wildcard $(SIGMA_COLLECTIONS)/src/*.c)

DEP_OBJS = $(SIGMA_CORE)/package/sigma.core.o \
           $(SIGMA_MEMORY)/package/sigma.memory.o \
           $(SIGMA_TEST)/package/sigma.test.o

BIN = bin
VALGRIND_LOG = $(BIN)/valgrind_test.log
UNIT_CASES = $(patsubst test_%.c,%,$(wildcard test_*.c))

# Same approach as Anvil's test Makefiles: gcc namespaces each source's
# .gcno/.gcda by the binary's own name when compiling multiple sources into
# one -o target with --coverage, so every suite's coverage data coexists in
# bin/ without colliding.
COVERAGE_DIR = coverage-html
COVERAGE_INFO = $(BIN)/coverage.info
COVERAGE_FILTERED = $(BIN)/coverage.filtered.info

.PHONY: coverage
coverage:
	@$(MAKE) clean
	@$(MAKE) all CFLAGS="$(CFLAGS) --coverage"
	@echo "-- running every suite to collect coverage data --"
	@for t in $(BIN)/test_*; do "$$t" >/dev/null 2>&1 || true; done
	lcov --capture --directory $(BIN) --output-file $(COVERAGE_INFO) --rc branch_coverage=0
	lcov --remove $(COVERAGE_INFO) '*/sigma.core/*' '*/sigma.memory/*' '*/sigma.test/*' \
		-o $(COVERAGE_FILTERED)
	genhtml $(COVERAGE_FILTERED) --output-directory $(BIN)/$(COVERAGE_DIR)
	@echo "Coverage report: $(BIN)/$(COVERAGE_DIR)/index.html"

define run_test_bin
	@if [ -n "$(filter build,$(MAKECMDGOALS))" ]; then \
		echo "-- build only: $(1) --"; \
	elif [ -n "$(filter val,$(MAKECMDGOALS))" ]; then \
		mkdir -p $(BIN); \
		echo "-- valgrind: $(1) --"; \
		valgrind --leak-check=full --track-origins=yes \
			--error-exitcode=42 \
			--log-file=$(VALGRIND_LOG) \
			$(BIN)/$(1); \
		echo "Valgrind log: $(VALGRIND_LOG)"; \
	else \
		$(BIN)/$(1); \
	fi
endef

.PHONY: all clean val build test test-all deps
.PHONY: build_test_collections build_test_collections_allocator build_test_farray
.PHONY: build_test_indexarray build_test_iterator build_test_list build_test_map
.PHONY: build_test_parray build_test_query build_test_slotarray build_test_stack
.PHONY: test_collections test_collections_allocator test_farray test_indexarray
.PHONY: test_iterator test_list test_map test_parray test_query test_slotarray test_stack
.PHONY: $(UNIT_CASES)

all: $(BIN)/test_collections \
     $(BIN)/test_collections_allocator \
     $(BIN)/test_farray \
     $(BIN)/test_indexarray \
     $(BIN)/test_iterator \
     $(BIN)/test_list \
     $(BIN)/test_map \
     $(BIN)/test_parray \
     $(BIN)/test_query \
     $(BIN)/test_slotarray \
     $(BIN)/test_stack

# sigma.core's package/include splits headers between its own root
# (types.h, guid.h, ...) and an inner sigma.core/ subfolder (allocator.h,
# application.h, ...) — real consumers only see these merged into one
# directory via `cbuild install` to /usr/local/include/sigma.core/, which
# isn't available in every environment this repo is built in. This target
# reproduces that merge locally (symlinks, not copies) so
# `#include <sigma.core/whatever.h>` resolves via a single -I$(DEPS).
# sigma.test's package/include has no such split, so it's one directory
# symlink.
deps: $(DEPS)/sigma.core/.stamp $(DEPS)/sigma.test

$(DEPS)/sigma.core/.stamp:
	@mkdir -p $(DEPS)/sigma.core
	@for f in $(abspath $(SIGMA_CORE))/package/include/*.h \
	          $(abspath $(SIGMA_CORE))/package/include/sigma.core/*.h; do \
		ln -sf "$$f" $(DEPS)/sigma.core/; \
	done
	@touch $@

$(DEPS)/sigma.test:
	@mkdir -p $(DEPS)
	@ln -sfn $(abspath $(SIGMA_TEST))/package/include $@

$(BIN)/test_collections: test_collections.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_collections: $(BIN)/test_collections

test_collections: $(BIN)/test_collections
	$(call run_test_bin,test_collections)

$(BIN)/test_collections_allocator: test_collections_allocator.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_collections_allocator: $(BIN)/test_collections_allocator

test_collections_allocator: $(BIN)/test_collections_allocator
	$(call run_test_bin,test_collections_allocator)

$(BIN)/test_farray: test_farray.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_farray: $(BIN)/test_farray

test_farray: $(BIN)/test_farray
	$(call run_test_bin,test_farray)

$(BIN)/test_indexarray: test_indexarray.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_indexarray: $(BIN)/test_indexarray

test_indexarray: $(BIN)/test_indexarray
	$(call run_test_bin,test_indexarray)

$(BIN)/test_iterator: test_iterator.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_iterator: $(BIN)/test_iterator

test_iterator: $(BIN)/test_iterator
	$(call run_test_bin,test_iterator)

$(BIN)/test_list: test_list.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_list: $(BIN)/test_list

test_list: $(BIN)/test_list
	$(call run_test_bin,test_list)

$(BIN)/test_map: test_map.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_map: $(BIN)/test_map

test_map: $(BIN)/test_map
	$(call run_test_bin,test_map)

$(BIN)/test_parray: test_parray.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_parray: $(BIN)/test_parray

test_parray: $(BIN)/test_parray
	$(call run_test_bin,test_parray)

$(BIN)/test_query: test_query.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_query: $(BIN)/test_query

test_query: $(BIN)/test_query
	$(call run_test_bin,test_query)

$(BIN)/test_slotarray: test_slotarray.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_slotarray: $(BIN)/test_slotarray

test_slotarray: $(BIN)/test_slotarray
	$(call run_test_bin,test_slotarray)

$(BIN)/test_stack: test_stack.c $(LIB_SRCS) $(DEP_OBJS) | deps
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@

build_test_stack: $(BIN)/test_stack

test_stack: $(BIN)/test_stack
	$(call run_test_bin,test_stack)

# Build and run every suite in one pass, continuing past a failing/crashing
# suite instead of stopping at the first one — both a suite that fails to
# *build* (make -k keeps going past that one target) and a suite that
# crashes at *runtime* (each suite is its own process, so one crashing —
# see test_slotarray's own notes — doesn't take the others down with it).
# Prints a pass/fail summary line per suite.
test-all:
	@$(MAKE) -k all || true
	@failed=0; \
	for t in $(UNIT_CASES); do \
		printf '%-24s ' "$$t"; \
		if $(BIN)/test_$$t >/dev/null 2>&1; then \
			echo "PASS"; \
		else \
			echo "FAIL (exit $$?)"; \
			failed=1; \
		fi; \
	done; \
	exit $$failed

# Shorthand aliases: make list [val|build] -> make test_list [val|build]
$(UNIT_CASES): %: test_%
	@:

# Mode-switch goal for: make test_list val
val:
	@:

# Mode-switch goal for: make test_list build
build:
	@:

test:
	@:

clean:
	rm -rf $(BIN) $(DEPS)
