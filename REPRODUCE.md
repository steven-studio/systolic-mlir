# Reproducibility Guide

**Updated 2026-09-22.**

This document answers two separate reproducibility questions:

1. **MLIR/compiler reproducibility** — can a clean checkout build `systolic-opt` and reproduce the IR transformations claimed by the project?
2. **Hardware/result traceability** — can every published hardware number be traced to a script, bitstream/configuration, and output?

Do not treat “the source file exists” as sufficient evidence. A reproducible compiler claim should have an input, an exact command, and an observable output. A reproducible hardware claim should additionally identify the hardware configuration and measurement output.

---

## 0. Current status

### Reproducible now

- Build and test the MLIR project with `./configure.sh --test`.
- `linalg.matmul -> systolic.stream + systolic.pe_array`.
- `systolic.pe_array -> scf.for + systolic.mac`.
- `linalg.matmul -> systolic.matmul_tile` tiling.
- Device selection and `est_cycles` annotation.
- Overlap scheduler annotation of `start_cycle`.
- A lowering pass exists from scheduled `systolic.matmul_tile` to LLVM-dialect external runtime calls.

### Integration boundaries that are **not yet closed**

- `SystolicTileToFpga.cpp` currently emits
  `systolic_dispatch_matmul4x4_dev(handle, device_id, ...)` and accepts only tile dimensions <= 4.
- The newer runtime contract in
  `runtime/lib/dispatch/systolic_dispatch_new.h` exposes
  `systolic_dispatch_matmul(handle, K, ...)` for an 8x8 array with `K <= 64`.
- Therefore the scheduling-to-runtime path and the newest 8x8 runtime ABI are presently at different interface generations.
- `systolic-schedule-overlap` computes `start_cycle`, but the current
  `SystolicTileToFpga.cpp` lowering does not consume `start_cycle`.
  Until a runtime/executable mechanism enforces it, `start_cycle` should be described as compiler schedule metadata/model output, not as hardware-enforced launch timing.

These are reproducibility findings, not things to hide: they define exactly which claims are executable today and which remain integration work.

---

## 1. Clean MLIR build

### 1.1 Environment

The build helper auto-detects LLVM/MLIR, Ninja, and `lit`. LLVM 18 can be forced explicitly:

```bash
LLVM_VERSION=18 ./configure.sh --fresh --test
```

Equivalent normal rebuild when the build directory is already valid:

```bash
./configure.sh --test
```

The script configures CMake with Ninja, builds the project, and runs the
`check-systolic` target.

### 1.2 Canonical compiler binary

For reproducibility commands in this document, use:

```bash
./build/bin/systolic-opt
```

`tools/systolic-opt/` is the source directory and
`build/tools/systolic-opt/` is a CMake/Ninja build directory. Do not use the existence of those directories as evidence of multiple compiler versions.

Confirm the binary and registered passes with:

```bash
file ./build/bin/systolic-opt
./build/bin/systolic-opt --help | grep systolic
```

Before recording results, record the source revision:

```bash
git rev-parse HEAD
git status --short
```

A paper result should preferably be associated with a clean commit.

---

## 2. MLIR transformation reproducibility

The commands below deliberately omit `FileCheck` first so the intermediate IR can be inspected directly. The corresponding tests then provide machine-checkable regression coverage.

### 2.1 Matmul -> Systolic PE array

Input:

`test/Systolic/matmul.mlir`

Run:

```bash
./build/bin/systolic-opt test/Systolic/matmul.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8"
```

Expected observable IR for the 8x8x8 case includes:

```mlir
systolic.stream
systolic.pe_array<8 x 8> stationary(weight)
```

The 16x16x16 case remains `linalg.matmul` in this pass because it does not exactly match the configured 8x8 array.

Machine-checkable test:

```bash
./build/bin/systolic-opt test/Systolic/matmul.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8" | \
  FileCheck test/Systolic/matmul.mlir
```

### 2.2 PE array -> explicit SCF loops + MAC

Input:

`test/Systolic/expand_to_mac.mlir`

Run:

```bash
./build/bin/systolic-opt test/Systolic/expand_to_mac.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8" \
  --expand-pe-array-to-mac
```

Expected observable IR:

- no `systolic.pe_array`
- no `systolic.stream`
- three `scf.for` levels
- `systolic.mac`

Machine-checkable test:

```bash
./build/bin/systolic-opt test/Systolic/expand_to_mac.mlir \
  --convert-matmul-to-systolic="rows=8 cols=8" \
  --expand-pe-array-to-mac | \
  FileCheck test/Systolic/expand_to_mac.mlir
```

This is the explicit lowering path:

```text
linalg.matmul
  -> systolic.stream + systolic.pe_array
  -> scf.for + systolic.mac
```

### 2.3 Matmul -> `systolic.matmul_tile`

Input:

`test/Systolic/tile_matmul.mlir`

Run:

```bash
./build/bin/systolic-opt \
  --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
  test/Systolic/tile_matmul.mlir
```

