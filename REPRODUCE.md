# Reproducing Systolic MLIR

This document provides a minimal, end-to-end procedure for reproducing
the MLIR/compiler results in this repository.

For the provenance of FPGA measurements, bitstreams, cycle counts, and
paper numbers, see `docs/PROVENANCE.md`.

## 1. Clone, environment, and clean build

Clone the repository from GitHub:

    git clone https://github.com/steven-studio/systolic-mlir.git
    cd systolic-mlir

Record the exact revision:

    git rev-parse HEAD
    git status --short
    git branch --show-current

The following environment was used for the verified reproduction:

    uname -a
    cmake --version
    ninja --version
    gcc --version
    g++ --version
    /usr/lib/llvm-18/bin/llvm-config --version
    /usr/lib/llvm-18/bin/FileCheck --version
    lit --version

The verified setup uses LLVM/MLIR 18 installed under:

    /usr/lib/llvm-18

Before configuring, verify that the LLVM and MLIR CMake packages exist:

    test -d /usr/lib/llvm-18/lib/cmake/llvm && echo "LLVM CMake: OK"
    test -d /usr/lib/llvm-18/lib/cmake/mlir && echo "MLIR CMake: OK"

Configure from a fresh build directory, build the project, and run all
Systolic regression tests:

    LLVM_VERSION=18 ./configure.sh --fresh --test

On the verified system, `configure.sh` selects `/usr/bin/gcc` and
`/usr/bin/g++` when they are available. Clang 18 may also be installed,
but it is not the compiler selected by this configuration.

A successful run should end with:

    Total Discovered Tests: 23
      Passed: 23 (100.00%)

The canonical compiler binary used below is:

    ./build/bin/systolic-opt

Verify the binary and registered Systolic passes:

    ./build/bin/systolic-opt --version
    ./build/bin/systolic-opt --help | grep systolic


## 2. Reproduce `linalg.matmul -> systolic.pe_array`

Input:

    test/Systolic/matmul.mlir

Run:

    ./build/bin/systolic-opt test/Systolic/matmul.mlir \
      --convert-matmul-to-systolic="rows=8 cols=8"

For the 8x8x8 case, the output should contain:

    systolic.stream
    systolic.pe_array<8 x 8> stationary(weight)

The 16x16x16 case remains `linalg.matmul` because its shape does not
match the configured 8x8 array geometry.

Machine-checkable regression:

    ./build/bin/systolic-opt test/Systolic/matmul.mlir \
      --convert-matmul-to-systolic="rows=8 cols=8" | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/matmul.mlir


## 3. Reproduce `systolic.pe_array -> scf.for + systolic.mac`

Run:

    ./build/bin/systolic-opt test/Systolic/expand_to_mac.mlir \
      --convert-matmul-to-systolic="rows=8 cols=8" \
      --expand-pe-array-to-mac

Expected transformation:

    linalg.matmul
      -> systolic.stream + systolic.pe_array
      -> scf.for + systolic.mac

The final IR should no longer contain `systolic.pe_array` or
`systolic.stream`, and should contain a three-level `scf.for` loop nest
and `systolic.mac`.

Machine-checkable regression:

    ./build/bin/systolic-opt test/Systolic/expand_to_mac.mlir \
      --convert-matmul-to-systolic="rows=8 cols=8" \
      --expand-pe-array-to-mac | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/expand_to_mac.mlir


## 4. Reproduce GEMM tiling

Run:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
      test/Systolic/tile_matmul.mlir

For the 16x16x16 test GEMM, this produces 64
`systolic.matmul_tile` operations.

The emitted IR also shows K-dimension tiles chained through the
accumulator input. The current regression checks the tiling structure
and tile count, but does not separately FileCheck the accumulator SSA
chain.

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
      test/Systolic/tile_matmul.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/tile_matmul.mlir


## 5. Reproduce device selection

Run:

    ./build/bin/systolic-opt \
      --systolic-select-device \
      test/Systolic/select_device.mlir

The resulting `systolic.matmul_tile` operations should contain:

    on @acc_...
    est_cycles = ...

The constants in this regression fixture are test parameters. They
must not be interpreted as the FPGA-calibrated `H = 95`.

Hardware-calibrated measurements are documented separately in
`docs/PROVENANCE.md`.

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-select-device \
      test/Systolic/select_device.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/select_device.mlir


## 6. Reproduce overlap scheduling

Run:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
      --systolic-select-device \
      --systolic-schedule-overlap \
      test/Systolic/schedule_overlap.mlir

The regression fixture checks:

    est_cycles = 28
    start_cycle = 16
    start_cycle = 44
    start_cycle = 72

