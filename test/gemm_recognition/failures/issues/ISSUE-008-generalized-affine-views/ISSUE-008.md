# ISSUE-008 — the generalized affine GEMM mapping: affine operand views, C purity dropped, SQ-5

| field | value |
|---|---|
| Status | **IMPLEMENTED** — recognition 2026-10-16 (Phase C, on checkpoint `8cb8b59`; a specification change decided by the user after the Phase A analysis), the lowering of skewed and merged views 2026-10-18 (Phase D, `docs/AFFINE_GEMM_MAPPING.md` section 19.11: one `memref.reinterpret_cast` per general view; `repro_skewed_views.mlir`, the former XFAIL reproducer, is a permanent regression test) |
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

## Phase D (2026-10-18): the lowering of general views

Every general layout is lowered as one `memref.reinterpret_cast` of the operand to rows x
columns: element offset sum_d w_d (c_d + sum_slot lambda_{d,slot} o_slot) (plus the batch terms
sum_d w_d beta_{d,s} x_s through one affine.apply), element strides sum_d w_d lambda_{d,slot}
s_slot, sizes the roles' extents (docs section 19.11, with the correctness argument: G1 is the
identity, G3 per dimension in checked arithmetic, G2 on the index tuple implies injective
addresses, a read view may repeat elements, overlapping C windows across batch values add up in
program order). The role-pure views keep their subview construction; the same explicit view also
replaces a rank-reducing subview whose rank-reduced type would be ambiguous (finding 7).
case_088, case_126 and ext_026 are TRUE_POSITIVE since; the generated skewed cases pass.
Tests: `test/Systolic/affine_gemm_generalized.mlir` (the emitted views), `test/SystolicCC/
affine_skewed_views.c` (eighteen kernels executed and compared bitwise with CPU references,
each offloaded call verified against the function as written), the oracle with
`GENERAL_VIEWS_LOWERED = True` (address-exact, repeated factor elements included), the
executable specification's predictor (per-dimension bounds of a general operand).

Review fixes of Phase D (2026-10-18, before the commit; docs section 19.11 "Review fixes"): the
adversarial review (four lenses, each finding re-examined by a skeptic) reported 14 findings, 13
confirmed, 1 refuted. Fixed: the general view's bounds test formed one per-role sum in plain
int64 (a wrapped sum let an out-of-bounds skew pass: `A[i][i + 2^61 k]` lowered with an offset of
2^62, or a negative reinterpret_cast offset) -- every term checked now, an unrepresentable offset
refused (`test/Systolic/affine_gemm_review_d.mlir`); one array as both factors: predictor,
specification and compiler now decide "one tensor serves both" the same way -- the same element
view in some orientation of the three coordinates (the lowering tries the eight, the predictor
the same eight analytically, the specification by enumeration) -- so `A[i][i + k] * A[k][k + j]`
and `A[i][i + 3 - k] * A[3 - k][3 - k + j]` are lowered through one view and `A . A^T`,
`A[i][i + k] * A[k][j]` are OUT_OF_SCOPE `same_memref_views`
(`test/Systolic/affine_gemm_shared_factor.mlir`); a nest of loops that run once with constant
subscripts is the 1 x 1 x 1 GEMM (lowered; UNKNOWN before), a nest whose every loop is a
repetition loop is the documented limitation `no_gemm_loop`; the readers predictor's count-down
coordinates (the step folded twice); the stale "not implemented yet" notes in showcase_cases.py,
gen_cases.py and the README.

Verification of Phase D (final run after the review fixes; logs in
`~/work/integration-backups-20261016/phaseD-validation/`): lit 111/111 on both builds; oracle
17,200 nests, 0 mismatches (785 general views lowered and checked); showcase 168 pass, 0 known
false negatives, 7 expected refusals; the regression gate clean, every reproducer passing,
check-systolic 111/111; generated cases: 800 of 800 pass; predictor = compiler on 1,999 random nests and 30
hand-written review nests; the end-to-end test's 18 kernels bitwise equal to their CPU references
(17 offloaded calls verified); permissions 48/48; adversarial review: 14 findings, 13 confirmed
and fixed, 1 refuted. FPGA board: NOT RUN.

## Open

