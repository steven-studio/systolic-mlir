# P1: MLIR CFG polynomial recurrence recognition

Worktree: `systolic-mlir-polynomial-summation`, branch
`feat/polynomial-summation`. Protected P0 checkpoint:
`polynomial-summation-p0-20261029`, commit
`057ac9e482ba9cb6c6cba7cd82b1c4d3f5e8bde1`.

## Recovery record (2026-10-29)

The interrupted implementation was preserved. At recovery, the only tracked
changes exposed P0's existing exact range enclosure as `Polynomial::bounds`.
The P1 header, implementation, standalone build/reporting tool, and seven
fixture files were already present. No production files had changed.

Initial validation, before further edits:

- P0: 42,277 C++ checks and 13,443 independent Python comparisons passed.
- P1 library and reporting tool compiled without errors.
- P1 CTest baseline: two P0 tests passed; two P1 entries failed because the
  C++ test contained only an unfinished placeholder and lit contained no tests.
- `mlir-opt` verified `accepted.mlir`, `rejected.mlir`, `overflow.mlir`, and
  `structured.mlir`. Three intentional malformed inputs failed (branch operand
  type mismatch, dominance violation, duplicate SSA name). These are expected
  negative fixtures, not fixture defects.
- All 14 acceptance examples reported PROVEN in modular mode. All 14 rejection
  examples reported UNKNOWN. Raw structured IR reported UNKNOWN.

The existing fixtures and P0 tests will remain unchanged. Completing the
assertion-free C++ placeholder and adding tests supplies missing coverage.

## Completion record

Implemented: verified SSACFG analysis using MLIR dominance and loop analysis,
branch-interface operand transfer, unique loop entry/backedge/exit checks,
simultaneous old-state expression DAGs, additive dependency extraction, P0
solving and exact symbolic identity checks, explicit integer contracts,
original-source overflow checks, and read-only deterministic JSON reporting.

P1 is complete for the subset below. Independent execution, symbolic identity,
rejection, read-only, determinism, verification, and differential tests pass.
No P1 implementation tasks remain for that subset. The limitations below are
explicit exclusions, not claims of completeness for arbitrary polynomial CFGs.
No P2, termination inference, compiler lowering, or IR rewriting was added.

Validation commands (from the worktree root):

```sh
cmake -S research/polynomial-summation-mlir -B build/polynomial-summation-mlir -G Ninja -DCMAKE_BUILD_TYPE=Release -DMLIR_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/mlir -DLLVM_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/llvm
cmake --build build/polynomial-summation-mlir -j3
ctest --test-dir build/polynomial-summation-mlir -V
```

Soundness boundary: expressions describe header states after a given number
of completed backedges, conditional on that visit occurring. They do not
predict trip counts, termination, or return values. Default arithmetic is
modulo `2^w`. Both integer interpretations of finite-width MLIR require an
explicit finite horizon and either proof or an explicit no-overflow assumption.
Dynamic casts, conditional updates, multiple backedges/entries, memory effects,
and unsupported nonlinear/cyclic recurrences fail closed.

## 1. Files and standalone interface

| File | Role |
| --- | --- |
| `include/Systolic/MLIRPolynomialRecurrence.h` | Options, extracted states, result, read-only `cfg::analyze(func::FuncOp)` API |
| `lib/Systolic/Analysis/MLIRPolynomialRecurrence.cpp` | Verified CFG extraction, semantic checks, P0 calls, deterministic serialization |
| `include/Systolic/PolynomialSummation.h`, `lib/Systolic/Analysis/PolynomialSummation.cpp` | Expose the existing P0 range enclosure as `Polynomial::bounds`; retain the same algorithm and reject a negative range horizon |
| `research/polynomial-summation-mlir/CMakeLists.txt` | Separate research build, including the unchanged P0 test registration |
| `research/polynomial-summation-mlir/PolynomialCFGReport.cpp` | Opt-in command-line JSON report and read-only assertions |
| `research/polynomial-summation-mlir/lit.site.cfg.py.in`, `lit/lit.cfg.py`, seven `lit/*.mlir` files | Dedicated lit configuration and acceptance, rejection, overflow, malformed, structured, read-only, and `extui nneg` tests |
| `test/polynomial-summation-mlir/PolynomialCFGTest.cpp` | Completed the existing assertion-free placeholder with independent real-IR execution and API tests |
| `test/polynomial-summation-mlir/differential.py` | Generated MLIR and Python arbitrary-precision direct-execution oracle |
| `test/polynomial-summation-mlir/ExtUINNegTest.cpp`, `Inputs/extui-nneg.mlir` | Focused cast-attribute regressions, including source-width boundaries and poison rejection |
| `test/polynomial-summation-mlir/Inputs/*.mlir` | All seven original P1 fixtures preserved byte for byte; `extui-nneg.mlir` added separately |

