# Cost Model V2: the FPGA fleet as one machine

**2026-10-21.** Cost Model V2 is the binary search's default cost model
(`cost-model=v2`); Cost Model V1 (`docs/COSTMODEL.md`) stays selectable with
`cost-model=v1` / `--cost-model v1` and prices exactly as before.

| | V1 | V2 |
|---|---|---|
| what it models | every accelerator on its own | the whole `systolic_dma_top`: four contexts and everything they share |
| operand fill | `T_read = 128 * ceil(k/8) + 11` always | the shared read engine beat by beat |
| shared read port | optional total-bandwidth bound only | round robin, burst lengths, outstanding limit, in-order return, head-of-line blocking |
| write-back | per accelerator | one engine, lowest-numbered context first |
| result readback | not modelled | the readback FSM, the 8-beat output FIFO, the DPTI byte path |
| job port | not modelled | the registered acceptance, queue depth, `RB_DEPTH` |
| against the RTL bench (1782 job lists) | mean 15 %, max 78 % device-makespan error | **every event identical** |

Code: `include/Systolic/CostModelV2.h`, `lib/Systolic/Transforms/CostModelV2.cpp`
(the simulator); `SystolicOptimizer.cpp` (the search's use of it);
`eval/cost_model_v2/` (the bench and the validation).

## 1. What "shared" means here

Shared means shared **hardware**: one AXI read port, one write-back engine,
one readback path, one job port. Two accelerators that need the same A tile
still fetch it twice: every invocation has its own operand slab (A and B
interleaved, `runtime/fpga_async_runtime.cpp`), the RTL reads each job's slab
at its own `a_base` and routes every beat to exactly one context
(`systolic_dma_top.sv` tag match). The model counts every job's bytes; it does
not deduplicate, cache, broadcast or multicast anything, because the hardware
does none of it.

| concept | in the hardware | in V2 |
|---|---|---|
| logical operand reuse (tiles share A rows / B columns) | exists in the math | ignored, like the hardware |
| physical DDR3 read reuse | none | none |
| shared AXI read port | yes | modelled |
| DMA request coalescing | none | none |
| broadcast / multicast | none | none |
| read scheduling overlap (fills of several contexts in flight) | yes | modelled |

## 2. The simulator

`simulateFleet(hardware, jobs)` steps the machine cycle by cycle: the array
clock (100 MHz) for the port, the contexts and the engines, the DPTI clock
(60 MHz) for the output path. Nothing is approximated: no interpolation, no
extrapolation, no sampling, no truncation. Every rule is the RTL's:

| part | rule | RTL |
|---|---|---|
| job port | the job offered at cycle s with its context ready fires at s + 1; the cycle after a fire accepts nothing; ready = queue (`JOB_QUEUE_DEPTH` = 4) not full and fewer than `RB_DEPTH` = 8 jobs not read back | `systolic_dma_top.sv` `accept_q`, `core_job_ready`, `result_readback_busy` |
| context | V1's recurrences (accept, fill start, fold, publish, write-back stage), with the fill's length from the engine: descriptor = fill start + 3, landed = last beat + 5 (v1) / + 2 (v2), fold no earlier than landed + 4 | `systolic_accel_context.sv` `fill_ok`, `fill_complete`, the release handshake |
| read engine | 4 operand slots + the readback slot; round robin from the slot after the last grant; 16-beat bursts while one slot is active, `SHARED_BURST_LEN` (1 on v1, 16 on v2) while two or more are; bursts clamped at 4 KiB pages; at most 8 bursts in flight (the AR register counted); an AR register; one ID, so data return in request order and the destination of the beat at the head stalls every beat behind it | `dma_engine_multi.sv` |
| operand writer | v1: a beat in 4 cycles, full except on its last; v2: a beat a cycle, never full; alone, a slot issues a new burst only while its writer is not full | `dma_operand_writer.sv`, `dma_operand_writer_v2.sv` |
| write-back | published -> request after `rows^2 + 5` cycles (the scan); the engine grants the lowest-numbered request when idle; done `rows^2/4 + 6` cycles later (one less when the request waited: its first beat is staged); next grant the cycle after | `systolic_accel_context.sv` W_SCAN, `systolic_dma_top.sv` write-back arbiter |
| readback | per-context completion queues; round robin from the context after the last served; the 16-byte record into the FIFO, then a 16- (8x8) or 4-beat (4x4) read through the STRICT_GATE slot (a burst only with FIFO room and nothing outstanding) | `systolic_dma_top.sv` RB_IDLE / RB_HDR / RB_DATA |
| output path | `dpti_async_fifo` depth 8 with two-flop Gray-pointer synchronizers both ways; `dpti_beat_to_byte`; the sync245 PHY's TX: a byte per two DPTI clocks, a 4-byte TX FIFO, turnarounds | `dpti_output_cdc.sv`, `dpti_async_fifo.sv`, `dpti_beat_to_byte.sv`, `dpti_sync245_phy.sv` |

Cycles in which provably nothing can change (every engine, queue and FIFO
empty, the next event known) are skipped; the result is identical with and
without the skip (checked on every validation case). A deadlock -- no event
possible while jobs remain, or nothing moving for 10^7 cycles with work in
flight -- is reported as an error, never estimated past.

**Inputs that are not the RTL's.** The phase between the array clock and the
DPTI clock (`dptiPhasePs`, default 0) is a physical input: the bench fixes it,
the board does not. The validation feeds each bench run's measured phase; with
the default phase 5 of 70 compiler schedules differ from the RTL by 1-2 cycles
(at most 0.009 %).

## 3. The binary search under V2

The search itself is unchanged: `binarySearchFeasible`, the search loop, the
upper bound and its doubling are V1's code. V2 changes only how
`buildSearchProblem` fills each geometry's tile stream:

1. **Saturated streams.** Every context of the fleet runs tiles of its own
   geometry (each tile its `ceil(K / k_max)` K-folded invocations), offered as
   the runtime offers them (a descriptor only to a context with room), all at
   once. The simulation is repeated with longer streams until every context
   was still busy when the last measured tile ended (each run doubles the
   streams that ran dry). Every tile is simulated; nothing is extrapolated.
2. A geometry's stream is the pointwise maximum over its contexts of the tile
   end (and start) cycles. Each context's tile ends strictly increase, so the
   stream does; this is checked, not assumed.
3. `capacity_g(T) = #accelerators(g) * tilesBy(T)` as in V1.

**Monotonicity.** `tilesBy(T)` is nondecreasing in T for any nondecreasing
stream, so V2 keeps the predicate's structure: the shared-read bound and the
greedy placement are untouched. That is the same argument V1 rests on, with
the same caveat (the greedy placement; geometries that divide each other).
It is not assumed: `systolic-cost scan-feasible` evaluates the unchanged
predicate at **every** deadline 0..2T* and reports any T with feasible(T) and
not feasible(T + 1). Results: section 5.

**Planning makespan vs predicted makespan.** The search's T* (the remark's
"planning makespan") prices a saturated fleet. After the search, the chosen
schedule itself is simulated (the tiles in decomposition order, the runtime's
offer policy): the "predicted device makespan". They are different scenarios
and neither bounds the other -- e.g. 16x16x33 on the pipelined fleet: 2458
planned, 2954 predicted, because the 4x4s' results queue for the readback path
in front of the 8x8's last fills (each 8x8 readback overflows the 8-beat FIFO
and stalls the in-order read channel).