For a 16x16x16 GEMM tiled at 4x4x4, the test expects 64
`systolic.matmul_tile` operations in total, with K-dimension tiles chained through the accumulator input.

Machine-checkable test:

```bash
./build/bin/systolic-opt \
  --systolic-tile-matmul="tile-m=4 tile-n=4 tile-k=4" \
  test/Systolic/tile_matmul.mlir | \
  FileCheck test/Systolic/tile_matmul.mlir
```

### 2.4 Device selection

Input:

`test/Systolic/select_device.mlir`

Run:

```bash
./build/bin/systolic-opt --systolic-select-device \
  test/Systolic/select_device.mlir
```

Expected output contains `systolic.matmul_tile` operations annotated with
`on @acc_...` and `est_cycles`.

The current test fixture expects, among other checks:

- 64x64x64 -> `@acc_8x8`, `est_cycles = 14336`
- 32x32x32 -> `@acc_4x4`, `est_cycles = 8192`

Important: these test values use the device/test calibration encoded by that test. They must not be silently equated with a separately measured hardware `fixedOverhead` value without checking the device attributes and cost-model configuration used for the experiment.

### 2.5 Overlap scheduling

Input:

`test/Systolic/schedule_overlap.mlir`

Run:

```bash
./build/bin/systolic-opt \
  --systolic-tile-matmul="tile-m=8 tile-n=8 tile-k=8" \
  --systolic-select-device \
  --systolic-schedule-overlap \
  test/Systolic/schedule_overlap.mlir
```

The regression test expects `est_cycles = 28` and example
`start_cycle` values 16, 44, and 72.

What this proves:

- the compiler computes an overlap-aware schedule model;
- the schedule is materialized as `start_cycle` attributes.

What this does **not** yet prove:

- that the runtime waits until those exact cycles;
- that measured FPGA execution overlaps according to those `start_cycle` values.

### 2.6 Scheduled tile -> LLVM-dialect runtime call

Implementation:

`lib/Systolic/Transforms/SystolicTileToFpga.cpp`

Pass:

```text
--systolic-tile-to-fpga
```

The rewrite matches `systolic.matmul_tile`, requires a device assignment,
maps the device symbol to an integer `device_id`, materializes/pads operands,
and creates LLVM-dialect calls to:

```text
systolic_dispatch_open()
systolic_dispatch_matmul4x4_dev(handle, device_id, A, B, C_init, C_out)
```

In C++ this is created with `LLVM::CallOp`; in textual MLIR it prints as
`llvm.call`.

Current limitation: this executable lowering accepts only `m,n,k <= 4`.
It is therefore not currently the same ABI as the newer 8x8 runtime described in Section 3.

---

## 3. Runtime ABI reproducibility

### 3.1 New 8x8 runtime contract

`runtime/lib/dispatch/systolic_dispatch_new.h` defines:

```c
#define SYS_DISPATCH_R      8
#define SYS_DISPATCH_C      8
#define SYS_DISPATCH_K_MAX  64

int systolic_dispatch_open(void);

int systolic_dispatch_matmul(int handle, int K,
                             const float *A, const float *B,
                             const float *C_init, float *C_out);
```

The documented semantics are:

```text
C_out[i][j] = C_init[i][j] + sum_k A[i][k] * B[k][j]
```

The end-to-end input shape in `runtime/e2e/e2e_gemm_new.mlir` is
17x100x9, chosen to exercise M/N boundary tiles and a K tail
(64 + 36).

### 3.2 Known ABI mismatch to resolve

The compiler scheduling path currently lowers to the older
`systolic_dispatch_matmul4x4_dev` interface, whereas the new runtime header
exposes `systolic_dispatch_matmul` for 8x8xK.

Before claiming a single end-to-end scheduling pipeline, add a regression that starts from one MLIR input and proves:

```text
input linalg.matmul
 -> systolic.matmul_tile
 -> device assignment
 -> schedule metadata
 -> executable lowering
 -> runtime ABI used by the current 8x8 backend
 -> successful execution / bit-exact result
```

Until that exists, keep the compiler scheduling results and the newest 8x8 runtime results distinguishable in the paper and slides.

---

## 4. Hardware/result traceability

Platform used by the hardware results below: Nexys Video,
`xc7a200tsbg484-1`, 100 MHz, Vivado 2026.1.

### 4.1 Handwritten fold RTL

| Result | Command | Output |
|---|---|---|
| K_MAX=16/64/128 LUT / FF / DSP / WNS | `vivado -mode batch -source build_kmax.tcl -tclargs <K>` | `build_kmax/k<K>/summary.csv` |
| drain 111, feed K+7, errors 0 | `vivado -mode batch -source sim_kmax.tcl -tclargs <K>` | stdout `KMAXCSV,` line |
| silicon cycles 134 / 150 / 182 | `python3 test_uart_kmax.py --kmax 64 --k <16\|32\|64>` | stdout `hardware cycles` |
| bit-exact | same command | stdout `BIT-EXACT` |
| one PE: 1,318 LUT / 4 DSP | `/tmp/pe.tcl` (see `eval/scalesim/`) | `/tmp/util_pe.rpt` |
| 8x8+4x4: 78.1% LUT | `eval/scalesim/dual.tcl` | `/tmp/util_dual.rpt` |

