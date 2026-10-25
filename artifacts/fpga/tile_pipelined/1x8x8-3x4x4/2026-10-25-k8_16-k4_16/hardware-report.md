<!-- Archive-relative evidence paths; historical executable commands are not a promise that binaries are included. Original report is in hardware-validation/hardware-report-original.md. -->
# Shared Writeback R2 physical FPGA verification

Hardware: v1 K8=16 K4=16, 1x8x8 + 3x4x4; shared_wb_r2_etopt.
Config: fpga_config.txt
TIMING_MET=1 DPTI_IO_GATE=PASS. Actual bitstream SHA256 verified: bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5.

Every hardware command used a sanitized environment: unset all inherited SYSTOLIC_* variables and LD_PRELOAD, then set SYSTOLIC_FPGA_CONFIG to the path above. Real libftdi/libusb, backend board; no cosimulation.

Runtime test binaries compiled from current checkout with g++ -std=c++17 -O2 -pthread -Iruntime TEST_SOURCE runtime/dpti_runtime.c runtime/dpti_ftdi.cpp runtime/fpga_matmul_tiled.cpp runtime/fpga_async_runtime.cpp -I/usr/include/libftdi1 -lftdi1 -lusb-1.0 -o OUTPUT.

Compiler-driven cases use the existing sibling systolic-matmul binary via a new symlink in this artifact directory. Sibling runtime sources match current checkout byte-for-byte; compiler source differs in scheduling cost-model selection (default v2), so these do not qualify an exact current-checkout compiler build.

## Commands and logs

### 01
```sh
hardware-validation/bin/runtime-async-test --backend board --mode A --devices 0 --gemm 1x1x1
```
Log: hardware-validation/01-smoke.log

### 02-full8
```sh
hardware-validation/bin/runtime-async-test --backend board --mode A --devices 0 --gemm 8x8x16
```
Log: hardware-validation/02-full8.log

### 03-full4-dev1
```sh
hardware-validation/bin/runtime-async-test --backend board --mode A --devices 1 --gemm 4x4x16
```
Log: hardware-validation/03-full4-dev1.log

### 03-full4-dev2
```sh
hardware-validation/bin/runtime-async-test --backend board --mode A --devices 2 --gemm 4x4x16
```
Log: hardware-validation/03-full4-dev2.log

### 03-full4-dev3
```sh
hardware-validation/bin/runtime-async-test --backend board --mode A --devices 3 --gemm 4x4x16
```
Log: hardware-validation/03-full4-dev3.log

### 04-small-fleet
```sh
hardware-validation/bin/runtime-async-test --backend board --mode B,C --devices 1,2,3 --gemm 16x16x32
```
Log: hardware-validation/04-small-fleet.log

### 05-mixed
```sh
hardware-validation/bin/runtime-async-test --backend board --mode B,C,S --gemm 16x16x32
```
Log: hardware-validation/05-mixed.log

### 06-consecutive
```sh
hardware-validation/bin/runtime-async-test --backend board --mode D,Q,M --gemm 16x16x32
```
Log: hardware-validation/06-consecutive.log

### 07-large
```sh
hardware-validation/bin/runtime-async-test --backend board --mode B,C --gemm 64x64x64
```
Log: hardware-validation/07-large.log

### 08-dispatch
```sh
hardware-validation/bin/runtime-dispatch-test
```
Log: hardware-validation/08-dispatch.log

### 09-sequence-wrap
```sh
hardware-validation/bin/runtime-async-test --backend board --mode C --gemm 128x128x64
```
Log: hardware-validation/09-sequence-wrap.log

### 10-sync-quick
```sh
bash test/e2e/run_e2e.sh --backend board --build-dir /tmp/shared-wb-r2-hw-20261025-run1 --mode sync --quick --out hardware-validation/sync-quick
```
Log: hardware-validation/10-sync-quick.log

### 11-async-regression
```sh
bash test/e2e/run_e2e.sh --backend board --build-dir /tmp/shared-wb-r2-hw-20261025-run1 --mode async --regression --out hardware-validation/async-regression
```
Log: hardware-validation/11-async-regression.log

## Numerical scoreboard audit

sync-quick: 26 cases, 1068 CPU-compared output elements, 0 mismatches.