**What does not change.** `est_cycles` and `start_cycle` keep their meaning:
the invocation's compute term and its fold start on the accelerator's
compute-only timeline. The hardware scheduler holds a fold until its
`cycle_counter` reaches `start_cycle` (`systolic_hw_scheduler.sv`), so the IR
value is part of the hardware's behaviour and V2 does not touch it.
`SystolicTilePlan` holds every tile's scheduled interval against the stream
the search used (rebuilt independently).

## 4. Using it

| | V2 (default) | V1 |
|---|---|---|
| `--systolic-binary-search-tile-matmul` | `cost-model=v2` | `cost-model=v1` |
| `--affine-matmul-to-systolic` | `cost-model=v2` | `cost-model=v1` |
| `--systolic-fpga-pipeline` | `cost-model=v2` | `cost-model=v1` |
| `systolic-matmul` | `--cost-model=v2` | `--cost-model=v1` |
| `systolic-cc` | `--cost-model v2` | `--cost-model v1` |
| `systolic-cost schedule / check-monotone / scan-feasible` | `--cost-model v2` | `--cost-model v1` |

`operand-path=v2` / `--v2-operand-path`: the beat-wide operand path (a v2
bitstream); default v1, the production bitstream. A fleet the simulator cannot
model (devices without the FPGA transport declaration -- compute-only test
fleets) is priced by V1 and the `report` remark says so.

`systolic-cost fleet-sim --jobs FILE` runs a job list (the bench's format) on
the simulator and prints every event; `scan-feasible` scans the predicate.

## 5. Validation

`python3 eval/cost_model_v2/run_validation.py` (eval/cost_model_v2/README.md).
All numbers are RTL **simulation** (Verilator, the bitstream's RTL); **no
board measurement** exists for any of this -- the board's DDR3 (MIG) timing,
the USB side of the DPTI link and the host are UNCALIBRATED.

- **Exact replay.** 1298 job lists on the v1 operand path (K_MAX 16/16,
  24/12, 32/32, 64/64, 256/64; single accelerators, every combination,
  submission gaps 0/3/40/300, long streams, one-outstanding chains, compiler
  schedules, random mixes; K across the fold boundaries) and 484 on the v2
  path: every job's acceptance, descriptor, last beat, landing, fold, publish,
  write-back grant, completion, readback take and readback done, and the AXI
  read request count, identical to the RTL in every case.
- **The compiler's prediction.** 70 schedules (14 GEMM shapes x 5 K_MAX), the
  runtime's offer order, replayed on the RTL bench: identical.
- **Error of the device makespan** (V2 with the default clock phase, no
  per-case input): `build/cost_model_v2/summary.md` after a run.
- **Monotonicity.** `scan-feasible` over 2160 problems (M, N in 4..32, K in
  {1, 9, 16, 33, 64}, three K_MAX pairs, serialized and pipelined, with and
  without the shared-read bound), every deadline 0..2T*.

## 6. Outside V2

- The host's traffic into the FPGA over DPTI (operand slab MEM_WRITEs at 5
  array cycles per byte, descriptors of 84 bytes) and the host's own pace: V2
  starts when the jobs are offered. In co-simulation that link dominates
  end-to-end time (a 1 KiB slab is ~5000 cycles); its model is future work
  and UNCALIBRATED for the board in any case.
- The board's MIG / DDR3 (refresh, row misses, latency): the RTL simulation's
  memory answers one beat per cycle with no latency, and so does V2.
- The hardware scheduler's start_cycle hold: a fold waits until the
  scheduler's busy-gated `cycle_counter` reaches the job's `start_cycle`.
  V2 does not model it; the bench offers every job with `start_cycle` = 0.
  The 2026-10-19 overlap audit found no hold on the v1 path or over DPTI,
  and holds on a v2-path context fed serialized `start_cycle` values --
  NOT VERIFIED for V2's predictions where it applies.
- Pipeline depths other than the RTL's 4 (and 1): V2 runs them, the RTL cannot
  be configured to check them -- NOT VERIFIED.
