# Reproducing Systolic-MLIR: C GEMM → systolic-cc → FPGA (board or RTL co-simulation)

This document reproduces the whole flow, from a C program to a GEMM executed
on the systolic-array FPGA and checked against a CPU reference, in two modes:

- **Board Mode**: a Digilent Nexys Video programmed with **one** archived
  bitstream (section 6), reached over DPTI (FT2232H, USB).
- **Co-simulation Mode**: no FPGA. The same host binaries talk DPTI to a
  Verilator simulation of the same RTL.

Every command below was run on 2026-10-21 against `main` (results:
section 15). The scripts in `scripts/reproducibility/` set the environment
themselves and work from an empty environment (`env -i`). Your own shell
variables are not used, and your shell is not modified.

`docs/PROVENANCE.md` covers the paper numbers. The compiler's passes are
exercised pass by pass by the lit suite (`lit build/test`, section 15), and
the cost models are described in `docs/COSTMODEL_V2.md` (the default) and
`docs/COSTMODEL.md` (V1).

---

## 1. Overview

```text
User C program (test/reproducibility/gemm_example.c)
  │  systolic-cc --runtime=board --fp-reassociation
  ▼
systolic-cfront (clang 24 AST → MLIR, per function)
  ▼
Affine GEMM recognition + exact partitioning   (--affine-matmul-to-systolic)
  ▼
Systolic lowering: systolic.matmul_tile per invocation, scheduled on the
1x8x8 + 3x4x4 fleet, K folded by K_MAX = 16     (--lower-systolic-matmul-tile-to-fpga)
  ▼
LLVM → object; a generated C shim calls the kernel; linked with
libsystolic_fpga_runtime.a + libftdi1
  ▼
Runtime job submission (async: fpga_tile_submit per tile, fpga_gemm_wait per GEMM)
  ▼
DPTI byte stream (MEM_WRITE operand slab, WRITE32 descriptor, completion record + tile)
  ├── Board Mode:  system libftdi1 → FT2232H → systolic_dma_top on the Artix-7
  └── Co-sim Mode: libftdi_cosim.so (LD_PRELOAD) → UNIX socket → Vcosim
                   (Verilator: the same systolic_dma_top RTL; Xilinx FP IP,
                    MIG/DDR3 and primitives replaced by bench models)
  ▼
Result readback → C → program's CPU reference → PASS / FAIL
```

| Path | What it is |
|---|---|
| `scripts/reproducibility/env.common.sh` | Shared compiler/runtime environment (sourced) |
| `scripts/reproducibility/env.board.sh` | Board Mode environment (sourced) |
| `scripts/reproducibility/env.cosim.sh` | Co-simulation Mode environment (sourced) |
| `scripts/reproducibility/check_env.sh` | Checks each requirement: `board`, `cosim` or `all` |
| `scripts/reproducibility/build.sh` | Configures and builds systolic-cc and the runtime; `--cosim` also builds Vcosim |
| `scripts/reproducibility/run_board.sh` | `check`, `verify-program`, `program`, `e2e`, `c FILE.c` |
| `scripts/reproducibility/program_bit.tcl` | Vivado hardware-manager script: program the xc7a200t with one given `.bit` (used by `run_board.sh program`) |
| `scripts/reproducibility/run_cosim.sh` | `check`, `build`, `e2e`, `c FILE.c` |
| `scripts/reproducibility/c_program.sh` | Shared helper: compile one C program, run it, report |
| `test/reproducibility/gemm_example.c` | Minimal user program: GEMM, CPU reference, PASS/FAIL |

## 2. Hardware / Software Requirements

`check_env.sh` checks every item below.

**Hardware (Board Mode only)**

- Digilent Nexys Video (`xc7a200tsbg484-1`). Its DPTI port is an FT2232H,
  USB ID `0403:6010`, which must be visible to the host.
- Read/write access to that USB device for your user. The tested host uses
  udev rules that set mode 0666 on `0403` devices from Digilent. Without such
  a rule, libftdi cannot open the device unless you run as root.
- The JTAG cable drivers that come with Vivado, needed only to program the
  board.

**Software**

| Item | Needed for | Tested version |
|---|---|---|
| Linux x86_64, bash | everything | Ubuntu, bash 5 |
| cmake, ninja, gcc/g++ | build | GCC 13.3.0, Ninja from `/usr/bin` |
| pkg-config, libftdi1 development files (`libftdi1-dev`) | runtime link | libftdi1 1.5 |
| python3 (standard library only) | `run_e2e.sh` helpers | 3.12 (`/usr/bin/python3`) |
| git, sha256sum, sed, awk | scripts | system |
| LLVM/MLIR 24 **build tree** (`lib/cmake/{llvm,mlir}`, `bin/mlir-translate`, `bin/llc`, `lib/libmlir_c_runner_utils.so`) | build + systolic-cc | 24.0.0git, rev 7615da10b75c |
| clang 24 (`-DSYSTOLIC_CFRONT_CLANG`) | systolic-cc's C front end and host compile | 24.0.0git, rev 7615da10b75c |
| Verilator ≥ 5.0 | Co-sim Mode | 5.020 |
| Vivado + hw_server | only to program the board | 2026.1 (the bitstream was built with it) |

