# P2b Milestone 1: sound termination certificates

Implemented on 2026-10-30 in the isolated Polynomial Summation research build.
This milestone adds termination analysis; it does not implement a transformation,
trip-count analysis, exit-value analysis, memory relations, or tensor recognition.

## Baseline and preservation

Branch: `feat/polynomial-summation`. Starting HEAD and protected P2a tag
`polynomial-cfg-recurrence-p2a-20261030` both resolve to
`f2e59266ad89011f3d3558d83d23187909f810b5`.
P0 remains `057ac9e482ba9cb6c6cba7cd82b1c4d3f5e8bde1`; P1 remains
`5d0a5bdbed665ccd7d76cbd014c37e972b038f63`.

All implementation, build, and test additions are new P2b files. No tracked
P0/P1/P2a file, production compiler file, existing regression assertion, RTL,
Runtime, or other worktree was changed. The P2a `PROVEN`, `CONDITIONAL`, and
`UNKNOWN` contracts and its refusal to assert requested-visit reachability or
termination are unchanged. The existing `arith.extui nneg` correction is intact.
A previous 5,020-file SHA256 manifest and 31-file regression manifest were
checked: zero mismatches. Checkpoint creation follows successful validation.

## Public interface and result contract

`include/Systolic/MLIRPolynomialTermination.h` defines a separate namespace,
`systolic::polynomial::termination`, with:

- `generateCandidates(function, options)`: bounded deterministic proposals;
  an empty vector means no supported proposal, not nontermination.
- `verifyCertificate(function, certificate, options)`: reconstructs the source
  model, checks certificate provenance/domain, and verifies obligations.
- `analyze(function, options)`: tries zero-iteration, linear ranking, then
  recurrent-guard certificates with a shared query budget.
- `formatResult(result)`: deterministic JSON including replayable SMT-LIB.

The statuses are independent of P2a:

| Status | Meaning |
| --- | --- |
| `PROVEN_TERMINATING` | Every defined input in the explicitly reported nonempty domain exits the selected loop after finitely many continuing transitions. |
| `PROVEN_NONTERMINATING` | Every defined input in that domain starts in a verified closed recurrent set and continues indefinitely under the supported MLIR semantics. |
| `UNKNOWN` | No complete certificate was verified; includes unsupported syntax, mixed behavior, solver failures, and resource exhaustion. |

There is no conditional positive status or implicit no-overflow assumption in
P2b. Domain restrictions are explicit parts of both the analysis request and the
certificate. Verification refuses a certificate that silently strengthens the
requested domain. A failed verification returns no certificate. Diagnostic
models and failed queries may remain in an UNKNOWN report; they are not partial
proofs. `defined_total_path_verified` alone is not a termination certificate.

P2b deliberately offers no API to evaluate an exit value or infer a requested
header visit. A termination proof does not establish an exact iteration domain,
a trip count, or any memory-access relation. Nontermination applies to the
selected supported loop and defined input bit patterns, not arbitrary calling
contexts, external scheduling, or arbitrary program termination.

## Exact supported CFG and arithmetic subset

P2a's unconditional **modular** symbolic recurrence result must first be
`PROVEN`. Its mathematical/signed conditional assumptions and finite horizon
are never imported as termination facts. The verifier then reconstructs the
actual original SSA expressions independently of candidate generation and
checks their bitvector equality with P2a's initial/next-state expressions.

Supported functions have precisely one reachable reducible natural loop:

1. A unique unconditional, acyclic prelude reaches the header.
2. The header ends in `cf.cond_br`, with exactly one continuing successor and
   one exit successor. Branch polarity is normalized from the actual edges.
3. A single unconditional path traverses all loop blocks and returns through
   exactly one backedge. All header arguments have simultaneous next values.
4. A unique unconditional acyclic exit tail reaches `func.return`.
5. No other reachable blocks, nested regions, or hidden cycles are allowed.

