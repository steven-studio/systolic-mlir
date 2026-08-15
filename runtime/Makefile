# runtime/Makefile
#
# lib/ is the only part the compiler links against: Conv2DToFpga and
# TileMatmulForFpga emit LLVM calls into these symbols, so this set has to
# stay buildable as a unit independently of anything under tests/ or attic/.
# Everything else here is host-side tooling.

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
CPPFLAGS = -Ilib

# systolic_dispatch_sim.c and systolic_dispatch_uart.c define the same two
# symbols (systolic_dispatch_open, systolic_dispatch_matmul), so exactly
# one goes into the archive. Putting both in makes the linker pick whichever
# it reaches first, which is silent and order-dependent.
#   uart -- talks to the board over $SYSTOLIC_PORT, default /dev/ttyUSB2
#   sim  -- host-side model, builds and runs with no board attached
DISPATCH ?= uart

LIB_SRC = $(filter-out lib/systolic_dispatch_%.c,$(wildcard lib/*.c)) \
          lib/systolic_dispatch_$(DISPATCH).c
LIB_OBJ = $(LIB_SRC:.c=.o)
LIB     = lib/libfpgart.a

BIN_DIR = bin
# Was a hardcoded list, so a new driver silently did not get built. The
# tests/ rule below already globbed; drivers/ now matches it.
DRIVERS = $(patsubst drivers/%.c,$(BIN_DIR)/%,$(wildcard drivers/*.c))
TESTS   = $(patsubst tests/%.c,$(BIN_DIR)/%,$(wildcard tests/*.c))

.PHONY: all lib drivers tests clean check
all: lib drivers tests

lib: $(LIB)
$(LIB): $(LIB_OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

drivers: $(DRIVERS)
tests: $(TESTS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: drivers/%.c $(LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB) -o $@

$(BIN_DIR)/%: tests/%.c $(LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB) -o $@

# The four configurations that were the original high-error suspects and are
# now the standing bit-exactness check. Needs the board programmed.
check: drivers
	./$(BIN_DIR)/run_fold_shape  8  64  8
	./$(BIN_DIR)/run_fold_shape  8  65  8
	./$(BIN_DIR)/run_fold_shape  8 100  8
	./$(BIN_DIR)/run_fold_shape  8 128  8
	./$(BIN_DIR)/run_fold_shape 24 100 16

clean:
	rm -f $(LIB_OBJ) $(LIB)
	rm -rf $(BIN_DIR)
