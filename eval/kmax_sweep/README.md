# K_MAX sweep framework — analytical phase

`kmax_transport_model.py` evaluates the thesis cost/transport model for every
candidate `K_MAX_8X8` / `K_MAX_4X4` and writes the plan that the Vivado phase
will later fill in.  It runs no synthesis, no implementation and no Vivado
TCL; the only executable work is the model and the CSV generation.

## Model (as implemented in the script, unchanged from the thesis)

```
T_compute(k, N)       = k + 2(N-1) + H
T_DMA(k, N, beta)     = 2*N*k / beta                     (A and B: 2*N*k words; N = lanes moved)
T_invocation          = max(T_compute, T_DMA)            (fill i+1 overlaps compute i)
k*(N, beta)           = (2(N-1) + H) / (2N/beta - 1)     (T_compute == T_DMA)
                        no crossover when 2N/beta <= 1 -> compute-bound at every k

tile with reduction length K on a context with K_MAX:
  n                 = ceil(K / K_MAX)
  k_i               = K_MAX (i < n),  K - (n-1)*K_MAX (last)
  compute_cycles    = sum_i T_compute(k_i) = K + n*(2(N-1)+H)      <- H paid once per invocation
  dma_cycles        = sum_i T_DMA(k_i)     = 2*N*K/beta
  overlapped_cycles = sum_i max(T_compute(k_i), T_DMA(k_i))        <- THE requested quantity
  pipelined_time    = T_DMA(k_1) + sum_{i<n} max(T_compute(k_i), T_DMA(k_{i+1})) + T_compute(k_n)
                      <- supplementary: the pipelined schedule with the first
                         fill and the last compute exposed; reported beside
                         overlapped_cycles, never substituted for it
```

`H = 95` cycles per invocation (thesis, measured; H_4 = H_8).  The per-tile
write-back (N*N words) is not in the model, as in the thesis: in the RTL it
runs in the write-back stage under the next fold and is exposed once per job,
independent of K_MAX.

## Payload lanes — the current RTL vs the thesis N=4 formula

The current RTL instantiates every context with `N_WIRE = N = 8`
(`systolic_dma_top.sv`, both `ACC_8X8` and `ACC_4X4`); a context moves
`job_k * N_WIRE * 8` bytes (`systolic_accel_context.sv`, `job_rx_bytes`) and a
4x4 context keeps lanes 0..3 at its buffers (`lane_in_array`).  So a 4x4
invocation moves **2*8*k** words today, not 2*4*k.  Simulation agrees:
`words_written = 256` for a k=16 job on devices 1..3.

| `--payload`      | N=8 lanes | N=4 lanes | meaning |
| ---------------- | --------- | --------- | ------- |
| `rtl` (default)  | 8 | 8 | the RTL as implemented; N=4 k* uses the moved width: (2(N-1)+H) / (2*8/beta - 1) |
| `thesis-4lane`   | 8 | 4 | the thesis formula with N=4 — a REFERENCE / sensitivity mode for a 4-lane payload format the RTL does not have |

Every row of every output carries `payload_model` and `payload_lanes`.
`--as-implemented` is an alias of the default.

## beta regimes (words per array cycle)

| beta  | basis | meaning |
| ----- | ----- | ------- |
| 0.99  | declared | effective fill of the v1 operand path: one 32-bit write port per context |
| 3.51  | **measured, N=8 DMA path** | DDR3 supply |
| 3.627 | **measured, N=8 DMA path** | DDR3 supply, 90.7 % of the 4.00 ceiling |
| 11.20 | demand, sensitivity | fold-average demand 2sK/(K+2(s-1)+H) at s=8, K=256 |

3.51 and 3.627 were measured for the N=8 path only.  For N=4 they are
declared aggregate transport parameters used for sensitivity analysis; there
is no N=4 bandwidth measurement.  Every row carries `beta_basis`
(`measured_N8_path`, `declared_for_N4_(N8_measurement_reused)`,
`declared_v1_fill_limit`, `demand_sensitivity_case`, `user_declared`).

## Fleet (1 x 8x8 + 3 x 4x4) — an analytical contention assumption

`fleet.csv` evaluates

```
beta_ctx = min(writer_cap, beta_aggregate / n_active)
```