LLVM 18, for example `/usr/lib/llvm-18`, does **not** build this tree.
Icarus Verilog is not used by either flow.

## 3. Clean Environment Setup

The scripts are written to work in an environment that has none of the
developer's variables (`SYSTOLIC_FPGA_CONFIG`, `LD_LIBRARY_PATH`,
`LD_PRELOAD`, custom compiler directories on `PATH`, and so on). To simulate
a new user:

```bash
cd ~/work/systolic-mlir

env -i \
  HOME="$HOME" \
  USER="$(id -un)" \
  PATH="/usr/bin:/bin" \
  bash --noprofile --norc
```

Inside that shell, from the repository root:

```bash
bash scripts/reproducibility/check_env.sh all
```

The scripts need only three inherited variables:

- `PATH=/usr/bin:/bin`, to find bash itself. The scripts then rebuild `PATH`.
- `HOME`, not used by the scripts themselves; Vivado (section 7) keeps its
  per-user settings under it.
- `USER`, used only by the reports.

`env -i` is **not** a fresh operating system. It only removes inherited
variables. The installed packages listed in section 2 (LLVM 24, clang 24,
libftdi1, Verilator, Vivado, USB rules) still come from the machine.
`check_env.sh` therefore reports two kinds of problems separately:

- `MISSING`: a package or device that is not there. Install it. Exporting a
  variable will not fix it.
- `NOT BUILT`: something `build.sh` has not produced yet.

## 4. Environment Isolation

The `run_*.sh` scripts **source** their environment file in their own process.
Nothing is written to `~/.bashrc` or `~/.profile`, and the calling terminal is
unchanged after a run. Board Mode and Co-sim Mode use separate files, so a
co-simulation setting cannot reach a board run and the reverse.

`env.common.sh` is shared by both modes:

| Variable | Value | Read by |
|---|---|---|
| `SYSTOLIC_BUILD` | `<repo>/build` unless set on the command line | systolic-cc, the scripts |
| `LLVM_BUILD` | `LLVM_DIR` from `SYSTOLIC_BUILD/CMakeCache.txt` without `/lib/cmake/llvm` | systolic-cc (mlir-translate, llc, libmlir_c_runner_utils) |
| `SYSTOLIC_CFRONT_CLANG` | `SYSTOLIC_CFRONT_CLANG` from `CMakeCache.txt` | systolic-cc, `test/e2e/systolic_cc/run_board.sh` |
| `PATH` | `SYSTOLIC_BUILD/bin:LLVM_BUILD/bin:/usr/local/bin:/usr/bin:/bin` | everything |
| unset | `LD_PRELOAD`, `LD_LIBRARY_PATH`, `SYSTOLIC_DPTI_COSIM_SOCKET`, `SYSTOLIC_FPGA_CONFIG` | — |

The compiled programs need no `LD_LIBRARY_PATH`. libftdi1 is a system library,
and `libmlir_c_runner_utils.so` is linked with an rpath.

`env.board.sh` adds:

| Variable | Value |
|---|---|
| `SYSTOLIC_FPGA_CONFIG` | `<repo>/artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/fpga_config.txt` |
| `PATH` (only if `vivado` is not found) | `+ $XILINX_VIVADO/bin`. `XILINX_VIVADO` is the variable Vivado's `settings64.sh` sets. |

`env.cosim.sh` adds:

| Variable | Value |
|---|---|
| `COSIM_BUILD_DIR` | `SYSTOLIC_BUILD/cosim`. This is `test/e2e/cosim/build.sh`'s own variable. |
| `SYSTOLIC_FPGA_CONFIG` | `COSIM_BUILD_DIR/fpga_config.txt`, written by the co-simulation build |
| `LD_PRELOAD`, `SYSTOLIC_DPTI_COSIM_SOCKET`, `SYSTOLIC_DPTI_TIMEOUT_MS=600000` | Set by `run_cosim.sh` **only on the one program it runs** against a live Vcosim, never exported |

The co-simulation does not use the board's `fpga_config.txt`. Both have
K_MAX 16/16, the completion record and `PIPELINE_DEPTH=4`. They differ in
`DDR_BYTES`: the co-simulation's DDR model holds 1 MiB, the board 512 MiB, and
the asynchronous runtime sizes its buffers to `DDR_BYTES`.

