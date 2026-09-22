# Reproducing Systolic MLIR

This document provides a minimal, end-to-end procedure for reproducing
the MLIR/compiler results in this repository.

For the provenance of FPGA measurements, bitstreams, cycle counts, and
paper numbers, see `docs/PROVENANCE.md`.

## 1. Environment and clean build

Tested environment:

- LLVM/MLIR 18
- CMake + Ninja
- Release build

Record the exact revision:

    git rev-parse HEAD
    git status --short

Build and run all regression tests:

    LLVM_VERSION=18 ./configure.sh --fresh --test

Expected result:

    Total Discovered Tests: 15
      Passed: 15 (100.00%)

The canonical compiler binary used below is:

    ./build/bin/systolic-opt

Check that the Systolic passes are registered:

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
      FileCheck test/Systolic/matmul.mlir


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


## 4. Reproduce GEMM tiling

Run:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
      test/Systolic/tile_matmul.mlir

For the 16x16x16 test GEMM, this produces 64
`systolic.matmul_tile` operations.

K-dimension tiles are chained through the accumulator input.


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


## 7. Runtime-call lowering

The pass:

    --systolic-tile-to-fpga

lowers assigned `systolic.matmul_tile` operations toward external
runtime calls.

Implementation:

    lib/Systolic/Transforms/SystolicTileToFpga.cpp

The current lowering emits calls corresponding to:

    systolic_dispatch_open()
    systolic_dispatch_matmul4x4_dev(...)

In the C++ MLIR implementation these calls are represented by
`LLVM::CallOp`; textual LLVM-dialect MLIR prints them as `llvm.call`.


## 8. Current end-to-end limitation

The scheduled-tile lowering currently targets the older 4x4 dispatch
interface:

    systolic_dispatch_matmul4x4_dev(...)

while the newer 8x8 runtime interface defines:

    systolic_dispatch_matmul(handle, K, ...)

with:

    SYS_DISPATCH_R = 8
    SYS_DISPATCH_C = 8
    SYS_DISPATCH_K_MAX = 64

Therefore this repository does not yet claim that these two interface
generations form one fully integrated compiler-to-board pipeline.

The intended final reproducibility chain is:

    00-input.mlir
      -> 10-tiled.mlir
      -> 20-selected.mlir
      -> 30-scheduled.mlir
      -> 40-runtime-call.mlir
      -> runtime
      -> FPGA
      -> bit-exact result


## 9. Full regression suite

At any time, run:

    ./configure.sh --test

or:

    cmake --build build --target check-systolic

A successful reproduction should pass the complete Systolic regression
suite.

For FPGA measurement provenance and the exact sources of paper numbers,
see:

    docs/PROVENANCE.md
