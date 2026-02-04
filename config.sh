# Configuration file for build.sh
# Defines build variables sourced by the main build script

CC=gcc
STD=c2x

# Memory checking configuration (declared early for conditional compilation)
# VALGRIND_ENABLED: Set to true if valgrind is installed on the system
VALGRIND_ENABLED=true
# VALGRIND_OPTS: Default valgrind options
VALGRIND_OPTS="--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose"

# ASAN_ENABLED: Set to true to compile with AddressSanitizer
ASAN_ENABLED=false
# ASAN_OPTIONS: Runtime options for AddressSanitizer
ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1:detect_invalid_pointer_pairs=1"

# Base compiler flags
BASE_CFLAGS="-Wall -Wextra -g -fPIC -std=$STD -I./include -I../sigma.core/include"

# Add ASAN flags if enabled
if [ "$ASAN_ENABLED" = true ]; then
    BASE_CFLAGS="$BASE_CFLAGS -fsanitize=address -fsanitize=undefined"
fi

CFLAGS="$BASE_CFLAGS"
TST_CFLAGS="$CFLAGS -DTSTDBG -I/usr/include/sigmatest"
LDFLAGS=""
TST_LDFLAGS="-lstest -L/usr/lib -L/usr/local/packages -l:sigma.core.alloc.o"

SRC_DIR=src
BUILD_DIR=build
BIN_DIR=bin
LIB_DIR="$BIN_DIR/lib"
TEST_DIR=test
TST_BUILD_DIR="$BUILD_DIR/test"

# Bundle definitions:
declare -A PACKAGES=(
    ["collection"]="sigma.collections | arrays array_base collections list parray farray slotarray"
)

# Build target definitions:
declare -A BUILD_TARGETS=(
    ["all"]="compile_only"
    ["compile"]="build_lib"
    ["clean"]="clean"
    ["clean_all"]="clean_all"
    ["install"]="build_lib && install_lib"
    ["test"]="run_all_tests"
    ["root"]="show_project_info"
)

# Special test configurations:
declare -A TEST_CONFIGS=(
    #["test_setname"]="standard"
    #["setname"]="standard"
)

# Special test flags: Flags will be prefixed with -D automatically
declare -A TEST_COMPILE_FLAGS=(
    #[setname]="FLAG_EXAMPLE"
)