You can override these variables on the command line, for example
`SYSTOLIC_BUILD=/tmp/b bash scripts/reproducibility/run_cosim.sh e2e`. That is
how the fresh-build test in section 15 was run. A configuration file that
`SYSTOLIC_FPGA_CONFIG` points to in your shell is **ignored**; each mode sets
its own. `check_env.sh` lists any such variables it found.

Most variables need no manual `export`. The exception is the first configure,
which needs the toolchain locations as arguments (section 5).

## 5. Building systolic-cc

First build on a machine: give the LLVM 24 build tree and clang 24 once.

```bash
cd ~/work/systolic-mlir
bash scripts/reproducibility/build.sh --llvm-build <LLVM24_BUILD_DIR> --clang <CLANG24>
#   e.g. <LLVM24_BUILD_DIR> = the llvm-project build with lib/cmake/llvm and lib/cmake/mlir
#        <CLANG24>          = .../bin/clang reporting "clang version 24."
```

This runs:

```bash
cmake -G Ninja -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DLLVM_DIR=<LLVM24_BUILD_DIR>/lib/cmake/llvm -DMLIR_DIR=<LLVM24_BUILD_DIR>/lib/cmake/mlir \
  -DSYSTOLIC_CFRONT_CLANG=<CLANG24>
cmake --build build
```

`build.sh` refuses an LLVM or clang that is not version 24. It then checks
that these were built: `bin/systolic-cc`, `systolic-cfront`, `systolic-opt`,
`systolic-matmul`, `runtime-dispatch-test`, `runtime-async-test` and
`runtime/libsystolic_fpga_runtime.a`. The default (all) target builds all of
them; no other CMake target is needed.

Later builds need no arguments, because the locations are read back from
`build/CMakeCache.txt`:

```bash
bash scripts/reproducibility/build.sh            # incremental
bash scripts/reproducibility/build.sh --cosim    # also builds Vcosim (Co-sim Mode)
```

Optional regression tests use lit and need `lit`, plus `count` and `not`
built from LLVM 24 (the LLVM 24 build tree does not have them):
`cmake --build build --target check-systolic`.

## 6. FPGA Bitstream Information

The one bitstream this reproduction uses:

```text
artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit
SHA256 bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5
```

| Key (`fpga_config.txt`) | Value |
|---|---|
| Fleet | `NUM_8X8=1` (8×8), `NUM_4X4=3` (4×4), `N=8` |
| K_MAX | `K_MAX_8X8=16`, `K_MAX_4X4=16`, `K_DIM=16` |
| Operand path | `VARIANT=v1`, `USE_V2=0` |
| Host protocol | `HOST_SLAB_BYTES=1024`, `COMPLETION_HEADER=1`, `PIPELINE_DEPTH=4`, `DDR_BYTES=536870912` |
| RTL | `RTL_GIT_REV=229fe2431c0c72d25e6654a88e8e5d47c697301b`, `RTL_TREE_DIRTY=1`. The archived configuration records `RTL_TREE_DIRTY=1`, so the recorded Git revision alone does not identify the complete RTL source state used for this build. |
| Implementation | `IMPL_PLACE_DIRECTIVE=ExtraTimingOpt`, `IMPL_ROUTE_DIRECTIVE=Explore`, `DPTI_CLKOUT0_PHASE=0.0`, built `2026-10-25T13:08:22+0800`, Vivado v2026.1 |
| Timing (routed report) | WNS **+0.099 ns**, TNS 0, WHS **+0.016 ns**, THS 0, 0 failing endpoints; "All user specified timing constraints are met." `TIMING_MET=1` |
| DPTI I/O gate (`dpti_io_summary.txt`) | **PASS**: no unconstrained FT2232H ports, every DPTI pin's setup/hold positive, I/O registers in ILOGIC/OLOGIC |
| Utilization (placed report) | Slice LUTs 117110 / 133800 (87.53 %), Slice Registers 157059 / 267600 (58.69 %), Slices 33268 / 33450 (99.46 %), Block RAM tiles 40 / 365 (10.96 %), DSPs 672 / 740 (90.81 %) |

Files in the directory: `fpga_config.txt`, `SHA256SUMS`, `dpti_io_summary.txt`,
`systolic_dma_top_timing_summary_routed.rpt`,
`systolic_dma_top_utilization_placed.rpt` and `.ltx` are committed. **The
`.bit` itself is not**: `*.bit` is in `.gitignore`. A fresh clone therefore
has every file except the bitstream. Obtain the `.bit` separately and verify
it:

```bash
cd artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16 && sha256sum -c SHA256SUMS
```

`fpga_config.txt` records `BITSTREAM=systolic_dma_top_1x8x8_3x4x4.bit`, the archived bitstream filename, together with its SHA256.
The configuration is covered by `SHA256SUMS` and is left unchanged.
The runtime was checked to accept this `fpga_config.txt` as is:

```text
$ SYSTOLIC_FPGA_CONFIG=$PWD/artifacts/.../2026-10-25-k8_16-k4_16/fpga_config.txt build/bin/runtime-dispatch-test --print-config
K_MAX_8X8=16  K_MAX_4X4=16  SLAB_BYTES=1024  PIPELINE_DEPTH=4  COMPLETION_HEADER=1
```

No bitstream was regenerated for this document.

## 7. Programming FPGA

Program the FPGA with the artifact's bitstream itself:

```bash
cd ~/work/systolic-mlir

bash scripts/reproducibility/run_board.sh verify-program   # the .bit is there and its SHA256 is the artifact's: MATCH
XILINX_VIVADO=<Vivado dir> bash scripts/reproducibility/run_board.sh program
```

`verify-program` checks that
`artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit`
exists and that its SHA256 is `bd8aba72…41f3db5` (section 6). It reports MATCH,
MISMATCH (another file is there) or NOT VERIFIED (no file: `*.bit` is not in
git, section 6). `program` runs only after MATCH, and then runs Vivado's
hardware manager on exactly that file:

```bash
vivado -mode batch -nojournal \
  -source scripts/reproducibility/program_bit.tcl \
  -tclargs artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16/systolic_dma_top_1x8x8_3x4x4.bit
```

`program_bit.tcl` takes the JTAG target whose chain holds the xc7a200t (the
FT2232H registers two targets and only one reaches the FPGA; the other is
reported as "No devices detected" and skipped), programs the file, and exits
non-zero unless the device reports DONE. It needs no Vivado project, does not
synthesize, implement or write a bitstream, and is the same on every machine
that has the artifact's `.bit`.

**Not this command.** `dma_top_build.tcl -tclargs program v1 16 16 1 1 3`
programs the first `.bit` of a Vivado project under
`$HOME/work/vivado/tile_pipelined/`, not the artifact. On the development host
the project it opens holds an older build (2026-10-18, `ad0f74e8…`, RTL
15 files behind `main`), so that command programs a **different** bitstream.
It remains the build flow's programming step for a bitstream you build
yourself; to reproduce this document, use `run_board.sh program`.

Checked on 2026-10-22 on the development host: `verify-program` MATCH;
`program` programmed the artifact file (Digilent 210276C08FC0B, xc7a200t_0,
DONE = 1); the board runs of section 15 marked 2026-10-22 followed it.

The FPGA cannot report which bitstream it holds, because DPTI has no register
read: board runs trust that it holds the one programmed last. Program it
before a board run whenever that is in doubt; programming takes seconds.

## 8. Board Mode

```bash
cd ~/work/systolic-mlir
# 1. Build systolic-cc + runtime (section 5)
bash scripts/reproducibility/build.sh
# 2. Requirements, configuration, artifact hashes, FT2232H on USB
bash scripts/reproducibility/run_board.sh check
# 3. Program the specified bitstream (section 7)
bash scripts/reproducibility/run_board.sh verify-program
XILINX_VIVADO=<Vivado dir> bash scripts/reproducibility/run_board.sh program
# 4. Board E2E tests (existing harness, artifact configuration)
bash scripts/reproducibility/run_board.sh e2e                  # = test/e2e/run_e2e.sh --backend board --quick
bash scripts/reproducibility/run_board.sh e2e --mode async     # the asynchronous runtime, same cases
# 5.-7. Your C GEMM: compile, run on the FPGA, compare with the CPU reference
bash scripts/reproducibility/run_board.sh c test/reproducibility/gemm_example.c
```

`run_board.sh e2e` is the existing test with the environment set up for it:

```bash
cd ~/work/systolic-mlir
bash test/e2e/run_e2e.sh --backend board --quick     # needs SYSTOLIC_FPGA_CONFIG, which env.board.sh sets
```

Run it only when nothing else is using the board or the FT2232H.

**Evidence that the FPGA computed the result (not the CPU)**

- `systolic-matmul`, which `run_e2e.sh` uses, has no CPU fallback. Every case
  reports the bytes it sent and received over DPTI. The runtime layer step
  (`runtime-dispatch-test`) sends raw tiles to devices 0 to 3 and checks them.
- `run_board.sh c` prints `ldd`'s libftdi1, which is the system library, and
  shows that no `[libftdi_cosim]` line appears. It prints the driver's per-GEMM
  `[systolic-cc] FPGA Executed: GEMM #N` line. That line is reported only when
  the real hardware runtime completed every planned invocation without
  failure; otherwise the shim undoes the kernel's writes, reruns the function
  on the CPU and prints `CPU Executed`. It also prints the runtime's
  `[fpga-async] timing` line, with `completed=` invocations and
  `bytes_to_fpga=` / `bytes_from_fpga=` on the wire.
- The script exits 3 (`NOT ON THE FPGA`) when no GEMM reports
  `FPGA Executed`.

## 9. Co-simulation Without FPGA

**Is C-to-RTL co-simulation supported? Yes.** The pieces below are all in the
repository and were run end to end for this document.