`start_cycle` is compiler-generated scheduling metadata.
`--lower-systolic-matmul-tile-to-fpga` passes it, with `est_cycles`, into
the job descriptor the runtime writes (registers 0x18 / 0x1C). The runtime
submits one invocation at a time and waits for its result, so the value
should still not be read as hardware-enforced launch timing.

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
      --systolic-select-device \
      --systolic-schedule-overlap \
      test/Systolic/schedule_overlap.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/schedule_overlap.mlir


## 7. Reproduce Linalg -> Systolic -> FPGA runtime calls

The pipeline that runs on the FPGA starts from `linalg.matmul` and a
`systolic.device` fleet. For the board's bitstream (one 8x8 and three 4x4
arrays, `k_max = 16`, `tile_overhead = 95`):

    ./build/bin/systolic-opt --systolic-binary-search-tile-matmul \
      test/Systolic/linalg_to_systolic_shapes.mlir

decomposes every GEMM into `systolic.matmul_tile` ops, each one invocation
of one device (`on @acc_8x8_0` / `@acc_4x4_0..2`) with its schedule
(`est_cycles`, `start_cycle`). M and N need not be equal; K is folded by the
device's `k_max` into invocations chained through `c_in`.

    ./build/bin/systolic-opt --systolic-binary-search-tile-matmul \
      --lower-systolic-matmul-tile-to-fpga test/Systolic/systolic_to_fpga.mlir

turns each tile into one call of the runtime entry point
`fpga_matmul_tiled_auto_scheduled(m, k, n, A, B, C, device_id,
start_cycle, est_cycles)` (`runtime/fpga_matmul_tiled.cpp`), with the
device symbol mapped onto the bitstream's physical IDs (`@acc_8x8_0` -> 0,
`@acc_4x4_0..2` -> 1..3). `--systolic-fpga-pipeline` runs both and then
bufferizes and lowers to the LLVM dialect.

Machine-checkable regressions (all part of `check-systolic`):

    test/Systolic/linalg_to_systolic_4x4x4.mlir
    test/Systolic/linalg_to_systolic_shapes.mlir
    test/Systolic/bad_linalg_to_systolic.mlir
    test/Systolic/systolic_to_fpga.mlir
    test/Systolic/fpga_operand_staging.mlir
    test/Systolic/bad_fpga_lowering.mlir

`bad_linalg_to_systolic.mlir` checks that a matmul with user-defined
`indexing_maps` (a transposed operand) or an empty M, N or K is refused
rather than lowered as the default product. `fpga_operand_staging.mlir`
checks how the lowering hands contiguous tiles to the runtime (scratch
tiles staged from the whole matrices' buffers), which keeps bufferization
linear in the number of invocations.


## 8. End to end: `./build/bin/systolic-matmul`

`systolic-matmul` reads `M N K`, `A[M][K]` and `B[K][N]` from stdin, builds
the `linalg.matmul` module for that shape, runs the pipeline of Section 7,
JIT-compiles the result, executes it -- the runtime stages each invocation
over DPTI into DDR, submits its descriptor and reads the result tile back --
and checks C element by element against an exact integer CPU reference:

    printf '2 3 4\n1 2 3 4\n-1 0 2 -3\n1 0 -1\n2 1 0\n0 -2 3\n-1 1 1\n' \
      | ./build/bin/systolic-matmul --dump-ir=/tmp/e2e

`--dump-ir` keeps `00-linalg.mlir`, `10-systolic.mlir`, `20-fpga.mlir`,
`30-llvm.mlir` and `40-llvm.ll`. Matrix elements must be integers within
+-2^24 (exact in binary32); M and N are limited to 512, K to 4096, and a
run to 16384 FPGA invocations (`docs/E2E.md`, Section 1). The regression over 36 cases (the shapes
1x1x1, 2x3x4, 4x4x4, 8x8x8, 8x4x16, 16x8x4, 16x8x40, 17x9x33, 32x32x32,
each with zero / positive / negative / mixed integers):

    test/e2e/run_e2e.sh --backend board     # the programmed Nexys Video
    test/e2e/run_e2e.sh --backend cosim     # RTL co-simulation, no board

The co-simulation (`test/e2e/cosim/build.sh`, Verilator) runs the RTL of the
bitstream's tag `tile-pipelined-1x8x8-3x4x4-20261009`, with only the Xilinx
IP replaced, behind the unmodified runtime. It is a simulation: a board run
is the only evidence about the board. See `docs/E2E.md`.


## 9. Full regression suite

At any time after configuration, run:

    LLVM_VERSION=18 ./configure.sh --test

or, using the existing build directory:

    cmake --build build --target check-systolic

A successful reproduction should end with:

    Total Discovered Tests: 23
      Passed: 23 (100.00%)

For FPGA measurement provenance and the exact sources of paper numbers,
see:

    docs/PROVENANCE.md