The implementation reuses MLIR dominance, `CFGLoopInfo`, branch interfaces,
and P2a's SCC/topology checks. Noncanonical physical block order and unique
argument forwarding are supported. Unreachable blocks are ignored semantically
but still count toward the block cap; the entire input is verified MLIR.

States and every function parameter are scalar signless integers with a common
width `w` in 1..64. Constants, unflagged `arith.addi`, `arith.subi`, and
`arith.muli` are evaluated modulo `2^w`, including intermediate expressions.
There is no reinterpretation of their arithmetic as unbounded signed integers.

The continuation guard is either a constant Boolean, an invariant comparison,
or one header state compared with a proven invariant polynomial expression.
The supported predicates are `slt/sle/sgt/sge/ult/ule/ugt/uge`. Swapped operands
and false-edge continuation are normalized without losing signedness. The
controlling state's additive step must be an exact constant; ranking synthesis
uses nonzero steps. Step zero remains usable for recurrent/zero certificates.
Other states may follow P2a's additive acyclic polynomial recurrences, including
quadratic accumulator updates, symbolic initialization, and simultaneous updates.

All reachable operations must belong to the strict whitelist, including
operations outside the recurrence dependency graph. All overflow flags, casts
(including provable constant casts), calls, memory operations, division,
remainder, and unknown operations are rejected. Unknown attributes are rejected
on operations, the function, and enclosing operations. In particular,
`mustprogress`, `willreturn`, overflow flags, and `extui nneg` cannot be ignored.
This is intentionally more restrictive than P2a: no P2a semantic mode changes.
Even a constant false guard does not bypass the whitelist/totality checks.

## Formal source model

For immutable parameter bitvectors `p` and header state bitvectors `s`, extract:

- `D(p)`: a product of explicit source-width signed-representative intervals.
  Defaults include all bit patterns. Signed domain notation does not change
  unsigned guard semantics. Every function argument is included, even if P2a's
  recurrence does not use it (for example, a guard-only bound).
- `E(s,p)`: `s = e(p)`, the exact SSA entry transfer.
- `G(s,p)`: the actual continuation condition with normalized branch polarity.
- `T(s,s',p)`: `s' = f(s,p)`, the complete source-width backedge transfer.
- `I(s,p)`: typed states, optionally intersected with supplied state intervals.
  Candidate generation uses the full typed state domain (`I = true`). No general
  invariant synthesis is performed.

Expressions for `e` and `f` come from original SSA definitions and branch
operands. Parallel SMT `let` bindings substitute all next values at once; a
new state value is never accidentally substituted into another update RHS.
The verifier checks source/P2a expression agreement universally, not by sampling.

Totality is a **structural proof**, not an implication with a possibly empty
transition relation. Verification accepts only total, unflagged operations on
defined bitvectors; verifies the unique complete continuing path; and constructs
one next expression per state. Thus every `D ∧ I ∧ G` state has exactly one
modeled transition. There is no partial `T` filter that can make descent vacuous.
The finite prelude and tail contain no calls or effects that could conceal
nontermination. The semantic input domain excludes poison/undef arguments;
those are not integer bit patterns and are not manufactured as parameters.

## Ranking certificates and obligations

For an ascending comparison, propose `rho = value(bound) - value(counter) + c`;
for descending, reverse the difference. `value` uses the guard's signed or
unsigned interpretation. `c=0` for strict and `c=1` for inclusive comparisons.
The verifier permits certificate offsets only in `{-1,0,1}` and verifies the
actual claim; choosing a direction or an offset is not itself a proof.

Ranks use signed `w+2`-bit arithmetic after sign- or zero-extending the original
operands. The difference of any two source values plus these offsets lies in
`[-2^w, 2^w]`, which fits exactly in signed `w+2` bits. Consequently the rank
comparison is a comparison of mathematical integers, while source transitions
remain exact `w`-bit operations. No division, approximation, or overflow
assumption is introduced.