| Bridge | File | What it does |
|---|---|---|
| Compiler-generated calls | systolic-cc → `fpga_tile_submit` / `fpga_gemm_wait` (async) or `fpga_matmul_tiled_auto_scheduled` (sync) | Same binary as for the board (`--runtime=board`) |
| Runtime | `runtime/fpga_async_runtime.cpp`, `fpga_matmul_tiled.cpp`, `dpti_ftdi.cpp` | Unmodified: slab packing, job descriptors, completion records |
| DPTI interface | `test/e2e/cosim/libftdi_cosim.c` (LD_PRELOAD) | Replaces the libftdi1 calls and forwards the byte stream over a UNIX socket |
| Simulated FT2232H | `test/e2e/cosim/dpti_cosim.cpp`, `tb_dpti_cosim.sv`, `dpti_protocol_check.svh` | Socket ↔ sync-FIFO pins, protocol checks |
| RTL | `rtl/multi_200t/tile_pipelined/dma/*` (the `systolic_dma_top` source list of `dma_top_build.tcl`) | Same RTL as the bitstream (section 6) |
| Replaced Xilinx IP | `test/e2e/cosim/fp_ieee.sv` (+ `fp_ieee_dpi.cpp`), `tb/mig_axi_model.sv` (AXI DDR model, 1 MiB), `tb/xil_stubs.sv` | FP mul/add, MIG/DDR3, primitives and VIO |

```bash
cd ~/work/systolic-mlir
bash scripts/reproducibility/build.sh                     # systolic-cc + runtime
bash scripts/reproducibility/run_cosim.sh build          # Vcosim, libftdi_cosim.so, fpga_config.txt
bash scripts/reproducibility/run_cosim.sh check          # needs no FPGA, USB device or Vivado
bash scripts/reproducibility/run_cosim.sh e2e            # = run_e2e.sh --backend cosim --quick
bash scripts/reproducibility/run_cosim.sh c test/reproducibility/gemm_example.c
```

`run_cosim.sh c` performs these steps:

1. Starts `Vcosim +socket=<short /tmp path> +max_sessions=0`. AF_UNIX paths
   end at about 107 bytes.
2. Compiles the program with `systolic-cc --runtime=board --fp-reassociation`
   against the co-simulation's `fpga_config.txt`.
3. Runs the program with `LD_PRELOAD=<cosim>/libftdi_cosim.so
   SYSTOLIC_DPTI_COSIM_SOCKET=<socket>`.
4. Stops Vcosim by PID.
5. Prints the simulation's own account of the session from `cosim.log`, for
   example `cosim: session 1 closed (host closed): host->fpga 60318 B
   received, 60318 B consumed by the RTL, fpga->host 6624 B`. The runtime's
   `[libftdi_cosim] connected to RTL co-simulation at …` line confirms that
   the simulator, not a USB device, was used.

To do the same by hand:

```bash
cd ~/work/systolic-mlir
C=build/cosim; D=$(mktemp -d /tmp/systolic-cosim.XXXXXX)
L=$(sed -n "s|^LLVM_DIR:[A-Z]*=\(.*\)/lib/cmake/llvm\$|\1|p" build/CMakeCache.txt)   # the LLVM 24 build
$C/Vcosim +socket=$D/dpti.sock +max_sessions=0 > $D/cosim.log 2>&1 &  VPID=$!
sleep 1
SYSTOLIC_FPGA_CONFIG=$PWD/$C/fpga_config.txt \
  build/bin/systolic-cc --runtime=board --fp-reassociation --llvm-build "$L" \
  test/reproducibility/gemm_example.c -o $D/gemm_example
LD_PRELOAD=$PWD/$C/libftdi_cosim.so SYSTOLIC_DPTI_COSIM_SOCKET=$D/dpti.sock \
  SYSTOLIC_FPGA_CONFIG=$PWD/$C/fpga_config.txt $D/gemm_example
kill $VPID; grep '^cosim: session' $D/cosim.log
```

(`--llvm-build`: systolic-cc otherwise looks for LLVM 24 in `LLVM_BUILD`,
default `$HOME/work/llvm-project/build`; the scripts set `LLVM_BUILD` from
`CMakeCache.txt`.)

What it is not:

- It is a simulation. It shows that the RTL behind the unmodified host stack
  computes correct results. It is not evidence about a particular board,
  bitstream or speed.
- The Xilinx floating-point IP is replaced by an IEEE model, and the
  MIG/DDR3 by an AXI model.
- `+max_sessions=0` keeps one Vcosim alive for many programs. Start a fresh
  one after a program aborts mid-session.

## 10. Writing Your Own C GEMM

`test/reproducibility/gemm_example.c` is the template. To be offloaded, a
function must meet these conditions:

- `void` return type, parameters are arrays with constant extents and
  **`restrict`** (operands that may alias are not lowered).