- **Demotion** (not planned as part of this issue's recognition): constructing the batched
  decomposition when the canonical one fails (case_090, case_095, case_123--125; SQ-1).
- **Non-box domains**: the lattice parametrization of section 19.7 (a parallelogram, D* of
  Test 2) is not constructed (refused as UNKNOWN).
- **One array as both factors through two different views** in every orientation of the
  coordinates (`A . A^T`, `A[i][i + k] * A[k][j]`): recognized, refused by the lowering (two
  restrict tensors of one buffer; VALID_BUT_LOWERING_UNSUPPORTED), OUT_OF_SCOPE
  `same_memref_views` in the specification (docs section 7). The same view in some orientation is
  one tensor and is lowered (2026-10-18).
- **A nest whose every loop is a repetition loop** of two or more iterations around one scalar
  multiply-accumulate: no GEMM loop to anchor on, UNKNOWN; OUT_OF_SCOPE `no_gemm_loop` (the
  all-once nest is the 1 x 1 x 1 GEMM and is lowered since 2026-10-18).

## Review fixes (2026-10-18, before the Phase C commit)

The four-lens adversarial review of Phase C (C++ correctness, lowering safety of the newly
lowered shapes, predictor fidelity, specification soundness; each finding re-examined by a
skeptic) reported 27 findings, 25 confirmed, 2 refuted ("one tile per batch value" is a
wording; SQ-1 in the generalized branch -- fixed anyway, see below). Every confirmed finding is
fixed here; `docs/AFFINE_GEMM_MAPPING.md` sections 19.2, 19.7-19.10 and README sections 1-2
record the decisions.

| # | finding | kind | fix | tests |
|---|---|---|---|---|
| 1 | a batched product with M = N = K = 1 lowered through the positional form (the SQ-5 gate was the only refusal) | wrong decision (FALSE_POSITIVE by the project's classification; the values were right) | `getDenseLogicalGemm` refuses `numP > 0` with extents (1, 1, 1) as PROVEN_INVALID; casespec predictor and spec mirror it | `affine_gemm_generalized.mlir` @elementwise_positional, @elementwise_positional_once |
| 2 | readers refusals on non-box or symbolic-bound domains labelled VALID_BUT_LOWERING_UNSUPPORTED / PROVEN_INVALID | wrong class | every groups/readers verdict is UNKNOWN when a bound is not a constant (`refuseRoles`, `unsupportedRoles`) | @test1_triangle, @test1_symbolic, @parallelotope_rank1 |
| 3 | the general-view refusal ran before the bounds and alias checks | wrong class | Phase D: the general views go through the same per-dimension bounds and alias checks as every view (an out-of-bounds skewed nest is PROVEN_INVALID, a possibly aliasing one UNKNOWN) | `affine_gemm_generalized.mlir` @test1_oob, @test1_alias |
| 4 | an empty role loop in the readers path: a false reason, VALID_BUT_LOWERING_UNSUPPORTED | wrong class | the readers analysis skips the radix and G2 checks on an empty domain; the dense partition reports the empty GEMM (SQ-2) | @test1_empty |
| 5 | a repetition loop labelled PROVEN_INVALID | wrong class | UNKNOWN (the mapping is not analysed) | @repetition_inside_reduction |
| 6 | `hoistRepetitionLoops` needed a band of four with three dimensions | false refusal (SQ-4/SQ-5 nests) | a band of two with one dimension suffices | @matvec_repetition_between |
| 7 | subview rank reduction ambiguous when every kept dimension has size 1 (invalid IR, systolic-cc fails) | wrong code | Phase D: such a view is built as an explicit `memref.reinterpret_cast` (offset and strides stated, nothing to guess) | `affine_gemm_generalized.mlir` @unit_kept_dimension |
| 8 | predictor and spec: only reduction loops block the hoist | spec/predictor mismatch | the C-collision rule by enumeration (`_hoist_blocked`, `repeat_in_reduction`); the compiler's own test made integer-exact (`hoistReordersDependentIterations`: MLIR's dependence polyhedra tested with `isIntegerEmpty`, where `isValidLoopInterchangePermutation` was rational only and refused legal hoists such as a step-2 loop against a stride-3 placement) | review driver r01, r02, r11; the random fuzz (2,000 nests); the gate |
| 9 | predictor: swapped positional factors checked against the other arrays | predictor | `ka`/`kb` carried into the bounds and alias checks | r18, r19 |
| 10 | predictor: factor assignment by the author's arrays, compiler by transposition cost | predictor | the cost mirrored in `predict_by_groups` | r06, r07, r16 |
| 11 | predictor: zero-trip loops dropped from the support | predictor | `support` folds single-iteration loops only | r04, r14 |
| 12 | predictor: radix checked before all directions; G2 collision in the counter convention | predictor | two passes; the front end's ordinal convention; `_injective_on_box` | r05, r17 |
| 13 | spec: the SQ-5 shortcut accepted M = N = 1 (element-wise) and skipped the guards | unsound ACCEPT | removed; the batched path decides with the guards | gate (`random:1:51667`) |
| 14 | spec: broadcast-only nests "valid by demotion" with a false basis text; docs 19.2 "iff two readers" false | docs/spec | `generalized_gemm_on_domain` searches every decomposition of 19.1 (stride-0 roles included) and exhibits one; basis `broadcast`; docs 19.2 corrected ("not read by all three") | showcase case_090/095/123-125 |
| 15 | spec: a skewed GEMM with `-=`, alpha or init_inside was REJECT | wrong label | decided like the MAC forms, then OUT_OF_SCOPE alpha / init_inside | review nonmac.py |
| 16 | spec: SQ-1 in the generalized branch was OUT_OF_SCOPE demotion | label depended on declared roles | UNDETERMINED SQ-1 whenever two operands split a role differently | case_128 |
| 17 | docs: section 4 stale (C[b][b+i] PROVEN_INVALID; distinct batches touch distinct C elements), 19.2 SQ-1 label, 19.8 wording, 19.9 triangle row | docs | corrected | -- |
| 18 | ext_026/ext_028 math class `not_gemm`; ext_028 claimed a fixed false negative | showcase metadata | explicit classes; `redecided` record | showcase |

## Verification (Phase C, final run 2026-10-18; LLVM 24 only)

Logs: `~/work/integration-backups-20261016/phaseC-validation/` (`showcase2`, `oracle2`, `gate3`);
the details are in `docs/AFFINE_GEMM_MAPPING.md` section 19.9.

- lit, release and debug builds: 108 tests, 107 passed, 1 expectedly failed (this directory's
  `repro_skewed_views.mlir`, the open lowering).
- The oracle (debug build): 17,200 nests over 43 seeds, 0 mismatches.
- The showcase (soft runtime): 165 pass, 3 known false negatives of this issue (case_088,
  case_126, ext_026), 7 expected refusals (SQ-1, SQ-2, SQ-3); case_087, case_127 and ext_028
  offloaded and equal to their CPU references.
- The regression gate: case_088 and case_126 pass → known_false_negative against the baseline of
  2026-10-15 (re-decided ACCEPT; the baseline is not re-saved), ext_026 new; this issue's
  reproducer fails as expected (XFAIL); the other reproducers unchanged.
- 800 generated cases: 792 pass, 8 known false negatives of this issue (skewed or merged views),
  no unexpected stage; 21 verdicts changed against the previous run, all from this issue (8 skewed
  views now ACCEPT, 7 SQ-5 cases now offloaded, 6 all-unit batched products now REJECT and
  refused).
- The review's driver (19 cases), 244 formerly flagged and 1,200 fresh random nests: predictor =
  compiler, 0 flagged.
- The FPGA board: NOT RUN.

**2026-10-19 -- the demotion limitation closed in part (docs/AFFINE_GEMM_MAPPING.md section 21,
dimension-intersection partitioning).** The recognizer now constructs the decomposition that demotes
loops to batch loops when the demoted loops are read by C alone in dimensions of their own: a loop
read by C only, and the extra coordinates of a row or column role (`C[b][i][j] += A[b][i][k] B[k][j]`,
case_086; `C[i][j] += A[i][k] B[k][i]`, case_124: j read by C only), each partitioning the GEMM into
independent slabs of C with the factor that does not read it shared. Re-decided ACCEPT (basis
`dimension_partition`): case_086, case_124. Still a documented limitation: a loop read by both
factors and not by C beyond the reduction (basis `partial_reduction`), a loop read by one factor
only (basis `broadcast`), partitions whose outputs overlap or share a merged dimension (basis
`demotion`), and SQ-1.
