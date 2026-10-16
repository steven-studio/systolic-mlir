# test/gemm_recognition — GEMM recognition test system

A fixed showcase set of 100 C programs, an unbounded case generator, and an automated test loop
for the affine GEMM path of this compiler: `systolic-cfront` (C → affine) →
`recognizeLogicalMatmul` → `getDenseLogicalGemm` / exact partitioning → `--affine-matmul-to-systolic`
→ `--lower-systolic-matmul-tile-to-fpga` → `systolic-cc` → the runtime (software model or the
board). Everything here is a test: nothing under `test/gemm_recognition/` modifies the compiler.

```
test/gemm_recognition/
├── README.md                this file: layout, the support specification, how to run, the loop
├── showcase/                the hundred (case_001..100), the boundary cases added after it
│                            (case_101.., section 8), the extended cases (ext_*) and the MLIR-level
│                            cases (mlir_*): positive/, negative/, extended/ (each case in the set
│                            its SPECIFICATION puts it in), manifest.json, CASES.md, INVENTORY.md
│                            (+ inventory.json: per-case purpose, expectation, basis, index forms,
│                            loops, execution evidence, spec questions), README.md
├── generator/               casespec.py (model, oracle, analysis, predictor, spec, C and MLIR
│                            emitters), showcase_cases.py (every case's spec), make_showcase.py,
│                            gen_cases.py (the unbounded generator)
├── scripts/                 stages.py (one program through every stage), verdict.py (judging),
│                            run_showcase.py, loop.py (the closed loop), regression.py (the gate),
│                            run_issue.sh (issue reproducers), inventory.py, test_permissions.py
├── failures/                issues/ISSUE-NNN-*/ (recorded compiler issues: record, reproducers,
│                            lit XFAIL test), <stamp>_<id>/ (new findings of the loop),
│                            known_issue_log.jsonl
├── regression/              corpus/ (programs the loop found failing, re-run forever),
│                            baseline_<runtime>.json, last_run_<runtime>.json
├── generated/               batches of the generator, registry.json (signatures seen),
│                            coverage.json, loop_state.json
└── reports/                 actual results of run_showcase.py (json + md); latest_<mode>.json
```

## 1. What the compiler supports (read from the sources at commit 6e25528, with the fixes of ISSUE-001 and ISSUE-002 noted)

Stage by stage. "Documented" means the sources or the existing tests say so explicitly.

**Front end** (`tools/systolic-cfront`, clang 24 AST): `void` functions; parameters
`float|double A[M][K]` (`restrict` → `llvm.noalias`), scalar float/double; `for (int|long i = LB;
i < UB | i <= UB; ++i | i++ | i += S | i = i + S)` with integer-constant bounds; subscripts that
are affine expressions of the enclosing counters (`+`, `-`, constant `*`); assignments to array
elements (`=`, `+=`, `-=`, `*=`, `/=`); `+ - * /` and unary `-` on float/double. Outside the
subset (an error, never an approximation): local variables, `if`, `while`, calls, pointer
arithmetic, globals, integer arrays, non-affine or symbolic subscripts, symbolic bounds,
`!=` conditions, `return` with a value. Loops that count down (`i > B | i >= B; --i | i-- |
i -= S | i = i - S`, since 2026-10-15, ISSUE-004) are translated to the ordinal loop
`affine.for %i = 0 to N` (N the trip count) with the counter `LB - S * %i` (affine.for only
counts up); a zero step, a step against the condition's direction, and an unsigned counter that
would wrap below 0 are errors. A condition may also compare an affine expression of the counter
with a constant (`2 * i - 3 > 0`, `100 - 3 * i > 0`): it is translated as the equivalent
comparison of the counter (exact over the integers: `i > 1`, `i <= 33`), and refused when the
expression could overflow its C type at any counter value it is evaluated at -- including the
evaluation that ends the loop -- or is unsigned.

