# ISSUE-008 — the generalized affine GEMM mapping: affine operand views, C purity dropped, SQ-5

| field | value |
|---|---|
| Status | **RECOGNITION IMPLEMENTED** (2026-10-16, Phase C, on checkpoint `8cb8b59`) — a specification change decided by the user after the Phase A analysis; **OPEN**: the lowering of skewed and merged views (Phase D; `repro_skewed_views.mlir` is the XFAIL reproducer) |
| Decisions (the user, 2026-10-16) | G1 + G2 as the specification and decision framework, never as a sufficient condition alone (the reduction correspondence, the integer domain, output uniqueness, index legality and the lowering's correctness are proven separately); general affine operand views (skew, merged dimensions, transposes, batch-dependent affine offsets); C's legality redefined (C may read batch loops in any dimension; what matters is the output map's correctness and its injectivity on the integer domain per batch); Test 3 (SQ-5) analysed instead of presumed refused; non-box domains only with a proven correspondence to a (batch, I, J, K) box |
| Specification | `docs/AFFINE_GEMM_MAPPING.md` section 19 (definition 19.1, readers and demotion 19.2, the legality of C 19.3, the integer lattice 19.4, the screening 19.5, the three tests 19.6, non-box domains 19.7, SQ-5 19.8) |
| Before | the analysis by groups required every subscript to be role-pure: C reading every batch loop alone ("C purity": `C[i + k][j]` refused, PROVEN_INVALID), no dimension combining two roles (`A[i][i + k]`: "reads one logical coordinate in two of its dimensions"), no group read by all three operands without a batch loop ("a subscript couples loops of different coordinates"); a coupled positional reading was restored when the analysis by groups refused (`C[i + j][j]` with `A[i + j][k]`: "loop 1 drives I and J"); a batched GEMM with two unit extents was refused pending SQ-5 |
| After | the analysis by groups first; when it refuses only for one of those three role-purity reasons (`relaxableRefusal`), the **readers analysis** (`byReaders`): every loop's role by its readers (A and C: I, B and C: J, A and B: K, all three: batch, one: a broadcast), each role's direction the common direction of the subscripts restricted to its loops (`lattice::commonDirection`), every role a mixed radix of its loops (`lattice::mixedRadixExtent`), every dimension an affine view of its operand's two coordinates plus batch terms (`OperandLayout::roleCoefficients`, `dimConstants`; verified on the domain, `dimensionsAreViews`), C injective on the (I, J) box (`lattice::injectiveOnBox`, G2). A role-pure layout lowers as before; a **general** layout (`OperandLayout::general`) is refused at the lowering, VALID_BUT_LOWERING_UNSUPPORTED, until Phase D. The SQ-5 gate is removed. A coupled positional reading is never restored. A non-dense role is VALID_BUT_LOWERING_UNSUPPORTED (a batched GEMM by demotion); a non-box domain UNKNOWN |
| Components | `IntegerLattice.{h,cpp}`, `TestIntegerLattice.cpp` (Phase B, `8cb8b59`); `AffineMatmulAnalysis.{h,cpp}` (`byReaders`, `groupsOrReaders`, `relaxableRefusal`, `dimensionsAreViews`, the layouts' role coefficients, the screening of Phase B); `AffineMatmulPartitioning.cpp` (SQ-5 gate removed, labels); `AffineMatmulToSystolic.cpp` (the general-view refusal); `TestAffineMatmulDiscovery.cpp` (prints general layouts); `generator/casespec.py` (`generalized_gemm_on_domain`, `repeat_in_reduction`, bases `generalized_views` / `sq5_decided` / `demotion`, `legal_gemm`, `Case.redecided`; the predictor's `predict_by_readers`, `groups_or_readers`); `generator/gen_cases.py` (mutation notes, the ISSUE-008 tag); `generator/showcase_cases.py`; `test/Systolic/Inputs/affine_gemm_mapping_oracle.py` (decision by readers, no C purity, SQ-5, `skew` / `merge_c` mutations) |
| Snapshot before | commit `8cb8b59`; `~/work/integration-backups-20261016/pre-geometric-refactor/` (protected-file edit log `protected_edits.txt`) |

## The decision, per nest (docs section 19)

1. Every loop of two or more iterations is classified by its readers. A loop read by one
   operand is a broadcast: the canonical decomposition fails (a batched GEMM by demotion, not
   constructed). No role at all -- every loop read by all three operands -- is an element-wise
   product: **not a GEMM** in any decomposition (non-degeneracy, section 19.1).
2. Each role's subscripts, restricted to the role's loops, have one direction, else separate
   coordinates (demotion) or SQ-1 (UNDETERMINED).
3. Each role is a mixed radix of its loops (Theorem 3, S2), else demotion.
4. C is injective on the (I, J) box per batch (G2, exact criterion of section 19.3), else
   demotion (the J or I loops as batch loops).
5. Then every subscript is an affine view of its operand's two coordinates plus batch terms
   (G1), and the nest is a GEMM -- or a batched GEMM, the batches in program order, their
   output windows possibly overlapping (section 19.3).

## Expectations that changed (each argued from the mathematics; none to make a test pass)

| case | before | after | why (docs section 19) |
|---|---|---|---|
| case_087 `C[i+j][j] += A[i+j][k] B[k][j]` | REJECT (refused: loop 1 drives I and J) | ACCEPT, offloaded (8 batches of 8 x 1 x 8) | j is a batch loop; per j a GEMV, C injective per batch |
| case_127 `C[i+k][j] += A[i][k] B[k][j]` | REJECT (C purity) | ACCEPT, offloaded (8 batches of 8 x 8 x 1) | k is a batch loop; per k a rank-1 update; overlapping windows add up in order |
| case_088 `C[2i+j][j] += A[2i+3][k+1] B[k+2][3j+2]` | REJECT | ACCEPT; **FALSE_NEGATIVE** (lowering) | C's view (2I + J, J): affine, injective |
| case_126 `C[i][j] += A[i][i+k] B[k][j]` | REJECT | ACCEPT; **FALSE_NEGATIVE** (lowering) | A's view (I, I + K): affine |
| case_090, case_095, case_123, case_124, case_125 | REJECT | OUT_OF_SCOPE (`demotion`), DOCUMENTED_LIMITATION | a broadcast loop: a batched GEMM once it is demoted to a batch loop, not constructed |
| case_089 (`affine_gemm_singular_mapping.mlir` in C) | REJECT (refused by the dense partition) | REJECT (refused at recognition: element-wise) | every loop read by all three operands |
| SQ-5 (`C[i][k] += A[i][k] B[k][j]`, j once; Test 3) | UNDETERMINED (`expected_refusal`) | ACCEPT (`sq5_decided`), offloaded | a batched GEMM with two unit extents |
| generated `coupled_C_row` (`C[i + j][j]`) | REJECT | ACCEPT; FALSE_NEGATIVE (lowering) with one J loop | a skewed C view |
| generated `coupled_A_and_C_row` (`A[i + j][k]`, `C[i + j][j]`) | REJECT | ACCEPT, offloaded with one J loop | case_087's shape |
| generated `singular` (`A[i + k][k]`) | REJECT | ACCEPT; FALSE_NEGATIVE (lowering) with one K loop | a skewed A view |
| generated `output_in_reduction` (`C[i][k]`) | REJECT | OUT_OF_SCOPE (`demotion`); ACCEPT when j runs once (SQ-5) | case_095's shape |

Lit tests whose expectation changed (each with a RE-DECIDED comment in the test):
`affine_gemm_batch_views.mlir` (@c_mixes_batch, @c_skewed lowered; @coupled_a recognized, refused
at the lowering), `affine_gemm_batched_lowering.mlir` (@batch_coupled, @batch_scalar_rows
lowered), `affine_gemm_mapping.mlir` (@coupled lowered, 8 x 1 x 8 batched),
`affine_gemm_composed.mlir`, `affine_gemm_partition_coupled.mlir` (batched successes),
`affine_gemm_singular_mapping.mlir` (refused at recognition: element-wise),
`affine_to_systolic_refused.mlir` (@coupled replaced by @c_not_injective `C[i + j]`, @batched
changed to `C[b][i + j]`, both checked through the discovery pass), `affine_gemm_mapping_oracle.mlir`
(a new summary line), `ISSUE-007/repro_batch_views.mlir` (case_127's shape lowered).

## New tests

`test/Systolic/integer_lattice.mlir`, `integer_lattice_random.mlir`, `affine_gemm_geometry.mlir`,
`affine_gemm_geometry_random.mlir` (Phase B); `test/Systolic/affine_gemm_generalized.mlir` (Tests
1--3, Test 2 reparametrized over D*, skewed C, flat, Hankel, flat not injective, SQ-1, separate
coordinates); showcase ext_026..ext_028 (the three tests as C programs, executed against their CPU
references); this directory's `repro_skewed_views.mlir` (XFAIL until Phase D).

## Open

- **Lowering of general views** (Phase D): a `memref.reinterpret_cast` per operand with offset
  sum_d rowMajor_d alpha_d (dynamic for batch terms) and strides sum_d rowMajor_d rho_{d,r};
  until then case_088, case_126, ext_026 and the generated skewed cases are FALSE_NEGATIVEs of
  this issue.
- **Demotion** (not planned as part of this issue's recognition): constructing the batched
  decomposition when the canonical one fails (case_090, case_095, case_123--125; SQ-1).
- **Non-box domains**: the lattice parametrization of section 19.7 (a parallelogram, D* of
  Test 2) is not constructed (refused as UNKNOWN).

## Verification (2026-10-16, Phase C; LLVM 24 only)

Logs: `~/work/integration-backups-20261016/phaseC-validation/`; the details are in
`docs/AFFINE_GEMM_MAPPING.md` section 19.9.

- lit, release and debug builds: 108 tests, 107 passed, 1 expectedly failed (this directory's
  `repro_skewed_views.mlir`).
- The oracle (debug build): 17,200 nests over 43 seeds, 0 mismatches.
- The showcase (soft runtime): 165 pass, 3 known false negatives of this issue (case_088,
  case_126, ext_026), 7 expected refusals (SQ-1, SQ-2, SQ-3); case_087, case_127 and ext_028
  offloaded and equal to their CPU references.
- The regression gate: case_088 and case_126 pass → known_false_negative against the baseline of
  2026-10-15 (re-decided ACCEPT; the baseline is not re-saved), ext_026 new; this issue's
  reproducer fails as expected (XFAIL); the other reproducers unchanged.
- 800 generated cases: 792 pass, 8 known false negatives of this issue (skewed or merged views);
  21 verdicts changed against the previous run, all from this issue (8 skewed views now ACCEPT,
  7 SQ-5 cases now offloaded, 6 batched GEMMs with unit extents no longer refused).
- The FPGA board: NOT RUN.
