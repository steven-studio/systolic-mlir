# test/gemm_recognition — GEMM recognition test system

A fixed showcase set of 100 C programs, an unbounded case generator, and an automated test loop
for the affine GEMM path of this compiler: `systolic-cfront` (C → affine) →
`recognizeLogicalMatmul` → `getDenseLogicalGemm` / exact partitioning → `--affine-matmul-to-systolic`
→ `--lower-systolic-matmul-tile-to-fpga` → `systolic-cc` → the runtime (software model or the
board). Everything here is a test: nothing under `test/gemm_recognition/` modifies the compiler.

```
test/gemm_recognition/
├── README.md                this file: layout, the support specification, how to run, the loop
├── showcase/                the hundred (+ extended): positive/, negative/, extended/, manifest.json,
│                            CASES.md, README.md
├── generator/               casespec.py (model, oracle, analysis, predictor, spec, C emitter),
│                            showcase_cases.py (the hundred's specs), make_showcase.py,
│                            gen_cases.py (the unbounded generator)
├── scripts/                 stages.py (one program through every stage), verdict.py (judging),
│                            run_showcase.py, loop.py (the closed loop), regression.py (the gate),
│                            run_issue.sh (issue reproducers)
├── failures/                issues/ISSUE-NNN-*/ (recorded compiler issues: record, reproducers,
│                            lit XFAIL test), <stamp>_<id>/ (new findings of the loop),
│                            known_issue_log.jsonl
├── regression/              corpus/ (programs the loop found failing, re-run forever),
│                            baseline_<runtime>.json, last_run_<runtime>.json
├── generated/               batches of the generator, registry.json (signatures seen),
│                            coverage.json, loop_state.json
└── reports/                 actual results of run_showcase.py (json + md); latest_<mode>.json
```

## 1. What the compiler supports (read from the sources at commit 6e25528)

Stage by stage. "Documented" means the sources or the existing tests say so explicitly.

**Front end** (`tools/systolic-cfront`, clang 24 AST): `void` functions; parameters
`float|double A[M][K]` (`restrict` → `llvm.noalias`), scalar float/double; `for (int|long i = LB;
i < UB | i <= UB; ++i | i++ | i += S | i = i + S)` with integer-constant bounds; subscripts that
are affine expressions of the enclosing counters (`+`, `-`, constant `*`); assignments to array
elements (`=`, `+=`, `-=`, `*=`, `/=`); `+ - * /` and unary `-` on float/double. Outside the
subset (an error, never an approximation): local variables, `if`, `while`, calls, pointer
arithmetic, globals, integer arrays, non-affine or symbolic subscripts, symbolic bounds,
downward loops, `!=` conditions, `return` with a value.

**Recognizer** (`recognizeLogicalMatmul`): a perfect chain of `affine.for` ops (from some anchor
down to the innermost) whose innermost body holds exactly three `affine.load`, one
`affine.store`, one `arith.mulf`, one `arith.addf` and nothing else; the stored value is
`addf(load C, mulf(load, load))` (either order of the add); the accumulator is loaded and stored
through the same map and indices; every loop of the chain is used by an access, and no access
uses a loop outside the chain; all index operands are loop IVs (no symbols); the three access
maps have the same number of results r ≥ 2, with a partition prefix of r − 2 results identical in
all three, followed by `A = (I, K)`, `B = (K, J)`, `C = (I, J)` **as identical affine
expressions** for each shared coordinate, pairwise distinct; the expressions are linear forms
(no mod/floordiv). Static or symbolic (`0 to %n`) bounds. A repetition loop (used by no
access) must be outside the chain — the lowering pass first moves one that sits between the
GEMM's loops outward when the interchange is legal (not inside a reduction loop).

**Dense partition** (`getDenseLogicalGemm`): no partition prefix (batched is refused with a
message); static bounds; every loop drives exactly one of I, J, K with a non-negative
coefficient; the loops of one coordinate enumerate it densely (sorted by scale = coefficient ×
step, each scale is the previous scale × the previous loop's trip count). Tiled/split
coordinates, steps, origins, offsets and strides are therefore in; coupled (`i + j`), singular
and reversed mappings are out.

**Lowering** (`--affine-matmul-to-systolic`): rank-2 identity-layout memrefs; each operand's
view built from the coordinate's `(offset, scale, extent)`; offsets ≥ 0 and in bounds on static
dims; equal element types; A and B provably disjoint from C (`restrict` / fresh allocations, or
`assume-no-alias`); `A × A` through one view, not two different views of one memref; the fleet
plan's tiles proven an exact partition.

**Driver** (`systolic-cc`): fp32 only (double arrays stay on the host), not `static`/`inline`,
not from a macro; `--verify` compares every offloaded call with the function as written.

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
specification by explicit, documented decision (DOCUMENTED_LIMITATION): batched GEMMs
(partition prefix), transposed operand views (the operand convention is row-major (I,K)/(K,J)/(I,J)),
alpha ≠ 1 (including `-=`), beta = 0 written as an initialisation inside the nest or a scalar
accumulator, double or mixed precision, operands that may alias C, a coordinate that decreases
along its loop (negative stride), a repetition loop of more than one iteration inside a reduction
loop of more than one iteration (hoisting it would reorder the accumulation; with a single-iteration
reduction the hoist is legal and the nest is offloaded — `failures/resolved/NOTE-001`), two
different views of one array as A and B, and matrix-vector products (two loop dimensions, rank-1
operands).

**Inside the specification (must be ACCEPTED):** every dense GEMM not listed above — any loop
order, origins, steps, `<`/`<=`, any affine offsets and strides on any operand (**per operand**:
`A[i+2][k]` with `C[i][j]`, `C[2i][j]` with `A[i][k]`, `A[i][k+1]` with `B[k+2][j]` are dense
GEMMs: the basis is the compiler's own documentation — `AffineMatmulPartitioning.h`: "nonzero
origins, access offsets and strides ... are all of this form"; `AffineMatmulAnalysis.h`: one
factor map `phi` per operand — and the task's index family `c_i i + c_j j + c_k k + c_0` per
operand), coordinates split over several loops in any order, repetition loops outside the
reduction, the GEMM inside larger arrays, `A × A`.

**Must be REJECTED (TRUE_NEGATIVE):** element-wise products, nests without a reduction
coordinate, the reduction index in the output, coupled (`C[i+j][j]`) and skewed placements
(GEMM values over a non-rectangular region are not one dense GEMM), singular mappings, products
with a third factor or sums, no accumulation. The compiler must also never offload anything the
specification refuses (a FALSE_POSITIVE is the most serious finding: wrong code).

**Gaps found so far** (the specification says ACCEPT, the compiler refuses): per-operand offsets
or strides on a shared coordinate — **ISSUE-001**, `failures/issues/`. Specification refinements
made after a measurement (never to make a failing compiler look green; each one argued from the
mathematics and the documented rule): `failures/resolved/NOTE-*.md`.

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
(references disagree), **tool_error**. Results: `reports/`. Nothing is ever marked PASS without
having run; a stage that did not run is `not_run` with the reason.

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
| `showcase/{positive,negative,extended}/*.c` | `generator/make_showcase.py` (create and regenerate) |
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