The certificate verifier requires a SAT witness for nonempty `D` and proves
UNSAT for the negation of each required obligation:

1. Source agreement between independently extracted `e,f` and P2a's equations.
2. Initial invariant: `D ∧ E ⇒ I`.
3. Inductiveness: `D ∧ I ∧ G ∧ T ⇒ I'`.
4. Well-foundedness: `D ∧ I ∧ G ⇒ rho ≥ 0`.
5. Descent: `D ∧ I ∧ G ∧ T ⇒ rho' < rho`.
6. Continuing-successor well-foundedness:
   `D ∧ I ∧ G ∧ T ∧ G' ⇒ rho' ≥ 0`.

Source agreement and the two invariant counterexamples are a disjunction in
one UNSAT query, which proves each separately. The last obligation is explicitly
checked even though it also follows from invariant preservation and item 4.

An infinite execution would yield an infinite strictly decreasing sequence of
natural numbers, contradicting well-foundedness. This argument does **not**
require `rho' ≥ 0` on an exiting successor. Example: signed i8 `i < 126`, step 2,
state `i=125` has rank 1; successor 127 exits with rank -1. The verifier accepts
this valid overshoot. Restricting the bound to `[-128,126]` gives a universal
certificate; allowing bound 127 introduces mixed parity behavior and returns
UNKNOWN.

## Zero-iteration and nontermination certificates

A zero-iteration certificate proves `D ⇒ not G(e(p),p)`, after the same source,
invariant, and totality checks. It establishes zero backedges, not a guessed
exit expression.

The only synthesized recurrent set is `R = I ∧ G`. Nontermination verification
requires:

1. Every permitted entry satisfies `I` and `G`.
2. Every `D ∧ I ∧ G` transition preserves both `I` and `G`.
3. Every such transition is defined, total, and the complete continuing path.
4. No unsupported progress attributes or hidden effects are present.

By induction, every allowed entry has an infinite defined execution inside R.
The proof is independent of failed ranking attempts. It covers constant true
guards, suitable stuttering loops, and some wraparound loops with an always-true
guard. It does not synthesize arbitrary recurrent sets or use cycle/time limits
as proof evidence. Mixed input domains fail the universal entry/closure or
ranking obligations and remain UNKNOWN.

## Certificate format and solver isolation

The public `Certificate` contains kind, source SHA256, canonical guard, exact
parameter domain, optional invariant intervals, and (for a ranking) counter
index, direction, signedness, and offset. `verifyCertificate` re-extracts E/G/T
from the supplied function; a caller cannot substitute a fabricated transition
relation, assume an invariant, or reuse a certificate on a different function.

JSON reporting includes the model, domain, limits, source fingerprint, solver
version, successful certificate (or null), and every attempted verification
query with name, expected result, actual result, diagnostic, and complete
SMT-LIB. Failed candidates can precede a successful certificate; those earlier
SAT counterexample queries are intentionally retained. Only the final complete
successful certificate authorizes a positive status. Ranking queries contain
explicit `R_s/R_e/R_n` definitions, so attempted ranking candidates are recorded
even when rejected. Fingerprints hash the generic function with debug locations;
source relocation or location changes can intentionally invalidate reuse.

The new standalone CMake project links the system Z3 4.8.12 C++ API only into the
new termination library/driver/tests. LLVM/MLIR 24.0.0git at commit
`7615da10b75c57286a4032eac8bb43c1b8d2ba46` is reused without rebuilding LLVM or
enabling its Z3 support. The original P2a report binary has no Z3 dependency.
No production target gains a dependency or pass.

Each query declares `QF_BV` and is replayable. Both solver API limits and script
limits are recorded. A separate Python C-API consumer replays the emitted
scripts without calling the project verifier. This uses the same installed
Z3, not a second solver implementation. The trusted base is MLIR's verifier and
integer/CFG semantics, the restricted source encoder and structural checker,
and Z3's SAT/UNSAT answers. This milestone does not produce independently checked
resolution proof objects. Small-width exhaustive exploration and native LLVM
runs test the encoder but do not replace the universal SMT proof.