| Case | Mismatches | Checked elements |
|---|---:|---:|
| 16x8x4_mixed | 0 | 128 |
| 16x8x4_negative | 0 | 128 |
| 16x8x4_positive | 0 | 128 |
| 16x8x4_zero | 0 | 128 |
| 1x1x1_mixed | 0 | 1 |
| 1x1x1_negative | 0 | 1 |
| 1x1x1_positive | 0 | 1 |
| 1x1x1_zero | 0 | 1 |
| 2x3x4_mixed | 0 | 6 |
| 2x3x4_negative | 0 | 6 |
| 2x3x4_positive | 0 | 6 |
| 2x3x4_zero | 0 | 6 |
| 4x4x17_mixed | 0 | 16 |
| 4x4x4_mixed | 0 | 16 |
| 4x4x4_negative | 0 | 16 |
| 4x4x4_positive | 0 | 16 |
| 4x4x4_zero | 0 | 16 |
| 8x4x16_mixed | 0 | 32 |
| 8x4x16_negative | 0 | 32 |
| 8x4x16_positive | 0 | 32 |
| 8x4x16_zero | 0 | 32 |
| 8x8x17_mixed | 0 | 64 |
| 8x8x8_mixed | 0 | 64 |
| 8x8x8_negative | 0 | 64 |
| 8x8x8_positive | 0 | 64 |
| 8x8x8_zero | 0 | 64 |

async-regression: 52 cases, 15875 CPU-compared output elements, 0 mismatches.

| Case | Mismatches | Checked elements |
|---|---:|---:|
| 12x12x15_mixed | 0 | 144 |
| 12x12x16_mixed | 0 | 144 |
| 12x12x17_mixed | 0 | 144 |
| 12x12x1_mixed | 0 | 144 |
| 12x12x35_mixed | 0 | 144 |
| 15x15x16_mixed | 0 | 225 |
| 16x16x31_mixed | 0 | 256 |
| 16x8x40_mixed | 0 | 128 |
| 16x8x40_negative | 0 | 128 |
| 16x8x40_positive | 0 | 128 |
| 16x8x40_zero | 0 | 128 |
| 16x8x4_mixed | 0 | 128 |
| 16x8x4_negative | 0 | 128 |
| 16x8x4_positive | 0 | 128 |
| 16x8x4_zero | 0 | 128 |
| 17x9x33_mixed | 0 | 153 |
| 17x9x33_negative | 0 | 153 |
| 17x9x33_positive | 0 | 153 |
| 17x9x33_zero | 0 | 153 |
| 1x1x1_mixed | 0 | 1 |
| 1x1x1_negative | 0 | 1 |
| 1x1x1_positive | 0 | 1 |
| 1x1x1_zero | 0 | 1 |
| 2x3x4_mixed | 0 | 6 |
| 2x3x4_negative | 0 | 6 |
| 2x3x4_positive | 0 | 6 |
| 2x3x4_zero | 0 | 6 |
| 32x32x32_mixed | 0 | 1024 |
| 32x32x32_negative | 0 | 1024 |
| 32x32x32_positive | 0 | 1024 |
| 32x32x32_zero | 0 | 1024 |
| 32x32x65_mixed | 0 | 1024 |
| 32x64x32_mixed | 0 | 2048 |
| 33x33x16_mixed | 0 | 1089 |
| 4x4x16_mixed | 0 | 16 |
| 4x4x17_mixed | 0 | 16 |
| 4x4x4_mixed | 0 | 16 |
| 4x4x4_negative | 0 | 16 |
| 4x4x4_positive | 0 | 16 |
| 4x4x4_zero | 0 | 16 |
| 64x64x64_mixed | 0 | 4096 |
| 7x7x16_mixed | 0 | 49 |
| 8x4x16_mixed | 0 | 32 |
| 8x4x16_negative | 0 | 32 |
| 8x4x16_positive | 0 | 32 |
| 8x4x16_zero | 0 | 32 |
| 8x8x16_mixed | 0 | 64 |
| 8x8x17_mixed | 0 | 64 |
| 8x8x8_mixed | 0 | 64 |
| 8x8x8_negative | 0 | 64 |
| 8x8x8_positive | 0 | 64 |
| 8x8x8_zero | 0 | 64 |

## Limits
Mode X is supplemental asynchronous-versus-synchronous FPGA bitwise consistency, not an independent CPU numerical reference. All integer GEMM tests independently compute an int64 CPU reference and compare every output element exactly.
Mode F NOT APPLICABLE: simulation-only transport-failure injection. No unexpected failing tests. Mode P expected 1 ms timeout and invalid API operations; V/H expected rejections and successful recovery.
Host overlap2/overlap4 counters are transfer-worker passes, not hardware compute-cycle evidence. No Vivado/VIO run: prohibited by task; existing read_vio command also calls arm_run. Four outstanding jobs per context does not establish multiple pending DDR writes.
No direct hardware proof of simultaneous compute, early result-bank reuse before B completion, internal FIFO/metadata full conditions, AXI backpressure/error handling or round-robin fairness. These retain prior RTL-only verification where covered. No deadlock occurred in finite executed workloads; universal deadlock freedom is not proven.
No source/sibling changes, no FPGA programming, no Vivado, no synthesis or implementation. All artifacts created in a new /tmp directory.