No production CMake target includes this directory or links the P0/P1 research
libraries. `lib/Systolic/CMakeLists.txt` still includes only `IR` and
`Transforms`. The original production lit configuration excludes `Inputs`;
new executable lit cases reside in the research directory. No existing test
expectation, fixture input, or P0 test source was changed. SHA-256 checks
confirmed all seven preexisting P1 fixtures were preserved. No commits, tags,
merges, or pushes were made. The protected tag still resolves to the checkpoint
above; its annotated tag object is
`52d48b45b6645ca2e280eec3657475abbdaf1439`.

The build uses the local LLVM/MLIR package at
`~/work/llvm-project/build/lib/cmake/{llvm,mlir}`. The package disables RTTI;
P1 matches that setting while retaining C++ exceptions for P0's checked algebra.
No new dependencies are added to production lowering.

Example use:

```sh
build/polynomial-summation-mlir/polynomial-cfg-report \
  test/polynomial-summation-mlir/Inputs/accepted.mlir \
  --function=sum_squares --evaluate=10
```

This reports initial state `(0,0)`, increments `(1,s0*s0)`, and binomial
coefficients `[0,1]` and `[0,0,1,2]`. At header iteration 10, the states are
`10` and `285`. In ordinary notation:

```text
i(t) = t
s(t) = C(t,2) + 2*C(t,3) = t*(t-1)*(2*t-1)/6
```

The source adds the old `i`, starting at zero; it sums squares from `0` through
`t-1`. The preserved `sum_squares_next_i` mutation instead uses the SSA value
`i+1`, yields coefficients `[0,1,3,2]`, and evaluates to `385` at `t=10`.
These forms are derived, not stored per-power formulas.

`--mode=signed --horizon=10` requests the signed contract. The mathematical
mode similarly requires a horizon. `--assume-no-overflow='...'` supplies an
explicit caller assumption if proof is inconclusive; the result is then
CONDITIONAL, never mislabeled PROVEN. `--print-ir` prints the parsed module
after analysis and read-only checks. Invalid parsing/verification exits with
code 1 and an UNKNOWN JSON result. A successfully reported unsupported
function has status UNKNOWN and an empty `states` array; the reporting process
itself exits with code 0. Results hold a non-owning header pointer: the input
IR must remain alive and unchanged while that pointer is used.

## 2. Supported MLIR syntax and CFG shapes

The input is a verified, nonexternal `func.func` with an SSACFG body. Reachable
operations are restricted to scalar integer `arith.constant`, `arith.addi`,
`arith.subi`, `arith.muli`, `arith.cmpi`, the constant-only casts below,
`cf.br`, `cf.cond_br`, and `func.return`. Unknown/nonempty memory effects are
rejected, including unused loads/stores and unregistered operations.

There must be one reachable natural loop, one entry edge to its header, one
backedge, and one conditional exit edge. Apart from the exit condition, the
cycle consists entirely of unconditional branches. Initialization follows an
unconditional entry path; the exit tail follows unconditional branches to a
return. The exit test may be before or after the update. Physical block order
does not matter. Blocks may forward and permute arguments. All header state
arguments must have the same scalar signless `iW` type.

Initial states and transition coefficients are exact constants. Initial
constant expressions may use integer add/subtract/multiply. Function parameters
can supply comparison bounds, but parameter-dependent arithmetic and symbolic
initial states are outside this subset. Comparisons retain their MLIR meaning;
the analysis does not interpret a comparison as a mathematical trip count.

`arith.extsi`, `arith.extui`, and `arith.trunci` are accepted only for
constant-only chains, evaluated with APInt sign extension, zero extension, or
truncation. Their values may change: `extui(-1 : i8)` becomes `255`. These are
not assumed identity casts. Cast operands depending on state, function
parameters, or an intervening arithmetic computation are rejected. Overflow
flags on truncation are rejected. Index types, vectors, tensors, and mixed
state widths are unsupported.

