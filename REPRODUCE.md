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

    Total Discovered Tests: 38
      Passed: 38 (100.00%)

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
the job descriptor the runtime writes (registers 0x18 / 0x1C). The
synchronous runtime submits one invocation at a time and waits for its
result, and the asynchronous one (section 8b) keeps up to PIPELINE_DEPTH in
flight per array; either way the value should not be read as
hardware-enforced launch timing (the hardware scheduler's clock is
session-global and only a lower bound).

Machine-checkable regression:

    ./build/bin/systolic-opt \
      --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
      --systolic-select-device \
      --systolic-schedule-overlap \
      test/Systolic/schedule_overlap.mlir | \
      /usr/lib/llvm-18/bin/FileCheck test/Systolic/schedule_overlap.mlir


## 6b. Reproduce the pipelined per-accelerator timeline (RTL vs cost model)

The 2026-10-18 RTL pipelines consecutive invocations of one accelerator
(docs/COSTMODEL.md section 8). The evidence, bench and model side by side:

    eval/pipeline_overlap/run.sh            # -> build/pipeline_overlap/summary.txt

runs rtl/multi_200t/tile_pipelined/dma/tb/tb_pipeline_overlap.sv on the
K-folded chains in eval/pipeline_overlap/jobs/ (one device at a time and
mixed; v1 and v2 operand path; offered back to back and one at a time), checks
every result tile exactly, and holds every job's fold / publish / write-back
cycle against `systolic-cost timeline` (eval/pipeline_overlap/compare_timeline.py).
One list by hand:

    rtl/multi_200t/tile_pipelined/dma/tb/run_pipeline_sim.sh 16 16 \
        eval/pipeline_overlap/jobs/dev0.svh /tmp/po v2
    python3 eval/pipeline_overlap/compare_timeline.py \
        --cost build/bin/systolic-cost --run /tmp/po/run.log --v2

Machine-checkable regression of the model alone:

    ./build/bin/systolic-cost timeline --geometry 8 --k-max 16 --k 16,16,16,16 \
        --pipelined --read-bytes-per-cycle 16

(test/Systolic/cost_model_pipelined.mlir, binary_search_pipelined.mlir).

## 7. Reproduce Linalg -> Systolic -> FPGA runtime calls

The pipeline that runs on the FPGA starts from `linalg.matmul` and a
`systolic.device` fleet. For the board's bitstream (one 8x8 and three 4x4
arrays, `k_max = 16`, `tile_overhead = 95`; a bitstream built with other
K_MAX_8X8 / K_MAX_4X4 declares those as the devices' `k_max` and passes
them to the lowering as `phys-k-max-8x8=` / `phys-k-max-4x4=`, see
`docs/KMAX.md`):

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
    test/Systolic/fpga_operand_staging_fallback.mlir
    test/Systolic/bad_fpga_lowering.mlir
    test/Systolic/kmax_heterogeneous.mlir
    test/Systolic/kmax_fpga_lowering.mlir
    test/Systolic/bad_kmax_device.mlir
    test/Systolic/bad_kmax_fleet.mlir
    test/Systolic/bad_kmax_lowering.mlir
    test/Systolic/cost_kmax.mlir
    test/Systolic/select_device_kmax.mlir