- Operands are **`float`** (fp32): the FPGA datapath is binary32. A `double`
  GEMM stays on the CPU.
- The loop nest is `C[i][j] += A[i][k] * B[k][j]`, or an equivalent affine
  form; see `docs/AFFINE_GEMM_MAPPING.md`. Clearing C first (`C[i][j] = 0`
  in its own nest) is allowed.
- No local variables other than loop counters. A function that has them is
  "outside the supported C subset" and stays C. The example's
  `gemm_reference` uses this on purpose so that it always runs on the CPU.
- Compile with **`--fp-reassociation`**. Offloading sums each K tile from zero
  and adds it to C, which is a different order of IEEE additions. Under the
  default strict FP mode every GEMM stays on the host, and `--explain` says so.
- M, N and K are arbitrary: edge tiles are padded and K is folded by K_MAX.
  `main` and anything else outside the subset stay C, unchanged.

Use `--explain` to see the decision for each function:

```text
note: 'gemm' is offloaded: 45 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet
note: 'gemm_reference' is outside the supported C subset and is not translated: local variables are not supported (only for-loop counters)
```

The invocation count comes from the binary search's cost model: Cost Model
V2 (the default, `docs/COSTMODEL_V2.md`) gives 45; `systolic-cc --cost-model
v1` gives the 54 of Cost Model V1.

## 11. Required C Headers

**For a C user of systolic-cc: none beyond the C standard library.** The
example includes only `<stdio.h>` and `<math.h>`. systolic-cc adds
`-I<repo>/runtime/offload` itself. `systolic_offload.h` there is an internal
header used by the generated call shims, and user programs do not need to
include it.

**For a developer calling the DPTI runtime directly** (no systolic-cc):

| Header | API |
|---|---|
| `runtime/fpga_matmul_tiled.h` | `fpga_matmul_tiled_auto_scheduled` (one tile on one device), `fpga_runtime_get_config`, `fpga_runtime_get_stats`, `fpga_runtime_close` |
| `runtime/fpga_async_runtime.h` | `fpga_gemm_begin`, `fpga_tile_submit`, `fpga_gemm_poll` / `wait` / `wait_for` / `release` (docs/ASYNC.md) |

Compile and link line (checked with a one-tile program against the
co-simulation, PASS):

```bash
gcc -std=c11 -I runtime my_dpti.c build/runtime/libsystolic_fpga_runtime.a \
    $(pkg-config --libs libftdi1) -lstdc++ -o my_dpti
SYSTOLIC_FPGA_CONFIG=<the target's fpga_config.txt> ./my_dpti
```

The existing examples are `test/e2e/runtime_dispatch_test.cpp` and
`runtime_async_test.cpp`, built as `build/bin/runtime-{dispatch,async}-test`.
Calling the runtime directly means you handle the slab and K_MAX limits that
systolic-cc otherwise takes care of: one call per tile, M,N ≤ 8 on device 0
and ≤ 4 on devices 1 to 3, 1 ≤ K ≤ K_MAX.

## 12. Compiling and Running

Through the scripts (recommended; they set every variable):

```bash
bash scripts/reproducibility/run_board.sh c path/to/your.c   [OUTDIR]    # FPGA
bash scripts/reproducibility/run_cosim.sh c path/to/your.c   [OUTDIR]    # RTL simulator
```

Outputs go to `build/repro/{board,cosim}/<name>/`: `<name>` (the binary),
`compile.log`, `stdout.txt`, `run.log` (runtime trace) and, for cosim,
`cosim.log`. Intermediate IR is in `<name>.systolic/`.

The underlying systolic-cc invocation:

```bash
SYSTOLIC_FPGA_CONFIG=<config> build/bin/systolic-cc --runtime=board --fp-reassociation [--explain] [--verify] your.c -o your
```

| Option | Meaning |
|---|---|
| `--runtime=board` | Link the real runtime (DPTI). The same binary serves the board and the co-simulation; the backend is chosen at run time by `LD_PRELOAD` of `libftdi_cosim.so` or its absence. |
| `--runtime=soft` | Software model of the runtime. **Not the FPGA**; GEMMs report `CPU Executed`. |
| `--fpga-config FILE` | Same as `SYSTOLIC_FPGA_CONFIG`: K_MAX, completion record and pipeline depth of the target |
| `--submission auto\|sync\|async` | `auto` (default) uses async when the configuration has `COMPLETION_HEADER=1` |
| `--verify` | Also compares every offloaded call with the CPU version; exit 4 on a mismatch |
| `--explain` | Explains the offload decision for each function |

The binary itself needs no environment variables on the board except
`SYSTOLIC_FPGA_CONFIG`. Without it, the runtime falls back to its built-in
default configuration, which has no completion record. The bitstream sends
the record, so the runtime refuses the result, and every GEMM falls back to
the CPU.