**Recognizer** (`recognizeLogicalMatmul`): a perfect chain of `affine.for` ops (from some anchor
down to the innermost) whose innermost body holds exactly three `affine.load`, one
`affine.store`, one `arith.mulf`, one `arith.addf` and nothing else; the stored value is
`addf(load C, mulf(load, load))` (either order of the add); the accumulator is loaded and stored
through the same map and indices; every loop of the chain is used by an access, and no access
uses a loop outside the chain; all index operands are loop IVs (no symbols); the three access
maps have the same number of results r ≥ 2, with a partition prefix of r − 2 results identical in
all three, followed by `A = (I, K)`, `B = (K, J)`, `C = (I, J)` where the two operands that share
a coordinate must address **the same coordinate, each through its own view**
(`matchSharedCoordinate`, since the fix of ISSUE-001): identical expressions, or both
`s · g + o` for one linear form `g` of the loop IVs with integer `s > 0` — a constant offset or a
positive stride on one operand only (`C[i+8][j]` with `A[i][k]`, `C[2i][j]` with `A[i][k]`); then
the coordinate is the primitive form `g` and the operand's view `(s, o)` is recorded
(`LogicalMatmul::viewA/B/C`, the factor maps `phiA/B/C`). Expressions over different loop
combinations (`4 i0 + i1` vs `8 i0 + i1`), of opposite direction (`i` vs `7 - i`) or without a
loop are not one coordinate (refused at recognition). I, J, K pairwise distinct; the
expressions are linear forms (no mod/floordiv). At 6e25528 the rule was expression identity
(ISSUE-001, fixed). Static or symbolic (`0 to %n`) bounds. A repetition loop (used by no
access) must be outside the chain — the lowering pass first moves one that sits between the
GEMM's loops outward when the interchange is legal (not inside a reduction loop).

**Dense partition** (`getDenseLogicalGemm`): no partition prefix (batched is refused with a
message); static bounds (a loop of zero iterations has no normalized extent either and is
refused with its own message, "a loop of the nest has no iteration (an empty GEMM ...)" — spec
question SQ-2; a coordinate no loop drives, a constant subscript, is refused — SQ-4, ISSUE-003);
every loop drives exactly one
of I, J, K with a non-negative coefficient; the loops of one coordinate enumerate it densely
(sorted by scale = coefficient × step, each scale is the previous scale × the previous loop's
trip count). Tiled/split coordinates, steps (dividing the bound or not), origins, offsets and
strides are therefore in; coupled (`i + j`), singular and reversed mappings are out. Every
product and sum of coefficients, steps, origins and extents is computed in checked 64-bit
arithmetic; one that does not fit refuses the nest ("... overflows 64-bit arithmetic";
ISSUE-002, fixed — at 6e25528 it wrapped around).

**Lowering** (`--affine-matmul-to-systolic`): rank-2 identity-layout memrefs; each operand's
`memref.subview` built from **its own view** of the coordinate's `(offset, scale, extent)`:
offset `o + s · offset`, stride `s · scale` (since the fix of ISSUE-001; before, one view per
coordinate); offsets ≥ 0 and the operand's last index in bounds on static dims, in checked
arithmetic (ISSUE-002); equal element types; A and B provably disjoint from C (`restrict` /
fresh allocations, or `assume-no-alias`); `A × A` through one view, not two different views of
one memref; the fleet plan's tiles proven an exact partition.

**Driver** (`systolic-cc`): fp32 only (double arrays stay on the host), not `static`/`inline`,
not from a macro; `--verify` compares every offloaded call with the function as written.