Timeout, UNKNOWN, parser errors, or budget exhaustion return UNKNOWN and no
certificate. Unknown top-level SMT commands are explicitly rejected: Z3 4.8.12
can otherwise warn and ignore such a command. The emitted command vocabulary
is checked before parsing. Candidate discovery and certificate verification
remain separate; malformed/corrupted candidates cannot confer a status.

## Resource limits and conservative boundaries

| Resource | Default and maximum |
| --- | ---: |
| Blocks / reachable operations | 64 / 512 |
| Header states / function parameters | 16 / 16 |
| SSA expression nodes / traversal depth | 4096 / 128 |
| Candidate proposals / total queries | 4 / 16 |
| Solver timeout per query | 2000 ms |
| Z3 deterministic resource budget per query | 5,000,000 |
| SMT model/query bytes | 1,048,576 |
| Source width | 1..64 |

Callers may lower these limits; zero or attempts to raise them return UNKNOWN.
The query budget is shared across candidates. SMT complexity can be exponential;
these caps are not a promise of a particular acceptance rate or total wall time
(including MLIR parsing). P2a/P0's existing algebra/expression limits remain
additional prerequisites. In particular P2a's parameter-monomial degree 16
boundary has not been raised or repurposed as a termination bound.

Unsupported cases include nested/multiple/irreducible loops; multiple backedges;
conditional updates; ambiguous forwarding; symbolic/state-dependent controlling
steps; nonlinear state guards such as `i*i < N`; varying bounds; equality guards;
casts and flags; memory and calls; incompatible widths; nonlinear cyclic
recurrences; lexicographic/multiphase/polynomial ranking synthesis; arbitrary
invariants or recurrent sets; and domains requiring relational constraints
rather than the supported intervals. Some universally terminating wraparound
loops return UNKNOWN because their guard-distance candidate is not decreasing.

## Adversarial tests and counterexamples

The C++ tests verify all 36 fixtures, repeat reports, preserve original IR and
P2a results, independently reverify positive certificates, and reject 14 mutated
certificates (direction, fingerprint, guard polarity description, narrowed/missing
domain, counter, signedness, offset, initiation, inductiveness, empty invariant,
certificate kind). They also exercise actual Z3 UNKNOWN, a 1 ms timeout on a hard
bitvector query, unsupported SMT syntax, malformed options, and resource limits.

The independent Python suite interprets the original generated SSA text and
simultaneous branch transfers using Python integers with explicit source-width
reduction. All start/bound bit patterns are enumerated for widths 3 and 4.
Additional source-width boundary tests cover 8, 16, 32, and 64 bits, plus a
noncanonical forwarded quadratic accumulator. A repeated **complete header
state** establishes a cycle in the finite reference interpreter; an iteration
watchdog alone never does. Mixed reference behavior must yield UNKNOWN.

Selected discoveries/checks:

- i8 `i <= 127; i += 1` is nonterminating for every initial bit pattern: the
  signed guard is always true and wraparound stays in the recurrent set.
- i8 `i < 127; i += 2` has mixed behavior: start 126 stays on the even residue
  cycle forever; start 125 exits at 127. It returns UNKNOWN on the full domain.
- i8 `i > bound; i += 1` can terminate through wraparound, but the proposed
  descending distance increases before wraparound. Failing that candidate is
  not a nontermination proof; the result is UNKNOWN.
- Rank 1 to rank -1 on an exiting successor is accepted (overshoot example
  above). Requiring nonnegativity on all successors would reject it incorrectly.
- Invalid initial or non-inductive interval invariants cannot hide bad states.
- `nsw`, `nuw`, `extui nneg`, progress attributes, Collatz-like branches,
  lexicographic resets, multiphase updates, and polynomial guards return UNKNOWN.