Working directory for the fold RTL experiments:

```text
hls/multi_200t/fold_pipelined/rtl/
```

Vivado environment:

```bash
source ~/tools/Xilinx/2026.1/2026.1/Vivado/settings64.sh
```

Summary: `eval/scalesim/KMAX_SCALING.md`.

### 4.2 Hardware-calibrated cost model

The previous version of this document recorded:

- silicon k_dim=16 -> 134 cycles
- silicon k_dim=32 -> 150 cycles
- silicon k_dim=64 -> 182 cycles
- `fixedOverhead = 104`, obtained by subtracting the geometry term
  `k_dim + 14` from the measured points

These numbers must remain tied to the exact hardware configuration and cost-model revision that produced them. Do not mix them with older presentation constants or with small synthetic regression-test constants such as the `fixedOverhead = 6` example used by `select_device.mlir`.

### 4.3 SCALE-Sim geometry validation

See `eval/scalesim/`.

Configuration:

```text
nexys_8x8_os.cfg
```

Experiment split/protocol:

- `VALIDATION_PLAN.md`
- `VALIDATION_PLAN_AMENDMENT_01.md`

Keep SCALE-Sim geometry validation conceptually separate from RTL/FPGA-specific calibrated overhead.

---

## 5. Unresolved hardware provenance

### 5.1 conv2d 48/48 bit-exact: exact bitstream not identified

This remains a publication-relevant provenance gap.

Known:

- `runtime/harness/sweep_out/sweep_results_nexys_8x8.csv` records 48 configurations as bit-exact.
- Historical commit `e3b8817` reports 48/48 bit-exact on Nexys Video 2x(8x8).
- The historical runtime call chain and protocol comments do not yet identify the exact bitstream unambiguously.

Until resolved, do **not** claim that the 48/48 conv2d result was verified on the current fold bitstream. The safe statement is that it was obtained on a historical hardware configuration whose exact bitstream provenance still needs to be pinned down.

Useful provenance commands:

```bash
git log --format='%h %ad %s' --date=short -- \
  runtime/harness/sweep_out/sweep_results_nexys_8x8.csv

git show e3b8817 --stat

grep -rn "MATRIX_ELEMENTS\|192\|ndev\|dev" \
  runtime/lib/fpga_matmul4x4_reliable.c | head
```

### 5.2 conv2d tile counts not hardware-validated

The existing sweep CSVs contain predicted tile counts but no populated measured tile-count field. Do not describe conv2d tile-count prediction as hardware-validated until a measured comparison is added.

### 5.3 Duplicate sweep CSVs

Historically, `sweep_results_48_bitexact.csv` and
`sweep_results_nexys_8x8.csv` were byte-identical. Choose one canonical artifact before citing the result.

### 5.4 Historical `matmul_top_dual` report

The utilization report exists, but the corresponding build should be rerun before treating it as a freshly reproducible result.

---

## 6. Reproducibility checklist for a paper revision

For every compiler claim, record:

```text
Git commit
input .mlir
exact systolic-opt command
expected intermediate/output IR
regression test / FileCheck
```

For every end-to-end FPGA claim, additionally record:

```text
runtime ABI
runtime backend
bitstream / hardware build
board and clock
host command
raw output
expected result / bit-exact check
```

A useful minimum artifact for the MLIR part is one script that performs a clean build and emits named IR snapshots, for example:

```text
00-input.mlir
10-tiled.mlir
20-selected.mlir
30-scheduled.mlir
40-runtime-call.mlir
```

The final snapshot should only be called “end-to-end executable scheduling” once its runtime symbol matches the runtime actually linked for the current 8x8 hardware path.

---

## 7. Historical/attic policy

`runtime/attic/` already exists. Historical artifacts may be moved there, but do not delete them while provenance questions remain open.

Candidates previously identified:

| Path | Reason |
|---|---|
| `hls/multi_200t/_recovered/db___home_*.tcl` | recovered Vitis HLS internal database files, not source |
| other duplicates under `hls/multi_200t/_recovered/` | recovered historical copies |
| one duplicate sweep CSV | keep one canonical result after provenance is resolved |

Do not move runtime implementations merely because several protocol generations coexist. The current reproducibility task is to identify which compiler path targets which ABI, then either update the lowering or explicitly preserve the old path as historical.

---

## 8. Maintenance rule

A result is ready to cite only when another person can answer:

> **Which commit, which input, which command, and which output produced it?**

For hardware results add:

> **Which runtime ABI, which bitstream/configuration, and which board measurement produced it?**

If those questions cannot be answered, list the item as unresolved rather than silently treating it as reproduced.
