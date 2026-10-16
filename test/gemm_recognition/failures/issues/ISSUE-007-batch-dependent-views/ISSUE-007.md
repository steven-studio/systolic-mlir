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
