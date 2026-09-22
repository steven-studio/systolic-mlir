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

    Total Discovered Tests: 16
      Passed: 16 (100.00%)

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

It should not currently be interpreted as hardware-enforced launch
timing because the runtime lowering does not yet consume this
attribute.

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
      --systolic-select-device \
      --systolic-schedule-overlap \
      test/Systolic/schedule_overlap.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/schedule_overlap.mlir


## 7. Reproduce runtime-call lowering

Input:

    test/Systolic/tile_to_fpga.mlir

Run:

    ./build/bin/systolic-opt \
      --systolic-tile-to-fpga \
      test/Systolic/tile_to_fpga.mlir

The input contains one assigned 4x4 `systolic.matmul_tile`. The
lowering emits declarations and calls for the current dispatch-runtime
ABI:

    llvm.func @systolic_dispatch_open
    llvm.func @systolic_dispatch_matmul4x4_dev
    llvm.call @systolic_dispatch_open
    llvm.call @systolic_dispatch_matmul4x4_dev

After successful lowering, no `systolic.matmul_tile` remains.

The implementation of this pass is:

    lib/Systolic/Transforms/SystolicTileToFpga.cpp

In the C++ MLIR implementation the calls are represented by
`LLVM::CallOp`; textual LLVM-dialect MLIR prints them as `llvm.call`.

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-tile-to-fpga \
      test/Systolic/tile_to_fpga.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/tile_to_fpga.mlir


## 8. Current end-to-end limitation

This repository currently contains two distinct FPGA-oriented lowering
paths.

The scheduled-tile path reproduced in Section 7 lowers:

    systolic.matmul_tile
      -> systolic_dispatch_matmul4x4_dev(...)

through:

    --systolic-tile-to-fpga

This path is regression-tested by:

    test/Systolic/tile_to_fpga.mlir

A separate pass:

    --tile-matmul-for-fpga

lowers `linalg.matmul` toward the newer runtime entry point:

    systolic_dispatch_matmul(handle, K, ...)

whose current runtime interface defines:

    SYS_DISPATCH_R = 8
    SYS_DISPATCH_C = 8
    SYS_DISPATCH_K_MAX = 64

The two lowering paths should not be treated as one integrated
scheduling-to-board pipeline. In particular, the
`--systolic-tile-to-fpga` implementation does not consume the
`start_cycle` metadata produced by `--systolic-schedule-overlap`, and
the separate `--tile-matmul-for-fpga` path is not currently covered by
the Systolic regression fixtures in `test/Systolic`.

Therefore the compiler-side stages reproduced in this document are
verified through runtime-call lowering, but this document does not
claim a single regression-tested chain from overlap scheduling through
the newer 8x8 runtime interface to FPGA execution and a bit-exact
hardware result.

The intended fully integrated chain is:

    00-input.mlir
      -> 10-tiled.mlir
      -> 20-selected.mlir
      -> 30-scheduled.mlir
      -> 40-runtime-call.mlir
      -> runtime
      -> FPGA
      -> bit-exact result


## 9. Full regression suite

At any time after configuration, run:

    LLVM_VERSION=18 ./configure.sh --test

or, using the existing build directory:

    cmake --build build --target check-systolic

A successful reproduction should end with:

    Total Discovered Tests: 16
      Passed: 16 (100.00%)

For FPGA measurement provenance and the exact sources of paper numbers,
see:

    docs/PROVENANCE.md