- An unsupported SMT command initially exposed Z3's warn-and-ignore behavior;
  the new wrapper's command check fixed this before final validation. No P2a
  implementation or existing expectation was changed.

Native tests lower the original MLIR to LLVM, add explicit ABI wrappers, and
execute in a separate child with a strict 10 s timeout. For nontermination
cases, separate execution-only clones add a 64-transition watchdog; their
prefixes are evidence about updates, never nontermination proofs. The local
Func-to-LLVM pass does not convert `func.return` in unreachable blocks, so the
execution copy is canonicalized to remove dead blocks before conversion.
The analyzed module and the source-SSA oracle retain the original CFG.

## Reproducible validation

Commands run locally (each final command exited **0**):

```sh
cmake -S research/polynomial-termination \
  -B /tmp/polynomial-termination-p2b-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DMLIR_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/mlir \
  -DLLVM_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/llvm
cmake --build /tmp/polynomial-termination-p2b-build -j3
ctest --test-dir /tmp/polynomial-termination-p2b-build -V

python3 test/polynomial-termination/differential.py \
  --driver /tmp/polynomial-termination-p2b-build/polynomial-termination-report \
  --mlir-opt /home/steven-studio/work/llvm-project/build/bin/mlir-opt \
  --mlir-translate /home/steven-studio/work/llvm-project/build/bin/mlir-translate \
  --clang /home/steven-studio/work/clang24-build/bin/clang \
  --z3-library /usr/lib/x86_64-linux-gnu/libz3.so

python3 test/polynomial-summation/llvm/compare_llvm.py \
  --clang /home/steven-studio/work/clang24-build/bin/clang \
  --opt /home/steven-studio/work/polly24-build/opt-asserts/bin/opt \
  --llvm-source /home/steven-studio/work/llvm-project \
  --output /tmp/polynomial-termination-p2b-llvm-comparison
```

Combined CTest: **12/12 PASS, 0 FAIL**. Counts below are per final run, not
cumulative totals across repeated validation commands:

| Suite | Observed result |
| --- | --- |
| P2b C++ | 667 checks PASS; 36 verified fixture functions; 28 terminating, 5 nonterminating, 56 expected UNKNOWN asserted results (including certificate re-verification/mutations); 14 rejected certificate mutations |
| P2b independent | 101 verified functions; 112 status probes: 32 terminating, 6 nonterminating, 74 expected UNKNOWN; 66 mixed domains rejected |
| P2b source-SSA oracle | 14,251 executions: 12,049 exits, 2,202 complete-state cycles |
| P2b SMT replay | 967 obligations replayed with matching SAT/UNSAT results |
| P2b native LLVM | 2,377 original terminating executions and 338 bounded nonterminating prefixes PASS (2,715 total) |
| P2b lit | 8/8 PASS, including nested/multiple/irreducible CFG rejection and IR/report preservation |
| P0 C++ | 42,277 checks PASS |
| P0 Python | 13,443 comparisons PASS |
| P1 C++ | 517,759 checks PASS; 1,961 independent APInt executions; 233,023 header-state comparisons |
| P1 nneg | 1,244 checks PASS; 27 accepted mode cases, 65 expected UNKNOWN; 594 direct comparisons |
| P1 Python | 98 accepted functions, 985 assertions; 27,702 exact and 27,702 modular values, 27,702 differences, 201 lowered values, 6 resource rejections |
| P1 lit | 7/7 PASS |
| P2a C++ | 41,653 checks PASS; 766 independent executions, 16,484 comparisons; 64 PROVEN, 22 CONDITIONAL, 121 expected UNKNOWN |
| P2a Python | 1,600 executions; 39,240 exact and 39,240 modular comparisons; 39,240 differences; 4,800 initial checks; 9 expected UNKNOWN and 3 expected CLI errors |
| P2a lit | 4/4 PASS |
| Existing LLVM/SCEV comparison | 1,814 default-O2 and 1,814 expanded-AddRec-limit runtime comparisons PASS |