## 13. Numerical Validation

- **In the program**: `gemm_example.c` computes a `double` CPU reference,
  allows a relative tolerance of 1e-5, prints the first wrong elements, and
  prints `PASS` (exit 0) or `FAIL` (exit 1). Its inputs are small integers, so
  every partial sum is exact in fp32. Any summation order gives identical
  bits, and the observed `max |error|` is 0.
- **By the driver**: `systolic-cc --verify` compares every offloaded call with
  the function compiled for the CPU, and the program exits 4 on a mismatch.
- **By the E2E harness**: `run_e2e.sh` compares every case element by element
  with an exact integer reference and classifies it as PASS, MISMATCH,
  TRANSPORT, CONFIG and so on (`test/e2e/e2e_verdict.py`).
- **Board vs co-simulation**: the example moved exactly the same bytes in
  both modes (60318 B to the FPGA, 6624 B back, 54 invocations; recorded
  with Cost Model V1's decomposition, before V2 became the default).

## 14. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `check_env.sh`: `MISSING libftdi1`, `verilator`, `FT2232H` | A system package or the board is missing. Install or connect it; no environment variable fixes this. |
| `no LLVM 24 build / clang 24 known` | First configure: `build.sh --llvm-build DIR --clang CLANG` |
| `… is LLVM 18…; this tree needs LLVM/MLIR 24` | The tree does not build with LLVM 18; point `--llvm-build` at an LLVM 24 build tree |
| `'gemm' stays on the host: … --fp-reassociation` | Add `--fp-reassociation` (section 10) |
| `[systolic-cc] CPU Executed` for your GEMM on the board | An invocation failed and the call was redone on the CPU. Check `run.log` for the runtime's rc (`-3` link not opened: board unplugged, link in use, or USB permissions; `-9` completion record mismatch: wrong configuration for the loaded bitstream) |
| `run_e2e.sh` stops with exit 2 (`CONFIG`) | No or the wrong `SYSTOLIC_FPGA_CONFIG`; use the scripts, which set it |
| Co-sim: `TRANSPORT rc=-3`, `cosim: socket path too long` | The socket path must stay under ~100 bytes; the scripts put it under `/tmp/systolic-cosim.*` |
| Co-sim build: `%Error: Internal Error: attempted to destroy locked Thread Pool` | A Verilator 5.020 failure seen with `-j 8` on the test host; rerun with fewer jobs: `JOBS=6 bash scripts/reproducibility/run_cosim.sh build` (`JOBS` is `test/e2e/cosim/build.sh`'s variable; `build.sh --jobs N --cosim` passes it) |
| Co-sim: the next run hangs after an aborted program | Restart Vcosim; `run_cosim.sh c` starts a fresh one every time |
| `verify-program`: MISMATCH | The file at the artifact path is not the artifact's bitstream; replace it with the right one (`sha256sum -c SHA256SUMS`, section 6) |
| `verify-program`: NOT VERIFIED | The artifact's `.bit` is not on this machine (`*.bit` is not in git); obtain it (section 6) |
| `program`: `no xc7a200t on this chain` for every target, or `cannot reach hw_server` | Board off, the PROG micro-USB not in this machine, or the Vivado cable drivers missing (section 2) |
| Board results look stale or wrong after another program used the link | Only one program at a time may use the FT2232H; rerun when the board is free |

## 15. Reproducibility Checklist

Results on `main`, development host, Nexys Video attached: rows 1-12, 15
and 16 of 2026-10-21 (Cost Model V1 was the default then, hence 54
invocations for the C example), rows 13-14b of 2026-10-22 (Cost Model V2):

| # | Check | Command | Result |
|---|---|---|---|
| 1 | Shell syntax | `bash -n scripts/reproducibility/*.sh` | PASS (shellcheck not installed: NOT RUN) |
| 2 | Environment scripts | `env -i … bash scripts/reproducibility/check_env.sh all` | PASS: every item OK; no inherited systolic variable |
| 3 | Isolation | `check_env.sh board` with wrong `SYSTOLIC_FPGA_CONFIG`, a cosim `LD_PRELOAD`, `LD_LIBRARY_PATH` and llvm-18 first on `PATH` exported | PASS: all of them replaced or unset, artifact configuration used |
| 4 | Fresh build in a clean env | `env -i … SYSTOLIC_BUILD=<new dir> bash scripts/reproducibility/build.sh --llvm-build … --clang … --cosim` | PASS (58/58 build steps + Vcosim, 4 min 37 s) |
| 5 | Incremental build of `build/` in a clean env | `env -i … bash scripts/reproducibility/build.sh` | PASS |
| 6 | `SYSTOLIC_FPGA_CONFIG` set by script only | runs 7 to 11 under `env -i`, variable never exported by hand | PASS |
| 7 | C example, software model (lit) | `lit build/test/reproducibility build/test/SystolicCC` | PASS 28/28 |
| 8 | C example, co-simulation (fresh build, `env -i`) | `run_cosim.sh c test/reproducibility/gemm_example.c` | PASS: GEMM #1 FPGA Executed, 54 invocations, 0/480 wrong; Vcosim session 60318 B in / 6624 B out |
| 9 | Co-sim E2E (fresh build, `env -i`) | `run_cosim.sh e2e` | PASS: 26/26, configuration refusal PASS, runtime layer PASS, injected write failure reported |
| 9a | Co-sim rebuilt in the default `build/cosim` (`env -i`) | `JOBS=6 run_cosim.sh build`, then `run_cosim.sh e2e`, `run_cosim.sh e2e --quick --mode async`, `run_cosim.sh c …` | PASS (sync 26/26, async 26/26 + RUNTIME ASYNC PASS, example PASS). The first attempt with the default `-j 8` hit the Verilator 5.020 internal error (section 14) |
| 9b | Manual co-sim commands of section 9, verbatim (`env -i`) | — | PASS |
| 10 | Board E2E (`env -i`) | `run_board.sh e2e` | PASS: 26/26 on backend board, configuration refusal PASS, runtime layer PASS (hardware configuration not independently verified) |
| 10a | Board E2E, async runtime (`env -i`) | `run_board.sh e2e --quick --mode async` | PASS: 26/26, runtime-async-test PASS (mode F NOT APPLICABLE on the board, by design) |
| 11 | C example on the board (`env -i`) | `run_board.sh c test/reproducibility/gemm_example.c` | PASS: GEMM #1 FPGA Executed, 54 invocations completed, 0 failed, 0/480 wrong, system libftdi1 |
| 12 | Direct DPTI API | one 8×16×8 tile via `fpga_matmul_tiled_auto_scheduled`, co-simulation | PASS |
| 13 | Bitstream to program (2026-10-22, `env -i`) | `run_board.sh verify-program` | MATCH (`18329505…`) |
| 14 | Programming (2026-10-22, `env -i`) | `run_board.sh program` | PASS: the artifact file programmed, xc7a200t_0, DONE = 1 |
| 14a | Board E2E after programming (2026-10-22, `env -i`) | `run_board.sh e2e` (all cases), `run_board.sh e2e --quick --mode async` | PASS: 43/43; async 26/26 (Cost Model V2) |
| 14b | C example on the board (2026-10-22, `env -i`) | `run_board.sh c test/reproducibility/gemm_example.c` | PASS: GEMM #1 FPGA Executed, 45 invocations (Cost Model V2), 0/480 wrong |
| 15 | Full lit suite | `lit build/test` (LLVM 24 `count`/`not` on PATH, developer shell) | PASS 154/154 |
| 16 | Whitespace | `git diff --check` | PASS |

For your own run:

- [ ] `check_env.sh all` (or `board` / `cosim`) reports OK
- [ ] `build.sh` (first time with `--llvm-build` / `--clang`) succeeded
- [ ] Board: the `.bit` in hand matches `SHA256SUMS`; `verify-program` MATCH before programming
- [ ] `run_board.sh e2e` / `run_cosim.sh e2e`: `E2E: PASS`
- [ ] `run_*.sh c test/reproducibility/gemm_example.c`: `FPGA Executed: GEMM #1` and `PASS`
- [ ] Recorded `git rev-parse HEAD`, the bitstream SHA256 and the logs under `build/repro/` and `build/e2e/`

## 16. Known Limitations

- **The `.bit` is not in git.** A fresh clone needs the bitstream from
  elsewhere; `SHA256SUMS` verifies it.
- **Program with `run_board.sh program`, not `dma_top_build.tcl program`.**
  The latter programs a Vivado project's own `.bit`, which on the
  development host is an older build (section 7).
- **What the board holds cannot be read back.** DPTI has no register read,
  and board runs trust the configuration.
- **Toolchain locations are machine-specific.** LLVM 24 and clang 24 are
  given once to `build.sh` and read back from `CMakeCache.txt`; there is no
  packaged toolchain. `env -i` does not isolate installed packages (section 3).
- **The co-simulation is not the board.** It uses an IEEE FP model instead of
  the Xilinx FP IP and a 1 MiB AXI DDR model instead of MIG/DDR3, it runs at
  Verilator speed, and it gives no timing evidence about the board.
- **C subset.** fp32 only, `restrict` arrays with constant extents, no local
  variables in an offloaded function, `--fp-reassociation` required for
  offload. Broadcast and other non-GEMM patterns stay on the CPU
  (`docs/AFFINE_GEMM_MAPPING.md`).
- **Single user of the link.** Board runs need exclusive use of the FT2232H.
- **lit** needs `lit` and LLVM-24-built `count` / `not`. The scripts do not
  provide them, and `check-systolic` in a pure `env -i` shell is NOT RUN.