**Update 2026-10-15 (ISSUE-005, the P0 operand views; specification `docs/AFFINE_GEMM_MAPPING.md`
v2).** The positional form above is tried first, unchanged; when it does not apply, the
recognizer finds the coordinates by **groups and roles** (loops that occur together in a
subscript are one coordinate; the operands that read it fix its role), so that operands of
**any rank**, **transposed** operands (`A[k][i]`, `B[j][k]`, `C[j][i]`), **fixed (extent-one)
dimensions** anywhere and **roles of extent one** (M, N or K = 1: a constant subscript, SQ-4
decided; a matrix-vector product's rank-1 operands, also under repetition loops) are
recognized; two subscripts of a role may run in **opposite directions** (`A[7 - i]` with
`C[i]`). Nests of one or two loops are candidates when their body is the multiply-accumulate.
A `min`/`max` of constants in a loop bound is folded. Each operand's view is a rank-reducing
`memref.subview` with **signed strides** (a dimension read backwards: a negative stride, its
largest index as the offset), then `memref.transpose` or `memref.expand_shape` as needed; every
index range and linear element address is checked in 64-bit-checked arithmetic. **Floating
point:** the systolic execution sums each tile from zero and adds the sum to C — a different
sequence of IEEE additions than the loop's — so by default no fp GEMM is offloaded; the
permission is the `reassoc` flag on the accumulation (`systolic-cc --fp-reassociation`,
`systolic-cfront --fp-reassociation`) or `--affine-matmul-to-systolic=allow-fp-reassociation=true`.
The showcase and the generated cases are run with the permission (`scripts/stages.py`). Every
refusal is classified (PROVEN_INVALID, VALID_BUT_LOWERING_UNSUPPORTED, UNKNOWN; an empty GEMM
and an FP refusal PROVEN_VALID): `--test-affine-matmul-discovery` prints `class:`,
`--affine-matmul-to-systolic=classify=true` appends it to the remark.

**Update 2026-10-15 (ISSUE-006, batched GEMMs).** A coordinate every operand reads (the
partition prefix P*, `C[b][i][j] += A[b][i][k] * B[b][k][j]`) is a **batch coordinate**: the
dense partition takes the P rows of T as such, and the lowering **re-emits the batch loops**
(the nest's loops that drive a batch coordinate, with their own iterations -- a max/min bound
folded, as the analysis folds it --, in their original order) around the tiles, one GEMM per batch iteration on its own slice of each operand
(the operand's batch dimension at a dynamic offset, an `affine.apply` of the coordinate's
expression). Nothing is required of a batch coordinate beyond the loops as written (no density,
no injectivity); a batch loop may not also drive I, J or K (the batches would be coupled:
refused). The three operands must read a batch coordinate through one and the same expression
(else VALID_BUT_LOWERING_UNSUPPORTED -- superseded by ISSUE-007 below); an operand without the
batch subscript (B shared by every batch) is refused as a second coordinate in one GEMM role;
the remark's "the enclosing loops run it N time(s)" counts the batch iterations, so
`systolic-cc` reports the invocations per call correctly.

**Update 2026-10-16 (ISSUE-007, batch-dependent operand views; `docs/AFFINE_GEMM_MAPPING.md`
section 16).** Every loop is classified by the operands that read it (A and C: I; B and C: J;
A and B: K; all three: a **batch loop**). The factors may read a batch loop through their own
expression -- in a dimension of their own (`A[b + 1]` with `C[b]`) or as an affine offset of a
role's dimension (`C[8i][j] += A[4i - j][5k] * B[5k][j]`: per `j` a 4 x 1 x 4 product, A's row
view moving with `j`) -- while C reads every batch loop alone (a dimension of C mixing a batch
loop with another loop, `C[i + k][j]`, is refused -- superseded by ISSUE-008 below). Per batch
value the role loops must be a dense GEMM of strided views and every index (role range plus
batch range) inside its array; the lowering offsets each operand's view per batch iteration (an
`affine.apply` per dimension). Offloaded since: `depth.c` ng_skew, showcase ext_015..ext_018,
and since the review fixes of 2026-10-16 ext_023..025 (a batch loop the positional reading has
drive two coordinates, `A[i][k + j] * B[k + j][j]`; a positional prefix that runs once -- a
fixed index, not a batch); case_087 and case_127 were still refused (offloaded since ISSUE-008).