This is an **analytical contention assumption, not a measured per-context
beta**.  The equal split follows from `dma_engine_multi`'s round-robin
single-beat bursts under sharing and matches simulation with an ideal
1-beat/cycle channel (1, 2 and 4 concurrent fills each took 260 cycles for
256 words).  `writer_cap` is the declared v1 fill limit (0.99) or the
beat-wide v2 limit (4.0).  `beta_aggregate` reuses the N=8 single-stream
measurements; the DDR3 controller's efficiency under single-beat bursts has
not been measured.  Every row carries `basis`.

## Memory cost columns

- `operand_words = 4*N*K_MAX` (A and B, ping-pong, N banks) — exact for the RTL.
- `operand_bram18k_est = 4*N*ceil(K_MAX/512)` — **estimate only**, v1 buffers
  (`USE_V2=0`): each bank is a 32-bit 1W1R `ram_style="block"` memory, one
  18 Kb block (512 x 36) up to 512 deep.  Flat up to 512.  Operand buffers
  only; the v2 CYCLIC layout (4 sub-banks per A bank) and the rest of the
  fleet are not covered.  Vivado's BRAM column replaces this.

Legal K_MAX (from the RTL): powers of two >= 16.  The context refuses a
non-power-of-two `RX_BYTES`; `dma_operand_writer` needs `WIN_W = K_W - 3 >= 1`
(K_MAX = 8 is illegal); 512 is legal (one 18 Kb block per bank); 1024 and
above are legal by width and bounded only by BRAM.

## Precision

All quantities are computed at full precision and rounded once, when a CSV
cell (2 decimals) or a console line (1 decimal) is written.  The Pareto
screen compares full-precision values.

## Run

```
cd ~/systolic-mlir
python3 eval/kmax_sweep/kmax_transport_model.py --out eval/kmax_sweep/out
python3 eval/kmax_sweep/kmax_transport_model.py --out eval/kmax_sweep/out --bram-cliff   # + 1024:1024
python3 eval/kmax_sweep/kmax_transport_model.py --out /tmp/ref --payload thesis-4lane     # reference mode
python3 eval/kmax_sweep/kmax_transport_model.py --help
```

Defaults: `--payload rtl --H 95 --betas 0.99,3.51,3.627,11.20
--kmax8 16,32,64,128,256,512 --kmax4 16,32,64,128,256,512 --K 16,64,256,1024
--K-rep 256 --cost words --pareto-metric overlapped
--pairs 16:16,32:16,32:32,128:128,256:256,512:512`.

## Candidate sweep plan

Initial pairs (K8:K4): `16:16, 32:16, 32:32, 128:128, 256:256, 512:512`.
`1024:1024` is an optional BRAM-cliff experiment (`--bram-cliff`), not a
default candidate.  The plan's analytical columns are evaluated at
`K_rep = 256`.

## Outputs (`--out`)

| file | one row per | contents |
| ---- | ----------- | -------- |
| `analysis.csv`   | geometry x beta x K_MAX x K | payload_model, payload_lanes, beta_basis, k_star, invocation_count, compute_cycles, dma_cycles, overlapped_cycles, pipelined_time, bottleneck, operand_words, operand_bram18k_est |
| `pareto.csv`     | geometry x beta x K_MAX     | overlapped_cycles at every representative K (default metric), memory, `pareto_candidate`, plus the metric/cost used |
| `fleet.csv`      | writer x beta x n_active     | beta_ctx, k*_8x8, k*_4x4, limited_by, basis |
| `sweep_plan.csv` | (K8,K4) x beta x geometry   | the sweep schema below; Vivado columns empty, `status = analysis_only` |

## Sweep CSV schema (`sweep_plan.csv`, to be completed by the Vivado phase)

```
K8,K4,beta,geometry,k_star,BRAM,LUT,FF,DSP,WNS,TNS,WHS,THS,
invocation_count,compute_cycles,dma_cycles,overlapped_cycles,status,
payload_model,payload_lanes,beta_basis
```

`BRAM LUT FF DSP` come from the post-route utilization report of the fleet
built with `-GK_MAX_8X8=K8 -GK_MAX_4X4=K4`; `WNS TNS WHS THS` from the timing
summary; `status` becomes `timing_met` / `timing_failed` / `build_failed`.
The last three columns record the assumptions each row was evaluated under.
