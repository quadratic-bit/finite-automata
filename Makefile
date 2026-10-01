CC := clang

WARNFLAGS := \
	-Wall -Wextra -Wpedantic \
	-Wconversion -Wsign-conversion \
	-Wshadow -Wformat=2 -Wundef \
	-Wcast-qual -Wstrict-prototypes \
	-Wmissing-prototypes -Wswitch \
	-Wimplicit-fallthrough -Wvla

COMMON_CFLAGS := -std=c17 $(WARNFLAGS)

DEBUG_CFLAGS := -g -fno-omit-frame-pointer -fsanitize=address,undefined

RELEASE_CFLAGS := -O2 -DNDEBUG

CPPFLAGS := -Iinclude
COVFLAGS := -fprofile-instr-generate -fcoverage-mapping

SRC      := $(wildcard src/*.c)
TEST_SRC := $(wildcard tests/*.c) $(wildcard tests/helpers/*.c)
LIB_SRC  := $(filter-out src/main.c,$(SRC))

.PHONY: release test coverage coverage-show clean

build/automata: $(SRC) | build
	$(CC) $(COMMON_CFLAGS)  $(DEBUG_CFLAGS) $(CPPFLAGS) $(SRC) -o $@

build/automata-release: $(SRC) | build
	$(CC) $(COMMON_CFLAGS) $(RELEASE_CFLAGS) $(CPPFLAGS) $(SRC) -o $@

release: build/automata-release

build/tests: $(LIB_SRC) $(TEST_SRC) | build
	$(CC) \
		$(COMMON_CFLAGS) \
		$(DEBUG_CFLAGS) \
		$(CPPFLAGS) \
		$(LIB_SRC) \
		$(TEST_SRC) \
		-lcriterion \
		-o $@

test: build/tests
	./build/tests

build/tests-cov: $(LIB_SRC) $(TEST_SRC) | build
	$(CC) \
		$(COMMON_CFLAGS) \
		$(DEBUG_CFLAGS) \
		$(COVFLAGS) \
		$(CPPFLAGS) \
		$(LIB_SRC) \
		$(TEST_SRC) \
		-lcriterion \
		-o $@

coverage: build/tests-cov
	rm -f build/coverage-*.profraw build/coverage.profdata
	LLVM_PROFILE_FILE="build/coverage-%p.profraw" ./build/tests-cov
	llvm-profdata merge -sparse build/coverage-*.profraw -o build/coverage.profdata
	llvm-cov report ./build/tests-cov -instr-profile=build/coverage.profdata $(LIB_SRC)

coverage-show: coverage
	llvm-cov show ./build/tests-cov \
		-instr-profile=build/coverage.profdata \
		-show-line-counts-or-regions \
		$(LIB_SRC)

build:
	mkdir -p build

clean:
	rm -rf build