**Update 2026-10-16 (ISSUE-008, the generalized affine GEMM mapping; `docs/AFFINE_GEMM_MAPPING.md`
section 19).** The user's decision: the specification and the decision procedure are those of a
batched GEMM of **affine operand views** (G1: every operand's subscripts are an affine view of
its two coordinates plus batch terms; G2: C's view is injective per batch; plus the integer
domain, the reduction correspondence, bounds and the lowering's exactness -- G1 + G2 alone are
never taken as sufficient). Consequences: (1) **C purity is dropped** -- C may read batch loops
in its role dimensions as long as its view is injective per batch (`C[i + k][j]`: per `k` a
rank-1 update, consecutive batches' output windows overlapping and adding up in program order;
`C[i + j][j]` with `A[i + j][k]`: per `j` a GEMV); (2) **skewed and merged views** -- a dimension
combining both of an operand's roles (`A[i][i + k]`, `B[j + k][j]`, `C[2i + j][j]`) or one role
across several dimensions -- are recognized (the readers analysis, `byReaders`) but **not
lowered yet**: VALID_BUT_LOWERING_UNSUPPORTED ("X has a skewed or merged view ... which is not
lowered yet"), a FALSE_NEGATIVE of ISSUE-008 until the lowering builds them
(`memref.reinterpret_cast`); (3) **SQ-5 is decided**: a batched GEMM with two of M, N, K equal
to 1 per batch (a batch of dot products, `C[i][j] += A[i][j + k] * B[i + k][j]`; a batch of
scaled vectors) is offloaded; (4) a nest whose canonical decomposition (roles by readers) fails
but which is a batched GEMM once more loops are demoted to batch loops (a loop read by one
operand, separate coordinates in one role, a role that is not one dense coordinate, C not
injective per batch) is **mathematically valid**; the compiler does not construct that
decomposition: a documented limitation (basis `demotion`), never "not a GEMM"; (5) a positional
reading in which one loop drives two coordinates is no longer restored when the analyses refuse
the nest (their refusal is the reason). Re-decided showcase cases (each with its record
`redecided`): case_087, case_127 (offloaded), case_088, case_126 (skewed views: recognized,
FALSE_NEGATIVE until the lowering), case_090, case_095, case_123, case_124, case_125 (valid by
demotion: DOCUMENTED_LIMITATION); case_089 stays REJECT (refused at recognition now:
element-wise). The three tests of the request are ext_026..028 (section 8, docs section 19.6).

## 2. Support specification (what a correct compiler must do)

The expectation of every test is set by this section and by the mathematics of the nest —
**never by the output of the current compiler**. A refusal of a valid GEMM is a finding, not a
redefinition of "GEMM".

**Definition.** A loop nest is a *dense GEMM* iff its iteration domain is a box over loop
counters that can be grouped into three disjoint sets enumerating I ∈ [0, M), J ∈ [0, N),
K ∈ [0, K) (each tuple once per repetition), the statement accumulates the product of two array
elements into a third, and the subscripts of A depend only on (I, K), of B only on (K, J), of C
only on (I, J), each subscript being an injective affine function of its coordinate's loops.
Then `C'(I,J) += A'(I,K) B'(K,J)` with `A'`, `B'`, `C'` the operands' own views. Mathematically
the following are *also* GEMMs (and are marked as such) but are **outside** this compiler's
specification by explicit, documented decision (DOCUMENTED_LIMITATION): a batched GEMM with an
operand shared by every batch (`C[b] += A[b] B`: B would be broadcast over the batch, or A and C
merged over (b, i)), alpha ≠ 1 (including `-=`), beta = 0 written as an initialisation inside the
nest or a scalar accumulator, double or mixed precision, operands that may alias C, a repetition loop of more than
one iteration inside a reduction loop of more than one iteration (hoisting it would reorder the
accumulation; with a single-iteration reduction the hoist is legal and the nest is offloaded —
`failures/resolved/NOTE-001`), two different views of one array as A and B, and -- since
ISSUE-008 -- a nest that is a batched GEMM only with more loops as batch loops than its readers
give (a loop read by one operand, separate coordinates in one role, a role that is not one dense
coordinate, C not injective per batch: `docs/AFFINE_GEMM_MAPPING.md` section 19.2, the compiler
constructs only the canonical decomposition). (Transposed
operand views, one operand reading a shared coordinate in the direction opposite to the other's
— a negative-stride view — and matrix-vector products were on this list until 2026-10-15; they are
inside the specification since ISSUE-005. A coordinate that decreases in every operand is
supported since ISSUE-004.) **Floating point:** the specification is about the mapping; an
offload additionally needs the program's permission to reassociate the floating-point sum
(section 1, update), which the tests give explicitly. The C front end's subset (section 1) refuses
some of these before the recognizer sees them (a counting-down loop: "the loop condition must be
'counter < bound' or 'counter <= bound'"); the stage does not change the classification.

**Inside the specification (must be ACCEPTED):** every dense GEMM not listed above — any loop
order, origins, steps, `<`/`<=`, any affine offsets and strides on any operand (**per operand**:
`A[i+2][k]` with `C[i][j]`, `C[2i][j]` with `A[i][k]`, `A[i][k+1]` with `B[k+2][j]` are dense
GEMMs: the basis is the compiler's own documentation — `AffineMatmulPartitioning.h`: "nonzero
origins, access offsets and strides ... are all of this form"; `AffineMatmulAnalysis.h`: one
factor map `phi` per operand — and the task's index family `c_i i + c_j j + c_k k + c_0` per
operand), coordinates split over several loops in any order, repetition loops outside the
reduction, the GEMM inside larger arrays, `A × A`; since ISSUE-005 also transposed operands
(A, B or both, independently -- executed coverage of both together since 2026-10-16:
`SystolicCC/affine_transpose_ab.c`, showcase ext_019..022, `docs/AFFINE_GEMM_MAPPING.md` section 17),
opposite directions (a negative-stride view), operands of any rank with fixed dimensions, and
roles of extent one (M, N or K = 1: a constant subscript — SQ-4, decided 2026-10-15 — or a
matrix-vector product); since ISSUE-006 also batched GEMMs (the batch loops run the tiles once
per batch iteration), and since ISSUE-007 batched GEMMs whose factors read the batch loops
through their own expressions or as offsets of their role dimensions (`docs/AFFINE_GEMM_MAPPING.md`
section 16); since ISSUE-008 (2026-10-16) every GEMM and batched GEMM of **affine operand views**
(`docs/AFFINE_GEMM_MAPPING.md` section 19: roles by readers, G1, G2, the integer domain): C
reading batch loops in its role dimensions, injective per batch (`C[i + k][j]`, `C[i + j][j]`
with `A[i + j][k]`), skewed and merged views (`A[i][i + k]`, `B[j + k][j]`, `C[2i + j][j]`,
`C[4i + j]` with `j` in [0, 4) -- recognized, their lowering not implemented yet: FALSE_NEGATIVE,
ISSUE-008), and batched GEMMs with two of M, N, K equal to 1 per batch (SQ-5, decided
2026-10-16).

**Must be REJECTED (TRUE_NEGATIVE):** element-wise products -- every loop read by all three
operands, in every decomposition a batch loop, no row, column or reduction loop (`C[i][j] +=
A[i][j] * B[i][j]`; the former "singular mappings" `C[x+z][y+z] += A[x+z][x+y+2z] *
B[x+y+2z][y+z]` and Test 2 on a box: docs section 19.6) --, a non-box iteration domain that is
not the affine image of a box (the triangle `j >= i`, mlir_009: no (batch, I, J, K) box
corresponds to its points; one that is, a parallelogram, is a GEMM by docs section 19.7 whose
parametrization the compiler does not construct: refused as UNKNOWN), products with a third
factor or sums, no accumulation; and nests whose subscripts leave a static array dimension
(undefined as programs: there is no GEMM to lower, and the refusal must say "out of bounds" or
"overflows 64-bit arithmetic", never come from a wrapped-around value — ISSUE-002). The compiler
must also never offload anything the specification refuses (a FALSE_POSITIVE is the most serious
finding: wrong code). Until 2026-10-16 this list also held the reduction index in the output,
coupled and skewed placements (`C[i+j][j]`, `A[i][i+k]`, `C[i+k][j]`), an output on a diagonal
(`C[i][i]`) and an operand reading the wrong coordinate (`B[k][i]`, `A[i][i]`); by the
generalized mapping (ISSUE-008) they are GEMMs or batched GEMMs of affine views -- inside the
specification, or valid by demotion (a documented limitation) -- see section 1.

**UNDETERMINED (spec questions, recorded in `generator/casespec.py: SPEC_QUESTIONS` and in
`showcase/manifest.json`):** a case whose expectation depends on one is run and its behaviour
recorded, its program must still compute the right result, and the question is decided here, in
writing — never by what the compiler happens to do. Two verdicts, neither of them a PASS:
**expected_refusal** — the compiler refused the case at the predicted stage with the predicted
reason and the program's check passed (a conservative answer that keeps the program on the host);
reported and counted on its own, not blocking; **spec_open** — the compiler offloaded it, a choice
the specification has not made: `run_showcase.py` / `regression.py` exit non-zero while one is
present unless `--allow-spec-open` is given (as for `--allow-known-issues`). A stage that differs
from the prediction is `unexpected_stage`, as for any case. The questions are also listed on
their own in `SPEC_QUESTIONS.md`. Open now:

- **SQ-1** — a coordinate split over several loops with a different mixed radix per operand
  (`A[4 i0 + i1][k]` with `C[8 i0 + i1][j]`): the definition admits it (each subscript is an
  injective affine function of the coordinate's loops), the "inside" list does not name it, and
  the operand views are not strided sub-matrices (a rank-2 `memref.subview` cannot express them).
  Inside (with a copy or a rank-3 view) or a documented limitation? Cases case_128, mlir_012.
  (Mathematically settled since 2026-10-16: with the coarse loop demoted to a batch loop the nest
  is a batched GEMM, `docs/AFFINE_GEMM_MAPPING.md` section 19.2; open is whether the compiler must
  construct that decomposition or the copy.)
- **SQ-2** — an empty GEMM (a loop of zero iterations): offload it with zero invocations, or
  refuse it? Either keeps the result correct. Cases case_129–131, mlir_013. (Refused with its own
  message since 2026-10-15; before, with the message meant for symbolic bounds.)
- **SQ-3** — a symbolic loop bound (`affine.for 0 to %n`), unreachable from the C front end: is
  the MLIR path's refusal a documented limitation, or must dynamic extents be supported? Case
  mlir_011.
Decided: **SQ-4** (2026-10-15, a role of extent one is offloaded) and **SQ-5** (2026-10-16, from
the user's instruction on Test 3, quoted in `SPEC_QUESTIONS.md`: mathematically legal, so inside
the specification): a batched GEMM whose per-batch GEMM has two of M, N, K equal to 1 --
the reduction index in the output with a single-iteration `j` (`C[i][k] += A[i][k] * B[k][j]`,
`j` in [1, 2): per `k` a column scaled by a scalar), a batch of dot products (Test 3,
`C[i][j] += A[i][j + k] * B[i + k][j]`, ext_028) -- is inside the specification and offloaded,
one small tile per batch value (basis `sq5_decided`; refused conservatively as
`expected_refusal` until then; `docs/AFFINE_GEMM_MAPPING.md` section 19.8).

**Gaps found and closed** (the specification says ACCEPT, the compiler refused): per-operand
offsets or strides on a shared coordinate — **ISSUE-001**, fixed (`failures/issues/ISSUE-001-*/`,
section 11 of the record); the unsound bounds arithmetic — **ISSUE-002**, fixed; a constant
coordinate crashed the dense partition — **ISSUE-003**, fixed (refused until SQ-4 was decided on
2026-10-15; offloaded since). Changed on request: loops that count down and coordinates that
decrease in every operand — **ISSUE-004**, supported since 2026-10-15 (README §1, §2); the P0
operand views (transposes, negative strides, extent-one roles, other ranks) and the
floating-point policy — **ISSUE-005**, since 2026-10-15; batched GEMMs — **ISSUE-006**, since
2026-10-15; batch-dependent operand views — **ISSUE-007**, since 2026-10-16; the generalized
affine GEMM mapping (affine operand views, C purity dropped, SQ-5) — **ISSUE-008**, since
2026-10-16 (recognition; the lowering of skewed and merged views is open). Specification
refinements made after a measurement (never to make a failing compiler look green; each one
argued from the mathematics and the documented rule): `failures/resolved/NOTE-*.md`.

## 3. Running the showcase set

```
cmake --build build                                  # systolic-opt, systolic-cfront, systolic-cc
python3 test/gemm_recognition/scripts/run_showcase.py --runtime soft      # software runtime model
python3 test/gemm_recognition/scripts/run_showcase.py --runtime board     # the FPGA (SYSTOLIC_FPGA_CONFIG)
python3 test/gemm_recognition/scripts/run_showcase.py --cpu-only          # plain C compiler only
```

Per case the runner executes, each a real tool invocation recorded with its command and output:
(1) the program with `cc` (the kernel as written vs its references); (2) `systolic-cfront
--function=<kernel>`; (3) `systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery`;
(4) `systolic-opt ... --affine-matmul-to-systolic=report=true`; (5) `systolic-cc --runtime=<r>
--explain --verify -DGEMM_RECOGNITION_STATS`; (6) the compiled program. The verdict per case
(`scripts/verdict.py`): **pass** (actual = specification, program check passed),
**known_false_negative** (spec ACCEPT, refused exactly as predicted; an open issue — fails the run
unless `--allow-known-issues`), **fixed_issue** (a predicted refusal no longer happens),
**unexpected_stage** (behaviour differs from the prediction: analyse), **false_positive**,
**fail_runtime** (wrong result, `--verify` mismatch, failed invocations, crash), **test_bug**
(references disagree), **tool_error**, **expected_refusal** (an UNDETERMINED case refused at
the predicted stage with the predicted reason, the program's check passed: not a PASS, not
blocking), **spec_open** (an UNDETERMINED case the compiler offloaded: not a PASS, and the run
fails on it unless `--allow-spec-open`). Results:
`reports/`. Nothing is ever marked PASS without having run; a stage that did not run is
`not_run` with the reason. MLIR-level cases (`mlir_*`, manifest `kind: mlir`) run the discovery
and lowering passes only — recognition, dense partition, operand views and the lowering's
remark are compared with the manifest, nothing is compiled by `systolic-cc` or executed, and
their report rows say so (`not_applicable`). After a run, `python3 scripts/inventory.py` writes
`showcase/INVENTORY.md`: every case with its expectation, basis, index forms, loops, the
execution evidence of the latest report, and the spec question it depends on, if any.

## 4. The unbounded generator

```
python3 generator/gen_cases.py --mode random    --seed 42 --count 50
python3 generator/gen_cases.py --mode enumerate --count 50 --start 0        # continue with --start 50
python3 generator/gen_cases.py --mode boundary  --count 50
python3 generator/gen_cases.py --reproduce random:42:17                     # one case, bit for bit
```

`--reproduce` prints exactly the bytes the batch run wrote (the manifest's `generated_from.reproduce`
command includes `--family`, which random mode depends on; the data seed is derived from the seed
tag with CRC-32, so it no longer depends on Python's per-process hash randomisation).

The index family: every loop order; origins; steps; offsets; strides; coordinates split over
2–3 loops; repetition loops outside / between / before the reduction; four statement
spellings; `<=`/`i++` syntax; leading-dimension slack; shapes from (1,1,1) upward without bound
(Cantor enumeration) or boundary values (1, k_max ± 1, k_max, 2 k_max, 3 k_max + 1, ...); three
families — `positive` (shared views), `views` (per-operand views: ISSUE-001's family, generated
on purpose to track it), `mutation` (one deliberate deviation: transposed operand, coupled,
singular, element-wise, output in the reduction, no accumulation, subtract, alpha, init inside,
scalar accumulator, double, no restrict, reversed rows, repetition inside the reduction, triple,
sum, batched). Every case is validated (in bounds via the Python oracle; the author's roles
checked against the subscripts), classified, de-duplicated against `generated/registry.json`
(canonical signature), written in the showcase format with its references, and counted in
`generated/coverage.json`. Invalid candidates are *generator rejections* (`rejected.json`),
never compiler results.

## 5. The closed loop

```
python3 scripts/loop.py --iterations 5 --batch 20 --mode random --seed 2026 --runtime soft
python3 scripts/loop.py --iterations 0 --mode enumerate                  # until interrupted
```

Per iteration: generate → validate → every stage → judge → **collect**: a new finding
(`fail_runtime`, `unexpected_stage`, `false_positive`, `test_bug`, `tool_error`, `fixed_issue`)
is written to `failures/<stamp>_<id>/` with the program, every stage's full output
(`stages.json`), the seed and reproduction commands, a diagnosis (stage, expected vs actual)
and — unless `--no-minimize` — a smaller program that still fails the same way; the program is
copied into `regression/corpus/`. A known issue reproducing (e.g. ISSUE-001's family) is
appended to `failures/known_issue_log.jsonl`. Then the next batch.

**Fixing the compiler** is a separate step, by a person or by Claude Code with the user's
approval, never by the loop:

1. read the failure record and the issue's analysis; decide whether the compiler or the
   expectation is wrong (the expectation changes only when the specification was wrong — in
   writing, in §2 — never to make a failing compiler look green);
2. change the compiler; add the reproducer as a lit test (`failures/issues/ISSUE-NNN-*/repro_*.mlir`
   is already one, marked `XFAIL` while the issue is open: remove the marker — the file is
   read-only, §7, so `chmod u+w` it first and `chmod 555` it again afterwards);
3. `cmake --build build`;
4. re-run the original failing case: `scripts/run_showcase.py --case ID` or
   `scripts/stages.py REPO FILE.c KERNEL soft`;
5. `scripts/regression.py run --runtime soft --check-systolic` (the whole showcase set, the
   corpus, every issue reproducer, and `check-systolic`), then `--runtime board`;
6. `scripts/regression.py diff` must show no REGRESSION; an IMPROVED line means the issue record
   is updated (status, date, commit) and the case's `current_implementation` prediction in
   `generator/casespec.py: predict()` is revised to the new rule — then
   `generator/make_showcase.py --force` regenerates the headers/manifest (the case files are
   read-only, §7, and a changed file is refused without `--force`; the specification layer does
   not change);
7. `scripts/regression.py save` writes the new baseline;
8. `scripts/loop.py` continues generating.

Rules the tooling enforces or expects: no PASS without execution; no editing of expectations to
match the compiler; a case is never moved out of the positive set because the compiler refuses
it; every PASS/FAIL comes with the command that produced it; the board run is the authoritative
one and the software model is labelled as such everywhere.

## 6. Safety

The scripts write only under `test/gemm_recognition/` (and the temporary work directory). They
never run `git reset`, `git clean`, remove files outside their output directories, or push.
Before any compiler change: `git status --short`, `git log -1 --oneline`, and keep uncommitted
research files untouched.

## 7. Test sources are read-only (mode 0555)

Every test-source file the tooling produces is written through
`generator/casespec.py: write_test_source()` (or re-protected with `protect_test_source()`) and
ends up `r-xr-xr-x` (0555), so that a case that was generated, judged and recorded cannot be
edited by accident afterwards — a case file is evidence, and its content must stay what the
`--reproduce` command regenerates. That covers:

| path | writer |
|---|---|
| `showcase/{positive,negative,extended}/*.c` and `*.mlir` | `generator/make_showcase.py` (create and regenerate; the MLIR-level cases too) |
| `generated/batch_*/*.c` | `generator/gen_cases.py` (random / enumerate / boundary; also when started by `scripts/loop.py`) |
| `failures/<stamp>_<id>/<id>.c` and `.../minimized/<id>.c` | `scripts/loop.py: write_failure()` |
| `regression/corpus/*.c` | `scripts/loop.py: add_to_corpus()` |
| `failures/issues/ISSUE-*/{repro_*,variants}.mlir`, `failures/resolved/*/*.c` | written by hand — `chmod 555` them when adding one (the existing ones are) |

Only the test sources are protected: `manifest.json`, `CASES.md`, `registry.json`,
`coverage.json`, `rejected.json`, `loop_state.json`, `results_*.json`, `REPORT.md`,
`stages.json`, `known_issue_log.jsonl`, the reports and the baselines stay writable; the Python
and shell tools keep their own modes; directories are never changed (so a protected file can
still be replaced, by unlinking it, and new files can still be created next to it); nothing
under the temporary work directory (compile outputs, `systolic-cc --keep-dir` copies, the
minimizer's candidate programs `min_N/<id>.c`) is touched.

Regeneration rules (`write_test_source`): an existing file with *identical* content is left
alone and just re-protected (the normal case for `make_showcase.py` and for `gen_cases.py`
re-run with the same seed/indices into the same directory); an existing file with *different*
content is refused with `ProtectedTestSource` — regenerating a case deliberately is
`python3 generator/make_showcase.py --force` (the corpus and the failure records are never
rewritten: a case is copied into them only once). The protection is enforced by the kernel for
ordinary users; root ignores mode bits, so the "cannot open for writing" check only means
something when the tests run as a non-root user.

`python3 scripts/test_permissions.py` verifies all of this in a temporary copy (every generator
mode, `make_showcase.py` create / unchanged / refuse / `--force`, the loop's failure record,
minimized program and corpus, the `--reproduce` byte-exactness, and that every tool file and
directory under `test/gemm_recognition/` keeps its mode); it needs no compiler.

## 8. The boundary review (cases added after the hundred)

The hundred cover: every loop order and statement spelling, shapes from 1×1×1 to 32×32×32 and
K-folding, non-zero origins, steps that divide their bounds, offsets and strides shared by the
operands, split (tiled) coordinates, repetition loops, sub-matrices, `A × A`, syntax variants;
negatives: element-wise, matrix-vector, batched, coupled, skewed and singular mappings,
transposed operands, the reduction index in the output, alpha, no accumulation, an imperfect
nest, double precision. What they left out, and what was added — each case in the set its
specification puts it in, never the set the compiler's behaviour would suggest:

| boundary | cases | specification |
|---|---|---|
| steps that do not divide the bound; three different steps; `<=` with a step; origin + step | case_101–105 | ACCEPT |
| a shared coordinate spelled differently by its two operands: offset on B (K or J), different offsets on A and C, stride on A / B / C / K, strides 2 and 3, every coordinate at once | case_106–114, mlir_014 | ACCEPT (ISSUE-001's family, offloaded since the fix) |
| a `long` counter past INT_MAX; an `int` counter whose last increment is INT_MAX; a 2^62 coefficient on a single-iteration loop; an `unsigned` counter | case_115–118, mlir_015 | ACCEPT |
| an affine loop condition counting down: `for (int i = 100; 2 * i - 3 > 0; i--)` (99 iterations, rows 2..100) | case_132 | ACCEPT |
| loops that count down (`i -= 2`, `--k`, `j > 0; j -= 3`, mixed directions) | case_119–122 | ACCEPT since ISSUE-004 (OUT_OF_SCOPE before; the cases stay in negative/) |
| one operand reversed against the other (`A[7 - i]` with `C[i]`) | mlir_010 | OUT_OF_SCOPE (opposite directions) |
| the output on its diagonal, an operand reading the wrong coordinate (`B[k][i]`, `A[i][i]`), coupled subscripts (`A[i][i+k]`, `C[i+k][j]`), a triangular domain | case_123–127, mlir_009 | REJECT until 2026-10-16; since ISSUE-008 (docs/AFFINE_GEMM_MAPPING.md section 19): case_127 ACCEPT (batched rank-1 updates, offloaded), case_126 ACCEPT (a skewed view of A: recognized, FALSE_NEGATIVE until the lowering), case_123–125 OUT_OF_SCOPE (batched GEMMs by demotion, not constructed); mlir_009 REJECT (not the affine image of a box) |
| the three tests of the 2026-10-16 request: `C[i][j] += A[i][i+k] * B[j+k][j]`; `C[i+j-2k][i+2j+k] += A[i+j-2k][i+j+k] * B[i+j+k][i+2j+k]` on a box; `C[i][j] += A[i][j+k] * B[i+k][j]` | ext_026–028 | ACCEPT (skewed views of A and B: FALSE_NEGATIVE until the lowering, ISSUE-008); REJECT (element-wise on a box; a GEMM only on the lattice parallelepiped D*, docs section 19.6); ACCEPT (a batch of dot products, SQ-5 decided: offloaded) |
| out-of-bounds views on static shapes (offset, per-operand stride, offset + extent by one, 2^62 offset) | mlir_001–003, mlir_006 | REJECT (undefined program) |
| coefficients whose products overflow int64 (scale 2^62 × extent, a per-operand view's last index, the coordinate's exclusive end) | mlir_004, 005, 007, 008 | REJECT; found and fixed ISSUE-002 |
| per-operand mixed radix of a split coordinate; empty loops (M, N or K = 0, `<=` form); a symbolic bound | case_128–131, mlir_011–013 | UNDETERMINED (SQ-1, SQ-2, SQ-3) |

Not added, and why: a C-level out-of-bounds or overflowing program is undefined behaviour and
cannot carry a CPU reference (those boundaries are MLIR-level cases); a loop whose counter would
overflow (`i <= INT_MAX; ++i`) is undefined as a C program as well (the front end refuses it:
"the loop counter could overflow"); a triangular C loop (`j = i`) cannot be written in the
front end's subset (static bounds) and is covered at the MLIR level.
