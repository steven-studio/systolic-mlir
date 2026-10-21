# eval/cost_model_v2: Cost Model V2 against the RTL

The validation of the fleet simulator (`docs/COSTMODEL_V2.md`). Everything here
is RTL **simulation** (Verilator) of `rtl/multi_200t/tile_pipelined`, the
bitstream's RTL, unchanged. Nothing is a board measurement.

| file | what |
|---|---|
| `tb_fleet_jobs.sv` | the bench: the real `systolic_dma_top` (1 x 8x8 + 3 x 4x4, the MIG model, IEEE fp), jobs from a file at run time, every result checked exactly, every event stamped through hierarchical references (it drives only the top's ports) |
| `build_bench.sh` | one Verilator build per (K_MAX_8X8, K_MAX_4X4, v1/v2) |
| `run_validation.py` | the whole validation (below) |

Job file: one line per job, `dev k present dep gap` (see the bench header).
`systolic-cost fleet-sim --jobs FILE` runs the same file on the model.

## Running

```bash
# build/bin/systolic-cost built; Verilator >= 5 on PATH
python3 eval/cost_model_v2/run_validation.py            # every step
python3 eval/cost_model_v2/run_validation.py exact report   # some steps (after build cases rtl)
```

Steps (all under `--out`, default `build/cost_model_v2`):

| step | checks |
|---|---|
| `build` | 7 bench builds (5 K_MAX pairs v1, 16/16 and 32/32 v2) |
| `cases` | the case matrix: 1298 job lists (K across 1, 7, 8, 9, 15, 16, 17, K_MAX +- 1, 2 K_MAX +- 1; one 8x8, one 4x4, 8x8+4x4, 8x8+3x4x4, 3x4x4; gaps 0/3/40/300; streams; one-outstanding chains; compiler schedules; random) |
| `rtl` | every case on the bench (v1; 16/16 and 32/32 also v2): 1782 runs, all results exact |
| `exact` | the model against every run: every event of every job, the AXI read request count, the readback completions, identical |
| `skip` | the model's idle skip changes nothing (both port policies, both operand paths) |
| `credit` | 70 compiler schedules (`systolic-cost schedule`, V2) in the runtime's offer order replayed on the bench, identical |
| `report` | V1 and V2 device-makespan errors against the RTL, V2 with the default clock phase: `summary.md` |
| `scan` | the search's predicate at every deadline 0..2T* on 2160 problems (V2): no counterexample to monotonicity |

The `exact` step feeds the model each bench run's measured phase between the
array clock and the DPTI clock (a physical input the board leaves free);
`report` uses the default phase, so its numbers are pure predictions.
