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
# symbols (systolic_dispatch_open, systolic_dispatch_matmul4x4), so exactly
# one goes into the archive. Putting both in makes the linker pick whichever
# it reaches first, which is silent and order-dependent.
#   uart -- talks to the board over /dev/ttyUSB1
#   sim  -- host-side model, builds and runs with no board attached
DISPATCH ?= uart

LIB_SRC = $(filter-out lib/systolic_dispatch_%.c,$(wildcard lib/*.c)) \
          lib/systolic_dispatch_$(DISPATCH).c
LIB_OBJ = $(LIB_SRC:.c=.o)
LIB     = lib/libfpgart.a

BIN_DIR = bin
DRIVERS = $(BIN_DIR)/run_conv2d_shape $(BIN_DIR)/run_shape
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
	cd harness && python3 run_conv2d_sweep.py --only 004,029,032,042 \
	    --bin ../$(BIN_DIR)/run_conv2d_shape

clean:
	rm -f $(LIB_OBJ) $(LIB)
	rm -rf $(BIN_DIR)