For `extui nneg`, the exact constant operand must have a zero sign bit at its
own source width. A negative operand or an unproven operand yields UNKNOWN in
every mode. The check precedes zero extension, including within cast chains;
the unsigned interpretation of `255 : i8` does not satisfy the precondition.

Unreachable blocks must still verify, but their operations and predecessor
edges do not affect extraction. Reachability is graph reachability, not a
speculative constant-condition analysis. Raw `scf.for`/`scf.while` regions are
UNKNOWN. A test separately runs MLIR's existing `--convert-scf-to-cf` pass and
then analyzes the resulting real CFG. P1 itself performs no conversion.

## 3. SSA recurrence extraction and dependency graph

1. Verify the function. Use `DominanceInfo` to require SSACFG dominance and
   identify reachable blocks. Check every reachable operation's effect and
   supported syntax, even if it has no live result.
2. Build edges with `BranchOpInterface::getSuccessorOperands`. Reject
   branch-produced arguments. Instantiate `CFGLoopInfo` on MLIR's dominator
   tree. Compare its unique natural loop with the unique cyclic component from
   LLVM's SCC iterator; require header dominance throughout that component.
   Check the unique entry, backedge, exit, and unconditional paths described
   above. This rejects irreducible components and ambiguous continuation paths.
3. Number states `s0,s1,...` by header argument position. Recover each initial
   operand from the entry edge, forwarding through unique reachable incoming
   edges and evaluating constants exactly. Never infer equations from SSA names.
4. Recursively trace each backedge operand. Every header argument becomes a
   variable for the **same old-state vector**. Other block arguments follow
   their unique incoming edge. Arithmetic definitions form immutable P0
   `Expression` DAGs. Thus a use of `%yn = addi %y, %square` expands to
   `y + x*x`, whereas a use of `%y` remains the old `y`.
5. Structurally remove exactly one additive occurrence of the argument being
   updated, obtaining `next_j = old_j + increment_j`. Direct forwarding gives
   increment zero. Subtraction is supported when the self occurrence is in
   its left operand only. Multiple/nonadditive self occurrences are rejected;
   algebraic coincidences are not guessed.
6. Collect increment dependencies in sorted header-argument order and pass
   the system to P0. P0 solves only an acyclic dependency graph. An affine
   induction variable is the special case with a constant increment, including
   a negative or zero increment. A polynomial accumulator depends on already
   solved states. Mutual/self-dependent increments return UNKNOWN.
7. Check full next-state polynomial identities, increment identities, and
   initial conditions before returning any forms. Check original-source
   intermediates separately for the requested integer contract.

For `x'=x+1, y'=y+x*x, z'=z+y`, the increment dependency order is
`x -> y -> z`. For explicit next-y use, `z'=z+(y+x*x)`, dependencies are
`x -> y` and `{x,y} -> z`. Simultaneous updates are retained in both cases;
neither uses an imperative in-place update order. The results are deterministic
in header argument order, regardless of dependency-solving order.

## 4. Mathematical model and correctness argument

Let `B_k(t)=binomial(t,k)`. P0 represents each sequence as
`Q(t)=sum_k a_k B_k(t)` with exact `cpp_int`/rational coefficients. This is the
Newton basis used for polynomial chains of recurrences. P1 implements no new
polynomial multiplication, finite differences, or integration algorithm.

For an accepted system, the graph/path and SSA checks establish a unique
transition term `F_j(old)` for every completed backedge. Branch arguments bind
simultaneously, so tracing SSA definitions is substitution into the old vector,
not sequential assignment. The additive check establishes the algebraic
identity `F_j(X)=X_j+G_j(X)`.

In dependency order, suppose the states needed by `G_j` have already been
derived. Substitute their polynomials using P0 operations to obtain
`P_j(t)=sum_k a_jk B_k(t)`. Then P0 constructs

```text
Q_j(t) = initial_j + sum_k a_jk B_(k+1)(t).
```

Pascal's polynomial identity gives
`B_(k+1)(t+1)-B_(k+1)(t)=B_k(t)`. Also `B_(k+1)(0)=0` for `k>=0`.
Consequently `Q_j(t+1)-Q_j(t)=P_j(t)` and `Q_j(0)=initial_j` exactly.
Induction on dependency order constructs the whole system; induction on
completed backedges establishes uniqueness of the resulting state sequence.
The implementation additionally compares canonical coefficient vectors for:

```text
Q_j.shift(1) == substitute(F_j, Q)
Q_j.difference() == substitute(G_j, Q)
Q_j(0) == initial_j.
```

These are symbolic polynomial identities, not identities inferred from a
finite sample. Direct execution is separate validation of extraction and
semantics. The formulas apply to **header visits after `t` completed
backedges**. A posttested loop can compute its returned value after its final
header visit without taking another backedge. This analysis deliberately
makes no claim that a function's return equals any particular header state.

## 5. Integer semantics and overflow contracts

MLIR signless integer arithmetic still has finite bit width; signless does not
mean unbounded. The source definitions are documented in the official
[Arith dialect](https://mlir.llvm.org/docs/Dialects/ArithOps/) and
[ControlFlow dialect](https://mlir.llvm.org/docs/Dialects/ControlFlowDialect/).

| Mode | Meaning and obligations |
| --- | --- |
| Modular (default) | Exact residues modulo `2^W`. Add/subtract/multiply overflow flags must be absent. `extui nneg` requires a proven source sign. `integerLifts` are integer representatives; use `solution.valueAt` for residues. Overflow is allowed. |
| Mathematical | Exact integer lifts with a finite horizon and a proven/assumed bridge from the finite-width source. This does not silently redefine MLIR arithmetic. |
| SignedNoOverflow | Same bridge, plus P0's signed result-range checks on evaluation. A finite horizon is mandatory. |

Projection from integers to `Z/(2^W)` commutes with addition, subtraction, and
multiplication. Constants and constant-only casts are evaluated according to
their actual source bits. The exact lifted transition therefore projects to
the source transition; induction proves the modular result for every reached
header iteration in the requested domain. Binomial coefficients are evaluated
in exact arithmetic before reducing. No division-based IR rewrite is emitted.

For both integer modes, solve the equations with P0's mathematical arithmetic,
then prove the **original MLIR operations** fit signed range. This avoids
treating extraction's reassociation as a new source overflow obligation. Every
reachable add/subtract/multiply, including canceled expressions, dead results,
guard computations, and tail computations, is checked. Initial constant
arithmetic that overflows is rejected. All state polynomials are checked too.

The range proof reuses P0's enclosure. On integer `0<=t<=N`, each `B_k(t)` is
nonnegative and bounded by `B_k(N)`. Each positive coefficient contributes to
the upper bound and each negative coefficient to the lower bound. These
rational bounds must lie in `[-2^(W-1), 2^(W-1)-1]`. There are no floating-point
roots or sampled-overflow claims. The proof is sound but incomplete because it
forgets cancellation between different basis terms.

Conservatively, source expressions are checked throughout `[0,N]`, including
a hypothetical body evaluation at the final header visit. A safe loop can
therefore return UNKNOWN, even if its body only executes for `[0,N-1]`.
The final header's guard arithmetic is not skipped. An explicit caller
assumption may discharge an inconclusive proof, yielding CONDITIONAL. It
cannot legalize unsupported casts, flags, memory, CFG shapes, or recurrences.
`nsw` source operations are accepted only under these checked integer modes;
the flag alone is not taken as a proof. `nuw`, combined flags, and flagged
modular arithmetic are conservatively rejected to avoid poison obligations
that P1 does not establish.

`valueAt` rejects negative iterations and iterations beyond an explicit
horizon. No horizon is required for modular results with supported flags. Mathematical
and conditional results are not permission to remove loops or disregard
control-flow reachability.

### P1 final-review correction: `arith.extui nneg`

The final checkpoint review found that constant evaluation called `zext`
without inspecting the cast's `nneg` property. A verifier-accepted cast of
`-1 : i8` with `nneg` produces poison, but P1 previously fabricated `255` and
reported PROVEN in all three modes. This was a soundness defect, not a valid
modular interpretation. The P1 checkpoint was withheld during correction.

The minimal fix is confined to the `ExtUIOp` branch in `Extractor::constant`:
evaluate the operand to an APInt, inspect `getNonNeg()`, reject if
`source.isNegative()`, and only then zero-extend. `IntegerAttr::getValue()`
preserves the source width, and APInt's signed test inspects bit `width-1`.
The local API represents the property as a `UnitAttr` named `nonNeg`, exposed
by the boolean `ExtUIOp::getNonNeg()` accessor. Unknown operands already throw
`Unsupported` during constant evaluation. A violated precondition throws
before any result is memoized; the existing analysis boundary clears all
states, forms, and the header pointer and returns UNKNOWN. No general poison
representation or propagation infrastructure was introduced.

An ordinary unflagged `extui` still zero-extends a negative source bit pattern.
A proven nonnegative flagged source is accepted, including when that source
was produced by an earlier valid cast. An explicit no-overflow assumption does
not override a violated or unproven cast precondition.

The focused audit used the same pinned LLVM/MLIR commit as section 7:

| Supported operation | Semantic attributes/preconditions and P1 handling |
| --- | --- |
| `arith.constant` | P1 requires a scalar signless integer and `IntegerAttr`; MLIR verifies value/result type agreement. No poison-producing constant attribute is accepted. |
| `arith.addi/subi/muli` | `nsw`/`nuw` can produce poison. Existing checks reject flags in modular mode and reject `nuw`/combined flags in integer modes. `nsw` requires the original-source range proof or the explicit conditional contract. |
| `arith.extui` | `nneg` requires the source sign bit to be zero; now checked before extension. MLIR verifies strictly increasing width. |
| `arith.extsi` | No additional semantic flags in this revision; exact sign extension, with strictly increasing width verified by MLIR. |
| `arith.trunci` | `nsw`/`nuw` impose poison-producing truncation preconditions; P1 already rejects all flagged truncations. Unflagged constant truncation uses APInt, and MLIR verifies strictly decreasing width. |
| `arith.cmpi` | Predicate specifies signed/unsigned comparison; there are no additional poison-producing flags in this revision. P1 does not reinterpret guards or derive a trip count. |
| `cf.br/cond_br`, `func.return` | Branch operands/types and SSA dominance are verified. Branch weights are profiling information. P1 retains simultaneous transfers and makes no exit-value or defined-termination claim. |

No additional unresolved soundness defect was identified in this supported
operation audit. Unsupported operations, including general poison producers,
remain UNKNOWN. This is not a new general poison analysis.

## 6. Validation and adversarial coverage

Final Release build and combined CTest run: **6/6 entries passed, 0 failures**.
The build used `/usr/bin/g++` and the local MLIR package. During completion,
the new test interpreter initially lacked `ControlFlowOps.h`; adding the
operation declarations fixed that compilation error. No fixture or expected
result was changed to resolve a failure.

| Test | Actual result |
| --- | --- |
| Existing P0 C++ suite | 42,277 checks PASS |
| Existing P0 independent Python oracle | 13,443 comparisons PASS |
| P1 C++ API/extraction/identity suite | 517,759 checks PASS, including 1,961 independent APInt executions and 233,023 header-state comparisons |
| Focused `extui nneg` C++ suite | 1,244 checks PASS; 22 verifier-accepted functions; 27 accepted mode cases; 65 UNKNOWN cases including explicit assumptions; 594 independent direct-update comparisons |
| P1 generated Python oracle | 98 accepted MLIR functions; 27,702 exact values, 27,702 modular values, 27,702 numerical differences, and 201 externally lowered while-loop values PASS |
| Generated adversarial/resource cases | 6/6 UNKNOWN with no forms |
| Dedicated lit suite | 7/7 PASS |
| Refreshed LLVM experiment | 1,814 default-O2 runtime comparisons and 1,814 size-16 comparisons PASS |
| Previously failing review probes and controls | 9/9 PASS; all three negative-`nneg` cases now UNKNOWN |

The Python oracle also reports 985 structural assertions and 7 successful
subprocess checks. These counts overlap the suite totals where noted; they
are not separate unique test files. The lit suite verifies all five valid
fixture modules, the converted structured loop, and expected verification
failure of all three malformed fixtures. Generated modules are separately
verified by `mlir-opt --verify-each` (unregistered dialects enabled only for
the intentional unknown-effect case).

The new focused suite was run against the original evaluator first: its
verifier-accepted negative-boundary case failed as expected. After the fix,
every case passes without changing its expected result. The new fixture's i1
literals use typed integer syntax (`0 : i1`, `1 : i1`). Cases cover ordinary
zero extension of negative bits, flagged positive/zero inputs, signed maxima
and minima at widths 1/8/65, unsigned spelling of negative source bits,
initial and backedge casts, extension/truncation chains, dynamic unproven
operands, unused casts, and canceled poison. All three modes and explicit
no-overflow assumptions are exercised. Every rejection checks empty states,
empty polynomial forms, UNKNOWN solution status, a null header pointer,
non-evaluability, and an explicit reason. Accepted cases check exact identities
and direct arbitrary-precision repeated updates.

The correction was validated in `build/polynomial-summation-p1-review` with
`cmake --build ... -j3` and `ctest --test-dir ... -V`. The LLVM script in
section 7 was rerun with output directory
`build/polynomial-summation-p1-review/llvm-comparison-nneg`. Existing P0/P1
test files, fixtures, and assertions were preserved byte for byte; only new
tests and their research-build registration were added. The full production,
FPGA, Runtime, and full LLVM/MLIR suites remain NOT RUN.

The C++ interpreter executes actual constants, arithmetic, casts, comparisons,
and branches with arbitrary-width APInt. It snapshots all successor operands
before binding destination arguments. It does not consume extracted equations,
P0 algebra, or the analyzer's branch-transfer helper. It compares header states
for every bound `0..80`, and `0..255` for the i8 wrapping fixture. Independent
expected initial values, increments, and dependency sets are specified for
all 14 accepted fixture functions. All 14 rejected fixture functions must
remain UNKNOWN even when a no-overflow assumption is supplied.

The Python generator emits Horner-form arithmetic but its independent oracle
executes conventional sums of powers with Python integers. It covers powers
1 through 10, random degree-0..8 polynomials, coefficients exceeding 180 bits,
nonzero accumulators, starts on either side of zero, negative/zero/positive
steps, widths 8/16/32/128/256, and triangular old/next-value dependencies.
Returned coefficients are evaluated using Python `Fraction` and `math.comb`;
the expected sequence comes from direct updates, not a second integrator.

Adversarial cases cover off-by-one old/next-index mutations, simultaneous
old/next-y mutations, posttested loops, permuted forwarding arguments and
noncanonical block layouts, unreachable unsupported arithmetic and predecessor
edges, conditional updates, multiple backedges/entries, irreducibility,
nonlinear/cyclic recurrences, dynamic narrowing, memory effects, unknown
initial values, mixed widths, index/select/division, invalid SSA/dominance,
canceled and dead overflow, final-header overflow, flags, signed proof versus
assumption, and evaluation outside the contract horizon. Six generated
rejections exercise excessive expression expansion, degree, depth, width,
operation count, and unknown effects. Public `Polynomial::bounds` is tested
by enclosure checks and negative-domain rejection without editing P0 tests.

Read-only validation compares complete generic IR with debug locations before
and after analysis, including rejection paths, and re-verifies malformed API
input on a cloned function. The reporting tool analyzes twice and compares
serialized results. Lit additionally compares printed IR before/after and
compares output from independent reporting processes. The Python test checks
cross-process determinism and unchanged source-file bytes.

The production compiler regression, runtime, FPGA, and full LLVM/MLIR test
suites were not rerun. The unchanged production-behavior claim rests on the
unchanged tracked production sources, tests, and build graph, and the absence
of any production linkage or pass registration. All seven preserved P1
fixtures and all P0 tests were audited against their recovery contents.

## 7. LLVM ScalarEvolution comparison

The inspected local tree is LLVM/MLIR **24.0.0git**, commit
**`7615da10b75c57286a4032eac8bb43c1b8d2ba46`**. It remained clean. P0's
[source inspection and measured comparison](POLYNOMIAL_SUMMATION_P0.md#6-llvm-implementation-inspected)
remain applicable. Relevant pinned implementations:

- `SCEVAddRecExpr`: `llvm/include/llvm/Analysis/ScalarEvolutionExpressions.h:324`.
- `ScalarEvolution.cpp`: `BinomialCoefficient:868`,
  `evaluateAtIteration:981/986`, `getMulExpr:3133` (polynomial fold near 3356),
  `getAddRecExpr:3685/3704`, `createSimpleAffineAddRec:5729`,
  `createAddRecFromPHI:5791`.
- `mlir/include/mlir/Analysis/CFGLoopInfo.h:47` instantiates LLVM's generic
  loop analysis for MLIR blocks. P1 reuses this, MLIR dominance and branch
  interfaces, and LLVM's SCC iterator rather than replacing CFG discovery.

LLVM already represents and evaluates polynomial AddRecs in the binomial
basis, multiplies compatible AddRecs, and recognizes additive PHI recurrences.
`createAddRecFromPHI` permits multiple incoming edges when their entry and
backedge **values** are respectively identical; P1 currently requires unique
edges and is stricter. LLVM can reason about invariant symbolic parameters
and exact backedge counts in cases P1 does not attempt. LLVM SCEV cannot be
called directly on `func.func` block arguments: it consumes LLVM IR, types,
and loops. P1 uses the existing P0 algebra instead of lowering the input IR
or duplicating LLVM's IR expansion machinery.

The unchanged P0 comparison script was rerun with its original C fixture,
writing **new build-local evidence**, leaving existing recorded evidence
untouched:

```sh
python3 test/polynomial-summation/llvm/compare_llvm.py \
  --clang /home/steven-studio/work/clang24-build/bin/clang \
  --opt /home/steven-studio/work/polly24-build/opt-asserts/bin/opt \
  --llvm-source /home/steven-studio/work/llvm-project \
  --output build/polynomial-summation-mlir/llvm-comparison
```

The resulting `report.json` records tool/source hashes, commands, version
strings, observations, and runtime counts. Observed behavior matched P0:
default O2 eliminated loops for powers 1..7, the general cubic, shifted
square, triangular system, and fixed square (exit `385`). The degree-8
accumulator remained unknown at the default size limit and two optimized
loops remained; with `-scalar-evolution-max-add-rec-size=16`, its AddRec was
recognized and its loop count became zero. Nonlinear self/doubling states
remained unknown. The volatile-store square had a recognized sum but retained
loops. The unknown-exit fixture was analyzed but not executed. These are
observations of those LLVM IR experiments, not optimization claims about P1.

## 8. Limits, complexity, and proposed P2 boundary

P1 intentionally rejects multiple/nested loops, irreducible or multi-entry
cycles, multiple backedges/exits, conditional entry or exit-tail paths,
conditional updates, unsupported structured regions, memory-dependent state,
non-additive self recurrences, cyclic increments, unproven dynamic casts,
symbolic initial values/coefficients, and unsupported arithmetic. A rejected
result has an explicit reason and no partial state forms. Rejection is
conservative: it is not a proof that a polynomial solution cannot exist.

Resource limits are 512 total blocks, 4,096 reachable operations, 128 header
states, scalar widths 1..4,096, expression traversal depth 128, and 65,536
characters per expanded transition/source expression. Shared DAG reporting is
memoized and checked before solving. P0 retains its degree cap 64 and its
expression limits. A generic degree-63 increment can integrate to degree 64;
triangular dependencies may increase degree further and then return UNKNOWN.
Exact integer coefficient bit sizes are not fixed machine widths and remain
subject to available memory. These bounds do not establish completeness or a
uniform runtime bound for all inputs below the caps.

For `B` blocks, `E` edges, `M` operations, and `V` states, the edge/path/SCC
checks are `O(B+E+M)` besides the reused verifier/dominance/loop analyses.
SSA DAG extraction is memoized. P0 substitution uses its existing exact
polynomial costs: addition/integration/difference `O(d)`, shift `O(d^2)`,
multiplication `O(d^3)`, and the current range helper `O(d^2)` arithmetic
operations. P1 separately substitutes each full transition and each source
arithmetic result; a conservative bound is `O((V+M)*M*d^3)` exact arithmetic
operations, plus P0 scheduling and reporting costs. Per-substitution DAG
memoization avoids tree expansion during algebra, but different source
substitutions do not share their polynomial cache. Bit costs depend on the
unbounded coefficient sizes. Reporting uses bounded textual expansion with
memoization. See P0 for the underlying algebra's storage and bit-cost caveats.

A future P2 CFG Recognition client could consume the extracted header/entry
mapping, initial constants, immutable transitions, dependency graph, integer
contract, and P0 forms. It must retain UNKNOWN as a fallback, keep CONDITIONAL
assumptions explicit, and separately justify any trip count, exit selection,
memory effects, casts, or emitted finite-width arithmetic. No such client,
production pass, CFG Recognition worktree change, GEMM behavior, Affine
Partitioning change, or RTL/runtime change is part of P1.
