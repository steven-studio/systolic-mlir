# ISSUE-007 — batch-dependent operand views: a factor's view may move with the batch

| field | value |
|---|---|
| Status | **IMPLEMENTED** (2026-10-16, on checkpoint `b13f9a3`) — a specification change: batched GEMMs whose factors read the batch coordinate(s) through their own expressions, or as affine offsets of a role's dimension, move from REJECT / VALID_BUT_LOWERING_UNSUPPORTED to inside the specification |
| Target case | `C[8*i, j] += A[4*i-j, 5*k] * B[5*k, j]` (the request's Phase 1): `j` is read by all three operands -- a batch loop -- and moves A's row view |
| Before | the analysis by groups joined `i` and `j` through A's row (`4i - j`), so C read the merged group in two dimensions: refused ("C reads one logical coordinate in two of its dimensions (0 and 1) ... (Theorem 3, S1)", PROVEN_INVALID). A batch coordinate read through different expressions (`A[b + 1]` with `C[b]`) was refused as VALID_BUT_LOWERING_UNSUPPORTED. The executable specification said REJECT ("A has a subscript that couples loops of different coordinates (mixed:I+J, K)") |
| Specification | `docs/AFFINE_GEMM_MAPPING.md` section 16 (the readers table, the acceptance rule, Theorem 5, counterexamples, the target worked out) |
| Components | `AffineMatmulAnalysis.cpp` (readers first, batch loops out of the grouping, C purity, role groups read by all three refused, C may read a batch coordinate in several dimensions, roles matched without batch terms, `OperandLayout::batchCoefficients`); `AffineMatmulAnalysis.h`; `AffineMatmulPartitioning.{h,cpp}` (`rangeOnDomain`); `AffineMatmulToSystolic.cpp` (one `affine.apply` per dimension with batch terms, dynamic subview offsets, index ranges widened by the batch terms); `TestAffineMatmulDiscovery.cpp` (prints the batch terms); `generator/casespec.py` (`batched_gemm_on_domain`, basis `batched_views`, the predictor's analysis by groups); `test/Systolic/Inputs/affine_gemm_mapping_oracle.py` (per-batch decision, address-exact check of batched views, two mutations) |
| Cases whose expectation changed | `depth.c` ng_skew (`C[i][j] += A[i + j][k] * B[k][2j]`): stays on the host → offloaded, 16 invocations per call, equal to the reference. `affine_gemm_batched_lowering.mlir` @batch_views_differ and the former `affine_to_systolic_refused.mlir` @batched (`A[b + 1]`, `B[b]`, `C[b]`): refused → lowered (the refused test now holds `C[b][i + b][j]`, which stays refused). showcase case_090, case_127: still REJECT, still refused; only their headers' predicted reasons follow the new compiler messages |
| New tests | `test/Systolic/affine_gemm_batch_views.mlir` (recognition, partition, lowering, LLVM; 7 accepted and 7 must-reject shapes), `test/SystolicCC/affine_batch_views.c` (7 kernels executed and compared bitwise with the CPU reference), showcase ext_015..ext_018, this directory's `repro_batch_views.mlir` |
| Snapshot before | commit `b13f9a3`; `~/work/integration-backups-20261016/pre-geometric-refactor/` (patch, untracked files, modes, checksums) |

## The rule

A loop of two or more iterations is classified by the operands that read it: A and C → I, B and
C → J, A and B → K, all three → a batch loop; one operand → refused (a broadcast). C must read
every batch loop alone (a dimension of C holds a role or batch loops, never both); the factors
may add batch terms to any dimension. For each value of the batch loops the role loops must be
a dense GEMM of strided views (Theorem 3), the same box for every batch value, and every index --
role range plus batch range -- inside its array. Then the lowering re-emits the batch loops and
offsets each operand's view per batch iteration; Theorem 5 shows it performs exactly the updates
of the nest.

## Must stay rejected (checked)

`C[i + j][j] += A[i + j][k] B[k][j]` (case_087: refused by the dense partition, loop j drives I
and J), `C[i + k][j]` (case_127: C mixes the batch loop k), `A[i][i + k]` (coupled),
`B[k][i]` with `C[i][j]` (j read by C only), `A[i][k0] B[k1][j]` (two one-reader loops),
`A[i][k0 + k1]` (non-injective K), every coordinate a batch one (element-wise), and the target
with `i` from 0 (A's row reaches −3: out of bounds).

## Verification

See `docs/AFFINE_GEMM_MAPPING.md` section 16 and the final report;
`~/work/integration-backups-20261016/phase1-validation/` holds the logs.

## Review fixes (2026-10-16, after commit `39b4d6d`)

An independent review found three defects in this issue's area; the strengthened oracle found a
fourth (`docs/AFFINE_GEMM_MAPPING.md` section 18):

| # | defect | kind | fix | tests |
|---|---|---|---|---|
| 1 | batch loops re-emitted with a bound map's first result (`j = 0 to min(4, 3)` ran j = 3, outside B and C) — since ISSUE-006 | wrong code | origin, step and trip count from the analysis's normalization, checked; a single-constant upper bound kept as written | `affine_gemm_batch_views.mlir` @batch_min_bound, @batch_max_bound, @positional_batch_min_bound, @batch_stepped_split; oracle (redundant max/min bounds, batch loops compared by iterations) |
| 2 | a positional reading in which a loop drives two of I, J, K kept and refused (`A[i][k + j] * B[k + j][j]`, PROVEN_INVALID) | false refusal | the analysis by groups decides whenever a loop drives two of {P*, I, J, K} (such a loop is read by all three operands); the positional reading kept only when the groups refuse too (case_087 unchanged); `casespec.py` predictor mirrored | @j_in_k, @i_in_k; executed `bv_j_in_k`, `bv_i_in_k`; showcase ext_023, ext_024; oracle mutation `role_in_k` |
| 3 | a positional prefix constant on the domain counted as a batch row: a plain GEMM refused pending SQ-5 — since ISSUE-006 | false refusal | only prefix subscripts that vary on the domain are rows of P*; a constant one is a fixed index | @constant_prefix; executed `bv_const_prefix`; showcase ext_025 |
| 4 | a stepped loop with a max lower bound: MLIR's domain drops the stride, the regions do not cover it | false refusal | the stride from the folded lower bound in the source domain | `affine_gemm_p0_views.mlir` @minmax_bounds_split_k, @batch_stepped_split; oracle |

Case_087 and case_127 stay refused with their diagnostics. No expectation of an existing lit or
showcase test changed (the new oracle summary fields are additions). Of the 800 generated cases
every verdict is unchanged; one predicted refusal reason changed together with the compiler's:
`gen_de03cb2b804b` (`C[i + j][3 + 3j]`, `i` of one iteration; UNDETERMINED, SQ-5) is refused with
the SQ-5 reason instead of "loop 4 drives I and J" (fix 2: the groups find the batch loop `j`
and the per-batch 1 x 1 x 8 product SQ-5 leaves open); `expected_refusal` either way.