Full production compiler, FPGA, Runtime, and full upstream LLVM/MLIR suites:
**NOT RUN (4 suite groups)**. None of their code or build targets changed. No
new optimization behavior is attributed to LLVM; the P0 comparison script was
rerun unchanged. Test logs and native/SMT build artifacts are outside the source
tree under `/tmp`; replayable obligations can also be regenerated from the
reporting executable. Existing P0/P1/P2a tests were not deleted or weakened.

## Focused boundary review before checkpoint

The implementation already preserves the original source-width guard and uses
`w+2`-bit rank arithmetic. No implementation correction was necessary. For
ascending guards the strict distance `N-i` equals the inclusive distance
`(N-1)-i+1`; the descending identity is `i-N = i-(N+1)+1`. Adjustments in these
identities are widened mathematical operations, never unchecked source-width
bound rewrites. At a signed/unsigned extremum the widened bound may lie outside
the source type; it must not be truncated and substituted into the original
comparison. The original comparison is retained in all verification queries.

New additive C++ regressions prove these symbolic guard/rank identities with
12 UNSAT obligations across widths 1, 8, and 64, both signednesses and both
directions. Another 12 SAT obligations demonstrate why source-width adjustment
at the corresponding extremum is invalid. Four endpoint analyses verify
zero-iteration results. Existing assertions are unchanged.

For signed i8 initial `i=0`, `i < 127`, step 2, analysis returns **UNKNOWN** with
no certificate: the descent obligation has a counterexample, while the current
recurrent-set template cannot express even parity. The equivalent `i <= 126`
fixture also returns UNKNOWN. The independent test checks all 128 distinct even
residues, verifies both guards throughout, and checks exact return to state 0.
This is a closed finite-state cycle under wrapping addition, not timeout-based
evidence, and confirms nontermination of this specific input without claiming
the analyzer synthesized a parity certificate.

Focused review commands (both exit 0):

```sh
cmake --build /tmp/polynomial-termination-p2b-build -j3
ctest --test-dir /tmp/polynomial-termination-p2b-build -V
```

The combined suite again passed 12/12, including all P0/P1/P2a regressions and
the existing independent LLVM execution/SMT replay checks. The C++ total above
includes the boundary additions. The separate P0 LLVM/SCEV comparison counts
above refer to the earlier milestone validation, not a rerun during this focused
boundary review. No production implementation files changed.

## File inventory and completion boundary

New implementation/interface:

- `include/Systolic/MLIRPolynomialTermination.h`
- `lib/Systolic/Analysis/PolynomialTerminationInternal.h`
- `lib/Systolic/Analysis/MLIRPolynomialTermination.cpp`
- `lib/Systolic/Analysis/PolynomialTerminationProof.cpp`

New isolated build/reporting:

- `research/polynomial-termination/CMakeLists.txt`
- `research/polynomial-termination/PolynomialTerminationReport.cpp`
- `research/polynomial-termination/lit.site.cfg.py.in`
- `research/polynomial-termination/lit/lit.cfg.py`
- `research/polynomial-termination/lit/{cfg-boundary,errors,preservation,ranking,recurrent,semantics,unknown,zero}.mlir`

New tests/documentation:

- `test/polynomial-termination/Inputs/loops.mlir`
- `test/polynomial-termination/TerminationTest.cpp`
- `test/polynomial-termination/differential.py`
- `docs/POLYNOMIAL_SUMMATION_P2B.md`

Milestone 1 is ready for review within this documented subset. No unresolved
soundness defect was found in the final validation. Z3, the semantic encoder,
and MLIR remain explicit trusted components; tests alone do not prove their
correctness. No lexicographic, multiphase, or polynomial ranking synthesis,
general invariant synthesis, symbolic-step extension, or later milestone work
is included. Merging the verified checkpoint additionally requires a clean main
worktree; unrelated local work must not be discarded or overwritten.