The `kmax_*`, `bad_kmax_*`, `cost_kmax` and `select_device_kmax` tests
cover independent K_MAX_8X8 / K_MAX_4X4 (folding K by each device's own
k_max, the cost of a tile on each geometry, the lowering's check of the
declared k_max against the bitstream's, and the refusals); see
`docs/KMAX.md`.

`bad_linalg_to_systolic.mlir` checks that a matmul with user-defined
`indexing_maps` (a transposed operand) or an empty M, N or K is refused
rather than lowered as the default product. `fpga_operand_staging.mlir`
checks how the lowering hands contiguous tiles to the runtime (scratch
tiles staged from the whole matrices' buffers), which keeps bufferization
linear in the number of invocations; `fpga_operand_staging_fallback.mlir`
checks the cases where it must not (C rooted in a `tensor.empty`,
loop-carried C, interleaved output blocks) and lowers tensors instead.


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
run to 16384 FPGA invocations (`docs/E2E.md`, Section 1). The regression:
36 fixed cases (the shapes 1x1x1, 2x3x4, 4x4x4, 8x8x8, 8x4x16, 16x8x4,
16x8x40, 17x9x33, 32x32x32, each with zero / positive / negative / mixed
integers) and the cases at the edges of the bitstream's two capacities
(K = 1, K_MAX - 1, K_MAX, K_MAX + 1 of K_MAX_8X8 and of K_MAX_4X4). The
harness has no default configuration: on the board, name the programmed
bitstream's fpga_config.txt, or the archived bitstream's profile:

    export SYSTOLIC_FPGA_CONFIG=<the .bit's directory>/fpga_config.txt
    test/e2e/run_e2e.sh --backend board --check-config   # host only, sends nothing
    test/e2e/run_e2e.sh --backend board     # the programmed Nexys Video
    test/e2e/run_e2e.sh --backend board --profile archived-1x8x8-3x4x4
    test/e2e/run_e2e.sh --backend cosim     # RTL co-simulation, no board
    cmake --build build --target check-e2e-config   # configuration handling, no FPGA

(`--quick`: the six required shapes and one case past each capacity;
`--fixed-only`: only the 36 fixed cases.) On the board the hardware
configuration is not independently verified: the FPGA does not report its
K_MAX.

The co-simulation (`test/e2e/cosim/build.sh`, Verilator) runs the RTL of the
working tree (`RTL_REV=tile-pipelined-1x8x8-3x4x4-20261009` for the archived
bitstream's), with only the Xilinx IP replaced, behind the unmodified runtime.
It is a simulation: a board run is the only evidence about the board. See
`docs/E2E.md`.

## 8b. The asynchronous runtime (`docs/ASYNC.md`)

A second runtime keeps the four arrays busy at once: up to PIPELINE_DEPTH
descriptors in flight per array (the context's job queue, 4 in this tree's
RTL; docs/COSTMODEL.md section 8), each with its own operand slab and
result tile in DDR, a further slab staged ahead, results attributed by the
job_id of a 16-byte completion record the bitstream sends ahead of every
tile. It needs a bitstream built from this tree's RTL (`dma_top_build.tcl`
writes `COMPLETION_HEADER=1` and `PIPELINE_DEPTH=4` into its
fpga_config.txt); the co-simulation built by `test/e2e/cosim/build.sh` from
the working tree has both (docs/ASYNC.md section 9 has the evidence runs).

    test/e2e/cosim/build.sh                               # COMPLETION_HEADER=1, PIPELINE_DEPTH=4
    cmake --build build --target check-systolic           # 79 lit tests, three for the async lowering
    test/e2e/run_e2e.sh --backend cosim                   # the synchronous runtime, as before
    test/e2e/run_e2e.sh --backend cosim --mode async      # systolic-matmul --async on every case
    SYSTOLIC_FPGA_CONFIG=build/cosim/fpga_config.txt build/bin/runtime-async-test --mode all

`runtime-async-test` runs the experiments of `docs/ASYNC.md` as modes
(A one array, B every array, C with the slab staged behind the running
job, D two GEMMs interleaved, X the fold order checked bit for bit against
the synchronous entry point, V the refusals); `--gemm MxNxK` sizes them. The
RTL changes have their own benches:

    rtl/multi_200t/tile_pipelined/dma/tb/run_top_sim.sh v1 16 1 1          # the six regression gates, as before
    COMPLETION_HEADER=1 rtl/multi_200t/tile_pipelined/dma/tb/run_dpti_direct.sh all
    (cd rtl/multi_200t/tile_pipelined/dma/tb && iverilog -g2012 -I ../../../../../test/e2e/cosim \
        -o /tmp/tb_arb tb_dpti_bus_arbiter.sv ../dpti_bus_arbiter.sv && vvp /tmp/tb_arb)

The co-simulation shows correctness and that the overlap happens (the
runtime's counters, the RTL's VIO counters); it runs at Verilator's speed, so
the speed-up itself is a board measurement (`docs/ASYNC.md`, Section 6).


## 8a. Cost model, binary search, K_MAX exploration

The read / compute / write cost of one invocation (`docs/COSTMODEL.md`) and
the binary-search schedule, from the command line, with exactly the
compiler's code:

    ./build/bin/systolic-cost invocation --geometry 8 --k-max 32 --k 65
    ./build/bin/systolic-cost schedule --m 64 --n 64 --k 64 --shared-read-words-per-cycle 3.51 --tiles
    ./build/bin/systolic-cost check-monotone --m 100 --n 36 --k 147 --k8 24 --k4 12

Invocations are variable-K: the runtime sends each invocation's own depth
(job.k = k_i <= K_MAX). Model against the RTL in Verilator, with the
reduction steps every PE executes counted by the bench (calibration set
K_MAX 24/12; held-out sets K_MAX 32/32, 16/7, 17/24, 1/1, 256/64, 512/512 at
K = 1, K_MAX - 1, K_MAX, K_MAX + 1 and K-folded K; concurrent waves; about 15
minutes on two cores):

    python3 eval/cost_model_validation/validate.py --jobs 2
    python3 eval/cost_model_validation/start_cycle_check.py

The whole pipeline on the co-simulated RTL of the working tree, with the
runtime's trace and the bench's step counts compared invocation by
invocation (compiler IR k = runtime job.k = RTL k = steps per PE):

    SYSTOLIC_RUNTIME_TRACE=1 test/e2e/run_kmax_matrix.sh 16:16 32:32 17:7 24:12 1:1
    SYSTOLIC_RUNTIME_TRACE=1 SKIP_RTL=1 test/e2e/run_kmax_matrix.sh 256:64 512:512
    python3 test/e2e/check_variable_k.py build/kmax_matrix/e2e/*/* --cost build/bin/systolic-cost

(`run_kmax_matrix.sh`'s RTL step, `run_kmax_sim.sh`, takes K_MAX up to 56, so
the deeper configurations skip it -- `validate.py` covers them in Verilator;
each call rewrites `build/kmax_matrix/summary.txt`.) The K-folding cases of
K_MAX 32/32 through the same pipeline:

    SYSTOLIC_RUNTIME_TRACE=1 SYSTOLIC_FPGA_CONFIG=build/kmax_matrix/cosim/k8_32-k4_32/fpga_config.txt \
        test/e2e/run_e2e.sh --backend cosim --cosim-dir build/kmax_matrix/cosim/k8_32-k4_32 \
        --out build/kmax_matrix/e2e/k8_32-k4_32/kfold --kmax-cases --extra-k 7,16,47,64,65

The ResNet-18 / AlexNet GEMM shapes (checked against torchvision v0.19.0
source when given) and the K_MAX design-space exploration (`docs/KMAX_DSE.md`):

    python3 eval/kmax_dse/layers.py --torchvision-src DIR
    python3 eval/kmax_dse/dse.py --verify --jobs 2


## 9. Full regression suite

At any time after configuration, run:

    LLVM_VERSION=18 ./configure.sh --test

or, using the existing build directory:

    cmake --build build --target check-systolic

A successful reproduction should end with:

    Total Discovered Tests: 79
      Passed: 79 (100.00%)

For FPGA measurement provenance and the exact sources of paper numbers,
see:

    docs/PROVENANCE.md
